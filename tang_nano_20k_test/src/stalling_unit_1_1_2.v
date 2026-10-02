`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 14:11:11
// Design Name: 
// Module Name: stalling_unit
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


module stalling_unit(
input [31:0] IF_ID_IC,//will be connected to IF_ID_instruction_code[31:26]
input rst,
input ID_EX_MEM_Rd, //this will be connected to ID_EX_M[1]
input [4:0] ID_EX_rt, //connected from pipeline register
input [4:0] IF_ID_rs, // conncted from input of ID/EX reg
input [4:0] IF_ID_rt,//connected from input of ID/EX reg 
output reg  IF_ID_stall, //will be connected to IF/ID pipeline register
output reg  ID_EX_stall,//will be conneected to ID/EX register 
output reg  PC_stall   );//will be connected to PC module 
always @ (*)
begin
if(rst)
begin
{IF_ID_stall,ID_EX_stall,PC_stall}={1'b0,1'b0,1'b0};
end
else if (
    ID_EX_MEM_Rd &&
    (
        (IF_ID_rs == ID_EX_rt)
        ||
        (
            (IF_ID_rt == ID_EX_rt) &&
            (IF_ID_IC[31:26] != 6'b000010) &&
            (IF_ID_IC[31:26] != 6'b000011)
        )
    )
) //last condition so it doesnt stall for lw in execution and addi or lw in decode stage with same rt  
begin
{IF_ID_stall,ID_EX_stall,PC_stall}={1'b1,1'b1,1'b1};
end
else
begin
{IF_ID_stall,ID_EX_stall,PC_stall}={1'b0,1'b0,1'b0};
end
end
endmodule
