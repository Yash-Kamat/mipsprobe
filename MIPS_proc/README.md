# MIPS_proc — the FPGA MIPS core

A 5-stage pipelined, MIPS-style processor with forwarding, load-use stalling and
dynamic branch prediction, wrapped in an I2C slave. Through that slave the STM32
probe in this repository loads programs, starts the core, and reads memory and
registers back. The design targets the **ZedBoard** (`xc7z020clg484-1`).

| Path | Contents |
|---|---|
| `rtl_files/` | Verilog sources; the top module is `top` in `top_1_2.v` |
| `constraints/const.xdc` | ZedBoard pin and clock constraints |
| `sim/` | Icarus testbench: `make -C MIPS_proc/sim` loads and runs `full_test` over a simulated I2C bus |

These files are **copies** of the Vivado project's sources. After changing any of them,
copy them back into the project before regenerating the bitstream (see
[RTL changes for readback](#rtl-changes-for-readback)).

## Board connections

| Port | ZedBoard | Notes |
|---|---|---|
| `clk` | Y9 (100 MHz oscillator) | constrained at 20 ns — see [Clock constraint](#worth-checking-the-clock-constraint) |
| `rst` | SW0 (F22) | active high: switch up = held in reset |
| `led` | LD0 (T22) | lights when DONE sets; goes off if RUN is cleared, and stays off until reset |
| `scl` | JA1 (Y11) | internal pull-up |
| `sda` | JA2 (AA11) | internal pull-up, open-drain |

Probe wiring: STM32 **PB6 → JA1** (SCL), **PB7 → JA2** (SDA), and **GND ↔ GND**. Both sides
are 3.3 V, and both enable their internal pull-ups.

## I2C interface

The 7-bit slave address is **`0x42`**, and the bus runs at 100 kHz. The slave exposes a 16-bit
address space of 32-bit words, and every transaction carries exactly **one word, MSB first**:

```
write: [0x42+W] [addr15:8] [addr7:0] [data31:24] [data23:16] [data15:8] [data7:0] STOP
read:  [0x42+W] [addr15:8] [addr7:0]  Sr  [0x42+R] [data31:24] [data23:16] [data15:8] [data7:0] NACK STOP
```

- A write takes effect when its last data byte arrives. A 7th payload byte is NACKed.
- A read reuses the 2-byte address write to set the pointer. The slave snapshots the value when it
  ACKs the read address. If the master keeps ACKing past the 4th byte, the extra bytes read `0xFF`.
- Unmapped addresses read back `0xDEADBEEF`.

On the STM32 side these map to `FPGA_MIPS_write()` and `FPGA_MIPS_read()` in `src/fpga_mips.c`.

## Memory map

Word *k* of each memory region lives at *base + 4k*. Inside the memory regions, writes are only
accepted at addresses whose low two bits are `00`.

| Address | Name | Access | Contents |
|---|---|---|---|
| `0x0000`–`0x03FC` | IMEM | R/W | Instruction memory, 256 words. The address **is** the PC the CPU fetches that word from. |
| `0x1000`–`0x107C` | REGFILE | R/W | `r0`–`r31` (`r0` at `0x1000`, `r31` at `0x107C`). Writes to `r0` are ignored. |
| `0x2000`–`0x23FC` | DMEM | R/W | Data memory `DM[0]`–`DM[255]` |
| `0x3000` | CSR | bit0 R/W, bit1 R | bit0 **RUN**: 0 freezes the whole pipeline, 1 runs it. bit1 **DONE**: set by hardware. |
| `0x3004` | TARGET_PC | R/W | PC of the first instruction that must **not** execute, normally `4 × instruction count` |
| `0x3008` | PC | R | The CPU's live program counter |

Read access to every address, and the `PC` register itself, were added for the probe's readback
(see [RTL changes for readback](#rtl-changes-for-readback)).

### Compared with the register map the probe originally assumed

The STM32 driver was first written against an assumed register map, before the RTL existed.
Only the byte order turned out to match:

| | Originally assumed | What the RTL does |
|---|---|---|
| Slave address | `0x50` | `0x42` |
| Registers | four 8-bit registers at `0x0`–`0x3` | 16-bit address space of 32-bit words (table above) |
| Loading memory | one register per memory, fed as a byte-at-a-time FIFO | each word written to its own address |
| Transaction | register byte + any number of data bytes | exactly one word: 2 address bytes + 4 data bytes |
| TARGET_PC | instruction count | PC value (`4 × count`) |
| CSR bit0 | cleared by hardware when finished | stays set; DONE (bit1) signals completion |
| Reads | CSR readable | none at all — now added |
| Register file | not accessible | writable, now readable |
| Byte order | big-endian | big-endian ✓ |

## Run lifecycle

1. **Reset**: flip SW0 up, then down. This zeroes the PC, the pipeline, CSR, TARGET_PC, DONE and all
   of DMEM, and resets every register to its own index. IMEM is **not** cleared. DMEM is block RAM,
   which can't be cleared in one cycle, so a counter walks its 256 words writing zero — about 5 µs at
   50 MHz, finished long before the MCU can set RUN over a 100 kHz bus.
2. **Load** while RUN = 0: write the instruction words, the data words (and optionally registers),
   then TARGET_PC.
3. **Run**: write CSR = 1. The pipeline starts fetching at PC 0.
4. **Stop**: the cycle the PC reaches TARGET_PC, IF sends a NOP into the pipeline instead of fetching
   and the PC holds. The NOP flows through ID, EX, MEM and WB like a stall while the instructions
   already in flight finish. Four cycles later DONE sets, LD0 lights, and the pipeline freezes.
5. DONE stays set until the next reset, so **reset before every new load**. The console's
   `fpga load` refuses to load onto a core that has already been started.

## Instruction set

The control unit decodes only the 6-bit **opcode**. The `funct` and `shamt` fields are ignored.

```
R-type:  | opcode (6) | rs (5) | rt (5) | rd (5) | shamt (5) | funct (6) |
I-type:  | opcode (6) | rs (5) | rt (5) |          immediate (16)         |
          31       26   25   21  20   16  15   11  10       6   5        0
```

| Mnemonic | Assembly (`isa.inc`) | Type | RTL opcode | Standard MIPS encoding | Semantics | Matches standard MIPS? |
|---|---|---|---|---|---|---|
| add  | `ADD rd, rs, rt`    | R | `000000` (0x00) | opcode 0x00, funct 0x20 | `rd = rs + rt` | Encoding yes. No overflow trap (behaves like `addu`), and funct is ignored |
| sub  | `SUB rd, rs, rt`    | R | `000001` (0x01) | opcode 0x00, funct 0x22 | `rd = rs − rt` | **No** — standard 0x01 is REGIMM (`bltz`/`bgez`) |
| addi | `ADDI rt, rs, imm`  | I | `000010` (0x02) | opcode 0x08 | `rt = rs + sign_extend(imm)` | **No** — standard 0x02 is `j` |
| lw   | `LW rt, off, base`  | I | `000011` (0x03) | opcode 0x23 | `rt = DM[(base + sign_extend(off))[7:0]]` | **No** — standard 0x03 is `jal`; word-indexed |
| sw   | `SW rt, off, base`  | I | `000100` (0x04) | opcode 0x2B | `DM[(base + sign_extend(off))[7:0]] = rt` | **No** — standard 0x04 is `beq`; word-indexed |
| beq  | `BEQ rs, rt, label` | I | `000101` (0x05) | opcode 0x04 | `if (rs == rt) PC = PC + 4 + (sign_extend(off) << 2)` | **No** — standard 0x05 is `bne`; no delay slot |
| NOP  | `NOP`               | – | `111111` (0x3F) | — (standard `nop` is `0x00000000`) | nothing: `0xFC000000` | Specific to this architecture |
| any other opcode | – | – | – | – | nothing | Standard MIPS would raise a reserved-instruction exception |

`NOP` is `0xFC000000`, the same bubble the pipeline inserts on reset, on a branch flush, and while
stopping at TARGET_PC. Every control signal is zero, so nothing is written.

### Conformance to standard MIPS

**This is a MIPS-*like* 6-instruction ISA, not a subset of standard MIPS.** It reuses MIPS's field
layout, 32-register numbering, sign extension, branch-offset formula and big-endian byte order, so
programs *read* like MIPS. But only `add` shares its standard encoding, so a stock MIPS assembler or
compiler produces code that runs wrong here. That's why `mips_assembly_test/isa.inc` emits this
core's encodings directly.

Other differences from standard MIPS32:

- **Word-addressed data memory.** The `lw`/`sw` address is a word index (`LW r8, 3, r0` reads `DM[3]`),
  and only its low 8 bits are used, so it wraps at 256. Standard MIPS uses byte addresses.
- **No branch delay slot.** The instruction after a taken `beq` never executes.
- **No exceptions.** No overflow trap on `add`/`addi`, and unknown opcodes do nothing.
- **Small address spaces.** 256 instruction words, fetched through `PC[9:0]`, and 256 data words.

Not implemented: jumps (`j`, `jal`, `jr`, `jalr`), other branches (`bne`, `blez`, `bgtz`, `bltz`,
`bgez`), comparisons (`slt`, `slti`, `sltu`), logic (`and`, `or`, `xor`, `nor` and their immediates),
`lui`, shifts, `mult`/`div`, byte/half-word loads and stores, `syscall`/`break`, and CP0. The absence of
`lui`/`ori` also means constants wider than 16 bits must be preloaded into data memory.

## Microarchitecture

| Stage | What happens |
|---|---|
| IF | IMEM read at `PC` (block RAM, addressed a cycle early with `PC_next` — see [Memories and block RAM](#memories-and-block-ram)). `beq` instructions consult a two-level predictor, other instructions go to PC+4. |
| ID | Opcode-only control unit, sign extension, register read with a WB→ID write-before-read bypass |
| EX | Forwarding from EX/MEM and MEM/WB; ALU (add/sub); branch target and decision |
| MEM | Data memory read/write. A load's result lands one cycle later, in WB, straight out of the RAM's output register |
| WB | MemtoReg mux, register write |

- **Branch prediction**: PHT (16 × 4-bit history, indexed by `PC[5:2]`) → BHT (16 × 1-bit, indexed
  by history XOR `PC[5:2]`) → BTB (16 × 32-bit targets). A branch whose predicted next PC was wrong
  flushes IF/ID and ID/EX and redirects the PC.
- **Load-use stall**: when an `lw` in EX writes a register the instruction in ID reads, PC and IF/ID
  hold for one cycle and a bubble enters ID/EX.
- **RUN = 0 or DONE = 1** freezes every pipeline register and the PC.

## Memories and block RAM

IMEM and DMEM are **block RAM**. Both are instances of one wrapper,
`rtl_files/sram_1rw1r_1_2.v`: 256 words of 32 bits, one read/write port plus one read-only port
(1RW + 1R), synchronous reads, no reset. On the ZedBoard each becomes a single `RAMB36E1` —
true-dual-port caps a port at 36 bits, so a 32-bit port can't pack into the smaller `RAMB18E1`.

They did not always infer that way. The original IMEM was `reg [7:0] IM [1023:0]` read as four
*separate* byte lookups (`IM[PC]`, `IM[PC+1]`, `IM[PC+2]`, `IM[PC+3]`) plus four more for the I2C
readback, and DMEM was a `reg [31:0] DM [255:0]` with an `always @(*)` read and a second async read
port. Two things stopped inference: the reads were combinational, and there were far more read ports
than any RAM primitive has. The result was ~16 k flip-flops and several thousand LUTs of address
muxing, on the CPU's own critical path. Three rules keep it inferring now, and all three also hold
for an ASIC SRAM macro:

- reads are synchronous — no `always @(*)` read, ever;
- at most one write port and two read ports;
- **no reset on the memory or its output register** (an *asynchronous* reset in particular kills
  inference outright).

### The one-cycle latency contract

A block RAM registers its address: read data appears on the clock edge **after** the address is
presented. There is no combinational-read mode — that is distributed/LUT RAM, which is effectively
what this design used to be. Latency is 1 cycle, or 2 if you enable the optional output register.

That one cycle costs the pipeline nothing here, because each memory already had a pipeline register
sitting on its read path. Both were absorbed rather than paid for:

- **IMEM is addressed one cycle early.** `Pc` now exposes `PC_next`, the combinational value `PC_out`
  takes at the next edge, and the fetch port is addressed with `PC_next[9:2]` instead of
  `PC_out[9:2]`. The RAM's output register therefore presents the instruction at `PC_out` during
  exactly the cycle the old combinational read did. `comparator` → `mux_2` → `PC_in` still resolves
  inside the fetch cycle, so branch prediction is untouched. This is the one thing to understand
  before editing the fetch stage — it is why `PC_next` exists.
- **DMEM lands in WB.** The read result was already consumed one stage later, by `M_WB_Register`.
  The RAM's output register now *is* that register: `M_WB_Register` no longer carries `Rd_data`, and
  `data_memory.rd_data` feeds `Write_back_mux` directly. A back-to-back `SW` then `LW` of the same
  address stays correct because both still address the RAM in MEM, one cycle apart.

Neither memory relies on the output holding its value while its enable is low — during a stall or
freeze the address is held, so the RAM simply re-reads the same word every cycle. That matters for
the ASIC port; see below.

**No cache is needed, and none should be added.** Caches exist to hide *multi-cycle or variable*
memory latency. A fixed 1-cycle on-chip memory is precisely what the classic 5-stage MIPS pipeline
is designed around, and the two tricks above absorb it with zero added stalls.

### Building the RAM with the Block Memory Generator (Vivado 2020.2)

`sram_1rw1r` infers a block RAM on its own, which is why it is written that way — it stays portable
and it still simulates in Icarus. If you would rather instantiate the IP explicitly, generate it and
swap the body of that one module:

1. **IP Catalog** → *Memories & Storage Elements* → *RAMs & ROMs & BRAM* → **Block Memory Generator**
   (v8.4). Name it `blk_mem_gen_0`.
2. **Basic** tab: Interface Type **Native**; Memory Type **True Dual Port RAM**; tick **Common Clock**
   (this design has one clock); ECC none; Write Enable → Byte Write Enable **unchecked** (writes are
   always full words); Algorithm *Minimum Area*.
3. **Port A Options**: Write Width **32**, Write Depth **256** (read width/depth follow). Operating
   Mode *No Change* — the CPU never reads and writes in the same cycle, so write-first vs read-first
   is a don't-care and *No Change* uses the least power. Enable Port Type **Use ENA Pin**.
   **Leave "Primitives Output Register" unchecked.** This is the setting that matters: checking it
   makes the read latency 2 cycles, which would break the `PC_next` and WB-stage arguments above and
   force a real pipeline change.
4. **Port B Options**: same width and depth. Port B is read-only in both memories, so Operating Mode
   *No Change* and `web` tied to 0. Enable Port Type **Use ENB Pin**. **Primitives Output Register
   unchecked** here too.
5. **Other Options**: no COE / Load Init File — both memories are filled over I2C, and an ASIC SRAM
   has no initial contents either.
6. The **Summary** page should report **1** `RAMB36E1` and a read latency of **1**. If it says 2,
   go back and clear the output-register checkboxes.

Then replace the body of `sram_1rw1r` with the instance:

| `blk_mem_gen_0` | `sram_1rw1r` |
|---|---|
| `clka`, `clkb` | `clk` (Common Clock, so both tie to the same net) |
| `ena` / `wea` / `addra` / `dina` / `douta` | `en0` / `we0` / `a0` / `d0` / `q0` |
| `enb` / `web` / `addrb` / `dinb` / `doutb` | `en1` / `1'b0` / `a1` / `32'b0` / `q1` |

`wea`/`web` are 1-bit vectors (`[0:0]`) unless byte write enable is turned on. Note that the `.xci`
lives in the Vivado project, not in `rtl_files/`, so an IP-based build no longer simulates in Icarus
without the Xilinx libraries — which is the main reason the inferred version is the one committed.

### Porting to an ASIC SRAM macro

`sram_1rw1r` was shaped to match what the open-source flows actually offer, so the swap is mechanical
rather than a redesign. **The macros are 1-cycle too**, with the same protocol as a block RAM. The
OpenRAM behavioural model for `sky130_sram_1kbyte_1rw1r_32x256_8` captures `addr0`/`csb0`/`web0` on
`posedge clk0` and drives `dout0` on the *following* `negedge`; a synchronous consumer sampling at
the next posedge sees the data, which is 1-cycle latency. (The negedge is how the model represents
access time, not a second cycle. For real timing, read the macro's `.lib` access time — that
determines how much of the cycle is left for the logic downstream, and the load-forwarding path out
of DMEM is the one to watch.)

`sky130_sram_1kbyte_1rw1r_32x256_8` is 1 KB, 1RW + 1R, 32 bits wide, 256 deep — the exact shape of
each of these memories. Its control pins are active **low**:

| Macro pin | `sram_1rw1r` |
|---|---|
| `clk0`, `clk1` | `clk` |
| `csb0` / `web0` / `wmask0` / `addr0` / `din0` / `dout0` | `~en0` / `~we0` / `4'b1111` / `a0` / `d0` / `q0` |
| `csb1` / `addr1` / `dout1` | `~en1` / `a1` / `q1` |

Four rules make that swap safe, and the RTL already obeys all four:

1. **Never rely on the output holding when the enable is low.** A block RAM holds; OpenRAM drives X.
   Here every consumer either re-presents the same address each cycle (stalls and freezes hold the
   address) or ignores the data — the DMEM output is only used when `MEM_WB_WB` says `MemtoReg`.
2. **Never put a reset on the memory output register.** Macros have none. Nothing here needs one:
   after reset `PC_next` is 0, so the fetch port re-reads word 0 every cycle and self-corrects.
3. **Never rely on power-up contents.** IMEM is always I2C-loaded, and DMEM's reset-clear is an
   explicit 256-cycle counter walk rather than a `for` loop over the array.
4. **Full-word writes only**, so byte write masks are a don't-care.

On **Tiny Tapeout** specifically, the current IHP SG13G2 shuttles offer single-port SRAMs from 256×8
to 1024×32 and dual-port up to 1024×32, but they cost real tile area — the published figures are
146.88 × 336.46 µm / 2×1 tiles for 1024×8, 416.64 × 191.34 µm / 3×4 tiles for 512×32, and
685.49 × 385.37 µm for a dual-port 1024×32. Two 256×32 memories are feasible but multi-tile. Sky130
density is far worse (roughly 1200 bits per tile for an experimental register file). Two caveats
worth knowing before committing: TT's memory page documents **area but not latency**, so confirm
against the macro's own `.lib`; and the IHP `RM_IHPSG13_1P_*` parts are **single-port**, which this
design's I2C readback port cannot use as-is — with a 1-port macro the readback would have to steal a
cycle from the CPU, or be restricted to when the core is halted.

### Deliberately still flip-flops

`reg_file` (32×32) and the `PHT`/`BHT`/`BTB` predictor tables (16 entries, 592 bits in total) are
**not** memories and should stay that way. No SRAM macro exists that shallow, a block RAM or macro
would be far larger than the flops it replaced, real cores use flip-flops for a register file
anyway, and their asynchronous reads sit on the fetch critical path. Keeping the register file as
flops is also what preserves its three async read ports and its reset-to-own-index behaviour.

## Program conventions

These are deliberate design decisions of this architecture:

- **`r0` is hardwired zero and is never a destination.** No instruction writes it, and it is not used
  as a special-purpose nop. That's why the NOP is `0xFC000000`, not the standard `0x00000000`
  (which would be `add r0, r0, r0`).
- **Registers reset to their own index** (`r8 = 8`, `r31 = 31`, …). A program initializes every register
  it reads.
- **Programs end by falling through to TARGET_PC.** The stop happens the cycle the PC reaches
  TARGET_PC, so a program ends with at least two non-branch instructions (e.g. `NOP`s) right before it,
  and never branches to TARGET_PC. The TARGET_PC stop is only ever triggered by normal fall-through.
- **Registers are written `r0`–`r31`** in assembly and documentation.

## Writing and building programs

`mips_assembly_test/` holds the toolchain glue and the test program:

| File | Purpose |
|---|---|
| `isa.inc` | `r0`–`r31` names and the `ADD`/`SUB`/`ADDI`/`LW`/`SW`/`BEQ`/`NOP` macros, emitting this core's encodings |
| `full_test.S` | Self-checking test of every instruction and the pipeline's hazard handling |
| `Makefile` | `clang` → `llvm-objcopy` → `tools/bin2c.py` → `src/mips_full_test.h` |

```bash
make DEMO=mips_console_demo flash          # assemble, rebuild and flash, in one step
```

The root `Makefile` regenerates `src/mips_full_test.h` by itself whenever `full_test.S` or
`isa.inc` is newer than it, and `-MMD -MP` makes the header a real dependency of
`src/mips_programs.c`, so the firmware is always rebuilt against the current program. Running
`make -C mips_assembly_test` by hand is no longer necessary.

This needs only `clang` built with the MIPS target (`clang -print-targets | grep mips`),
`llvm-objcopy` and `python3`, not a MIPS gcc. The Makefile defaults to `llvm-objcopy-14`; override it
with `make OBJCOPY=llvm-objcopy` if yours is unversioned. The generated header is committed, so
building the STM32 firmware doesn't need clang.

To add another program, write `foo.S`, run `make -C mips_assembly_test PROG=foo` (which produces
`src/mips_foo.h` with arrays `foo_instr`/`foo_data`), include that header in `src/mips_programs.c`,
and add an entry to `mips_programs[]`. To have the root `Makefile` regenerate it automatically too,
add it to the `src/mips_full_test.h:` rule's target list there.

### What `full_test` checks

109 instructions (TARGET_PC = `0x1B4`) and 13 preloaded data words.

| # | Checks | Expected `DM[32+#]` |
|---|---|---|
| 1 | `addi`, and `lw` of a word preloaded over I2C | `0x00000005` |
| 2 | negative immediate sign extension | `0xFFFFFFFD` |
| 3 | `add` | `0x00000002` |
| 4 | `sub` | `0x00000008` |
| 5 | full 32-bit `sw`/`lw` round trip | `0x12345678` |
| 6 | EX/MEM forwarding | `0x00000014` |
| 7 | MEM/WB forwarding | `0x00000006` |
| 8 | register write-before-read bypass | `0x00000008` |
| 9 | load-use stall | `0x0000000A` |
| 10 | `beq` not taken | `0x00000001` |
| 11 | `beq` taken, wrong-path instructions discarded (no delay slot) | `0x00000007` |
| 12 | backward loop summing an array (predictor + flushes + stalls) | `0x0000000F` |
| 13 | `sw`/`lw` through base register + offset | `0x00000055` |

Every check stores its computed value at `DM[32+#]` before comparing. If it doesn't match, the program
writes `DM[255] = 0xFFFFFFFF` and `DM[254] = #`, then loops forever (DONE never sets). If every check
passes it writes `DM[255] = 1` and runs into TARGET_PC.

### Running it from the probe

```
(flip SW0 up, then down)
> fpga load 0
> fpga run
> fpga wait
> fpga dump dmem 254 2        DM[255] = 0x00000001 means PASS
> fpga dump dmem 32 14        each check's value, compare with the table above
> fpga dump reg               final register values
```

| Outcome | LD0 | `fpga wait` | `DM[255]` | Look at |
|---|---|---|---|---|
| Pass | on | `done: … PC=0x000001B4` | `0x00000001` | — |
| A check failed | off | `gave up waiting` | `0xFFFFFFFF` | `DM[254]` = failing check; its `DM[32+#]` value |
| Never ran / load broken | off | `gave up waiting` | `0x00000000` | `fpga status` (PC), `fpga dump imem 0 8` vs the program |

## RTL changes for readback

Readback was added with the smallest possible change: the write path and all CPU logic are
untouched, and only a read path was added. **Six files changed** — copy all of them into the Vivado
project, then run Synthesis → Implementation → Generate Bitstream → Program Device:

| File | Change |
|---|---|
| `i2c_slave_mmio_1_2.v` | New `mmio_rdata` input. Stores the R/W bit and snapshots `mmio_rdata` on an address match. New states `S_TX`/`S_ACK_TX` shift the word out (SDA changes only on SCL falling edges). Write states unchanged. |
| `mmio_decoder_1_2.v` | New inputs from the memories and the PC, plus a combinational `mmio_rdata` mux over `mmio_addr` |
| `instruction_memory_1_2.v` | New `prog_rdata` output: the word at the existing `prog_addr` |
| `reg_file_1_2.v` | New `prog_rdata` output: `RF[prog_addr]` |
| `data_memory_1_2.v` | New `prog_rdata` output: `DM[prog_addr]` |
| `top_1_2.v` | Wires the above together, and connects `PC_out` as the readable PC |

The readback muxes (256 words each for IMEM and DMEM) add some LUTs, but they feed only the I2C
slave's snapshot register, not the CPU's own paths.

## RTL changes for block RAM

Converting the memories (see [Memories and block RAM](#memories-and-block-ram)) touched six files.
The CPU's behaviour is unchanged cycle for cycle; nothing was added to the pipeline:

| File | Change |
|---|---|
| `sram_1rw1r_1_2.v` | **New.** The 1RW+1R synchronous RAM both memories instantiate |
| `instruction_memory_1_2.v` | Byte array → one `sram_1rw1r`. Word-indexed, fetched at `PC_next[9:2]` |
| `data_memory_1_2.v` | Flop array → one `sram_1rw1r`, plus the 256-cycle reset-clear counter |
| `Pc_1_2.v` | Priority chain split into a combinational `PC_next` output and one flop |
| `M_WB_Register_1_1_2.v` | `Rd_data` field removed; the RAM's output register replaces it |
| `top_1_2.v` | Wires `PC_next` to IMEM, and `data_memory.rd_data` straight to `Write_back_mux` |

`sim/` holds an Icarus testbench that checks this end to end over a simulated I2C bus: it loads
`full_test`, runs it to DONE, and verifies `DM[255]`, all 13 per-check values, the final PC, IMEM and
DMEM readback for every loaded word, the `0xDEADBEEF` unmapped read, and that reset clears DMEM.

```bash
make -C MIPS_proc/sim        # prints "PASS <n> checks, 0 errors"
```

It passes identically on the pre-conversion and post-conversion RTL, and was checked against
deliberately injected bugs — delaying the fetch address by a cycle, and delaying the load result by
a cycle — both of which it catches.

These changes were simulated in Icarus Verilog with a testbench that drives the I2C bus the same way
the STM32 driver does. The whole suite ran with a 50 MHz and a 100 MHz system clock: 1056 checks, 0 errors.
It covered:

- reset values, and write + readback of every address
- protocol corner cases
- the STM32's real 100 kHz bus timing
- running `full_test`, with every register and data-memory write compared, in order, against an architectural model
- a failing program, DONE staying set, and the stop drain

To prove the testbench can fail, nine deliberately injected RTL bugs were tried, and every one was caught.
Simulation can't check timing closure; see below.

## Worth checking: the clock constraint

`const.xdc` constrains `clk` with a 20 ns (50 MHz) period, and its comment explains this as backing off
from 100 MHz, where timing failed (WNS −4.226 ns). But `clk` is the ZedBoard's **100 MHz oscillator**, and
`top` uses it directly, with no divider or MMCM. A clock constraint only tells Vivado's timing analysis
what frequency to check; it doesn't change the clock. So the design is analysed at 50 MHz but actually
runs at 100 MHz, the frequency it failed timing at.

If the core ever misbehaves intermittently, suspect this first. The fixes are a Clocking Wizard (MMCM)
producing a real 50 MHz clock, or a 10 ns constraint plus timing closure. The constraints file was left
unchanged.
