`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 17:39:49
// Design Name: 
// Module Name: branching_unit
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


module branching_unit(
input comparator ,//will be connected from ID_EX.comparator
input [31:0] IF_ID_PC_out,//will be connected from IF_ID.PC_out
input [31:0] PC_calculated ,//will be connected from output of mux which has select line zero status 
output reg PHT_write,
output reg BHT_write,
output reg BTB_write
    );
    always @ (*)
    begin
    {PHT_write,BHT_write,BTB_write}=3'b000;
    if(~comparator)
    begin
    PHT_write=1'b1;
    end
    if((comparator==0)&&~(IF_ID_PC_out==PC_calculated))
    begin
    {PHT_write,BHT_write,BTB_write}=3'b111;
    end
    end
endmodule
