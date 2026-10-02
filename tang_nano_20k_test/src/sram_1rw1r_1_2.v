`timescale 1ns / 1ps

// One synchronous RAM, shared by the instruction and the data memory.
//
// Shape: one read/write port plus one read-only port (1RW + 1R). That is what a
// Xilinx true-dual-port block RAM provides, and also what the open-source ASIC
// SRAM macros provide, so the same wrapper covers both.
//
// Reads are SYNCHRONOUS: data appears on the clock edge AFTER the address is
// presented. There is deliberately no reset and no asynchronous read here --
// either one stops Vivado inferring a block RAM, and neither exists on an ASIC
// SRAM macro.
//
// To build this from the IP catalog instead, or to swap it for an SRAM macro,
// see "Memories and block RAM" in ../README.md.
module sram_1rw1r #(
    parameter AW = 8                        // 2**AW words of 32 bits
)(
    input             clk,
    // Port 0: read/write. A read and a write cannot happen in the same cycle.
    input             en0,
    input             we0,
    input  [AW-1:0]   a0,
    input  [31:0]     d0,
    output reg [31:0] q0,
    // Port 1: read only.
    input             en1,
    input  [AW-1:0]   a1,
    output reg [31:0] q1
);
    (* ram_style = "block" *) reg [31:0] mem [0:(1<<AW)-1];

    always @(posedge clk) begin
        if (en0) begin
            if (we0) mem[a0] <= d0;
            else     q0      <= mem[a0];
        end
        if (en1) q1 <= mem[a1];
    end
endmodule
