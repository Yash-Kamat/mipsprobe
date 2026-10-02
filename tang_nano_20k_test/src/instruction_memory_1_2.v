`timescale 1ns / 1ps

// 256 x 32-bit instruction memory, as one synchronous 1RW+1R RAM.
//
// Word indexed by PC[9:2]. The PC is always a multiple of 4 -- it resets to 0,
// and the only next-PC sources are PC+4 and PC+4+(imm<<2), this ISA having no
// jumps -- so nothing is lost versus the byte array this replaces. The
// big-endian word layout the I2C protocol uses is unchanged: storing a word
// directly holds exactly what the four byte reads used to reassemble.
//
// The fetch port is addressed with PC_next, NOT PC_out. The RAM needs its
// address one cycle before the data, and PC_next is the value PC_out takes at
// the next clock edge, so instruction_code lines up with PC_out exactly as it
// did when the read was combinational. That is what keeps the RAM's one-cycle
// latency out of the pipeline entirely; see ../README.md.
module instruction_memory(
    input        clk,
    input [31:0] PC_next,            // next-cycle PC, straight out of Pc
    output [31:0] instruction_code,
    input        prog_we,
    input [7:0]  prog_addr,
    input [31:0] prog_wdata,
    output [31:0] prog_rdata
);
    // Port 0 does the I2C writes and readback, port 1 does the CPU fetch.
    // Dropping the fetch enable during a programming write keeps both ports off
    // the same address in the same cycle, which a real block RAM reports as a
    // collision. Programming only happens with the pipeline frozen, so the one
    // stale fetch that costs is never used.
    sram_1rw1r #(.AW(8)) RAM (
        .clk(clk),
        .en0(1'b1),     .we0(prog_we), .a0(prog_addr),     .d0(prog_wdata), .q0(prog_rdata),
        .en1(!prog_we),                .a1(PC_next[9:2]),                   .q1(instruction_code)
    );
endmodule
