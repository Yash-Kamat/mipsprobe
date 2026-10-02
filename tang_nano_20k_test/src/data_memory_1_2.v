`timescale 1ns / 1ps

// 256 x 32-bit data memory, as one synchronous 1RW+1R RAM.
// CPU addressing stays WORD-indexed: LW/SW address 7 accesses DM[7].
//
// The read is now synchronous, so rd_data is the value for the instruction in
// WB, not the one in MEM. That is exactly where M_WB_Register used to hold it,
// so this RAM's output register replaces that pipeline register and the pipeline
// keeps its original timing -- M_WB_Register no longer carries Rd_data at all.
// See ../README.md.
module data_memory(
    input        clk,
    input        rst,
    input        Mem_rd,
    input        Mem_write,
    input [31:0] rd_addr,
    input [31:0] write_data,
    output [31:0] rd_data,           // WB-stage value
    input        prog_we,
    input [7:0]  prog_addr,
    input [31:0] prog_wdata,
    output [31:0] prog_rdata
);
    // A block RAM cannot be bulk-cleared, so reset instead starts a counter that
    // walks all 256 words writing zero, one per clock. It finishes ~5 us after
    // reset release at 50 MHz -- long before the MCU can set RUN over a 100 kHz
    // bus. An ASIC SRAM needs this regardless, since it powers up undefined.
    reg [8:0] clear_cnt;
    wire      clearing = !clear_cnt[8];

    always @(posedge clk or posedge rst) begin
        if (rst)           clear_cnt <= 9'd0;
        else if (clearing) clear_cnt <= clear_cnt + 1'b1;
    end

    // Port 0 priority: the clear walk, then I2C writes, then the CPU. The CPU
    // never reads and writes in the same cycle, and both the clear walk and I2C
    // programming only run with the pipeline frozen -- where the CPU's access is
    // either idle or a repeat of a write it already did -- so nothing is lost.
    // Loading while the core runs was never supported and still isn't.
    wire        cpu_en = Mem_rd | Mem_write;
    wire        en0 = clearing | prog_we | cpu_en;
    wire        we0 = clearing | prog_we | Mem_write;
    wire [7:0]  a0  = clearing ? clear_cnt[7:0] : prog_we ? prog_addr  : rd_addr[7:0];
    wire [31:0] d0  = clearing ? 32'd0          : prog_we ? prog_wdata : write_data;

    sram_1rw1r #(.AW(8)) RAM (
        .clk(clk),
        .en0(en0),  .we0(we0), .a0(a0),        .d0(d0), .q0(rd_data),
        .en1(1'b1),            .a1(prog_addr),          .q1(prog_rdata)
    );
endmodule
