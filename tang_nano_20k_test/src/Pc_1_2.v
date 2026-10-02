`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 10:25:26
// Design Name: 
// Module Name: Pc
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module Pc(
input clk,
input PC_flush,//this connection comes from output of flushing unit 
input PC_stall,//this connection comes from output of stalling unit 
input [31:0] PC_in,//this connection from output of MUX2 in instruction fetch stage 
input [31:0] PC_calculated,//this comes from execution unit 
input        hold,
input        rst,
output [31:0] PC_next,//the value PC_out takes at the next edge; addresses the instruction memory
output reg [31:0] PC_out
    );
    // Same priority order as before, just split out so the instruction memory
    // can be given the address one cycle early. A synchronous RAM needs that;
    // see instruction_memory_1_2.v.
    assign PC_next = rst       ? 32'h00000000 :  //reset wins
                     hold      ? PC_out       :  //frozen or draining
                     PC_flush  ? PC_calculated:  //utmost priority to flush, giving the correct PC
                     PC_stall  ? PC_out       :  //retaining the same instruction
                                 PC_in;          //our prediction

    always @ (posedge clk or posedge rst) //making it sequential
    begin
    if(rst)
        PC_out <= 32'h00000000;
    else
        PC_out <= PC_next;
    end
endmodule
