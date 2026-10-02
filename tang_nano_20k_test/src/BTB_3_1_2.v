`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 13:42:33
// Design Name: 
// Module Name: BTB
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


module BTB(
input clk,
input [3:0] rd_addr,//this is connected to PC_out[5:2]
input BTB_write_control,//this is connected output of brnaching unit 
input [3:0] ID_EX_PHT_wr_addr,//this is connected to ID/ExPC_out[5:2]
input [31:0] BTB_write_data,//this is connected to PC_calculated in execution stage 
input rst,
output [31:0] BTB_rd_data // used as input in MUX-1 module 
    );
    integer i;
    reg [31:0] BTB [15:0];
    assign BTB_rd_data=BTB[rd_addr];
    always @ (posedge clk or posedge rst)
    begin
    if(rst)
    begin
    for(i=0;i<=15;i=i+1)
    BTB[i]<={32{1'b0}};
    end
    else if (BTB_write_control)
    begin
    BTB[ID_EX_PHT_wr_addr]<=BTB_write_data;
    end
    end
endmodule
