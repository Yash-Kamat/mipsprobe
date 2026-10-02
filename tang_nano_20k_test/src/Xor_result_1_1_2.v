`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 11:15:23
// Design Name: 
// Module Name: Xor_result
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


module Xor_result(
input [3:0] PHT_rd_data,//PHT 4 bit output 
input [3:0] PHT_rd_addr,//this is PC[5:2] 
output [3:0] BHT_rd_addr //this is the input to BHT written as rd_data
    );
    assign BHT_rd_addr=PHT_rd_data^PHT_rd_addr;//this will go as input BHT's rd-addr 
endmodule
