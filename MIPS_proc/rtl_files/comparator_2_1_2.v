`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 16:24:16
// Design Name: 
// Module Name: comparator
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


module comparator(
input [31:0] instruction_code,//this will come from output of instruction memory 
output comparator//now this is the input to 2nd mux it is the select line 
    );
    assign comparator = (instruction_code[31:26]==6'b000101)?0:1;
endmodule
