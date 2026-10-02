`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 16:53:29
// Design Name: 
// Module Name: mux_2
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


module mux_2(
input rst,
input [31:0] PC_out,//this will come from PC module output 
input [31:0] PC_prediction_if_branch,//this comes fom mux 1 output 
input comparator,//this comes from comparator module output 
output [31:0] PC_in//this is then input of module PC 
    );
    assign PC_in = rst?{32{1'b0}}:(comparator==0)?PC_prediction_if_branch:PC_out+4;
endmodule
