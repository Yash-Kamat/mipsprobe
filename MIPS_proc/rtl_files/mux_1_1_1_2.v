`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 16:48:35
// Design Name: 
// Module Name: mux_1
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


module mux_1(
input [31:0] PC_out,//this will come from PC modules output
input  [31:0] BTB_rd_data ,//this will come from output of btb or BTB.rd_data
input BHT_rd_data,//this will come from bht or BHT.read _data
output [31:0] PC_prediction_if_branch//now this is the input to 2nd mux
    );
    assign PC_prediction_if_branch = BHT_rd_data?BTB_rd_data:(PC_out+4);
endmodule
