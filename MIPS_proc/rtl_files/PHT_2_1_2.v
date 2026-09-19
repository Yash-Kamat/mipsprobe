`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 10:30:47
// Design Name: 
// Module Name: PHT
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


module PHT(
input clk,
input [3:0] rd_addr,//this will be connected to PC_out[5:2]
input PHT_write_control,//this is connecetd to output of branching unit 
input [3:0] ID_EX_PHT_wr_addr,//this is connected to the PC[5:2] that is in execution unit flowed from instruction fetch stage or in simple terms ID/EX.PC[5:2]
input PHT_write_data,//this is connected to the zero status of the ALU from execution unit 
input rst,
output [3:0] PHT_rd_data//used as input in Xor module 
    );
    integer i;
    reg [3:0] PHT [15:0] ;//first size then depth 
    assign PHT_rd_data = PHT[rd_addr];
    always @ (posedge clk or posedge rst)//async reset 
    begin
    if(rst)
    begin
    for(i=0;i<=15;i=i+1)
    PHT[i]<=4'b0000;
    end
    else if(PHT_write_control)
    begin
    PHT[ID_EX_PHT_wr_addr]<={PHT[ID_EX_PHT_wr_addr][2],PHT[ID_EX_PHT_wr_addr][1],PHT[ID_EX_PHT_wr_addr][0],PHT_write_data};
    end
    end
endmodule
