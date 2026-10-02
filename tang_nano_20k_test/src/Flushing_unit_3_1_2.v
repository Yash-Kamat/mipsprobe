`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 14:58:49
// Design Name: 
// Module Name: Flushing_unit
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


module Flushing_unit(
input comparator ,//will be connected from ID_EX.comparator
input [31:0] IF_ID_PC_out,//will be connected from IF_ID.PC_out
input [31:0] PC_calculated ,//will be connected from output of mux which has select line zero status 
output reg IF_ID_flush,
output reg PC_flush,
output reg ID_EX_flush
    );
    always @ (*)
    begin
    if((comparator==0)&&~(IF_ID_PC_out==PC_calculated))
    begin
    {IF_ID_flush,PC_flush,ID_EX_flush}=3'b111;
    end
    else
    begin
    {IF_ID_flush,PC_flush,ID_EX_flush}=3'b000;
    end
    end
endmodule
