`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 17:48:31
// Design Name: 
// Module Name: mux_3_execution
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


module mux_3_execution(
input [4:0] ID_EX_rd_maybe,//connected to basically ID_EX.IC[15:11]
input [4:0] ID_EX_rt,//connected to basically ID/EX.rt
input Reg_dst,//connected to ID/EX.EX[3]
output [4:0] rd
 );
 assign rd = Reg_dst?ID_EX_rt:ID_EX_rd_maybe;
endmodule
