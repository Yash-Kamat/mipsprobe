`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 23:04:41
// Design Name: 
// Module Name: Sign_extender
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


module Sign_extender(
input [15:0] imm, // will be connected to IF/ID.IC[15:0]
output [31:0] immediate );
assign immediate = {{16{imm[15]}},imm[15:0]};
endmodule
