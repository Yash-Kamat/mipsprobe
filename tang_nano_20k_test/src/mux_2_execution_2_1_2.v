`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 12:18:35
// Design Name: 
// Module Name: mux_2_execution
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


module mux_2_execution(
input zero,//will be connected from ALU zero 
input [31:0] immediate,//will be connected from ID/EX.immediate 
input [31:0] PC_out,//will be connected to ID/EX.PC_out
output [31:0] PC_calculated    );//will be input to flushing unit branching unit 
wire [31:0] PC_if_branch;
assign PC_if_branch = (PC_out+4)+(immediate<<2);
assign PC_calculated = zero?PC_if_branch:PC_out+4;
endmodule
