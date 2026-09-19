`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 12:03:43
// Design Name: 
// Module Name: mux_1_execution
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


module mux_1_execution(
input [31:0] rt_f, //will come from forward rt
input [31:0] ID_EX_immediate,//this will come from output of sign extendor and flow to execution 
input Alu_src, // this will be connected to ID_EX_EX[0] 
output [31:0] operand_2  );
   assign operand_2 = Alu_src?ID_EX_immediate:rt_f;
endmodule
