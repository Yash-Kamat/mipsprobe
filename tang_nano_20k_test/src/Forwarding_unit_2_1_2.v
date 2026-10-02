`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 13:09:35
// Design Name: 
// Module Name: Forwarding_unit
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


module Forwarding_unit(
input [4:0] ID_EX_rs, // will be connected from rs flown from decode to execute state
input [4:0] ID_EX_rt,//will be connected from rt of decode to execution 
input [4:0] EX_MEM_rd , //will be connected from result of mux of mux_3_execution that is between rd_maybe and rt in memory stage 
input [4:0] MEM_WB_rd ,//will be connected from result of the mux of mux_3 in write back stage 
input EX_MEM_Reg_write,//will be connected to EX/mem.write_back[1] 
input MEM_WB_Reg_write,//will be connected to mem/wb.write_back[1]
output reg [1:0] forward_rs,
output reg [1:0] forward_rt); 
always @ (*)
begin
forward_rs=2'b00;
forward_rt=2'b00;
if(EX_MEM_Reg_write)
begin 
if(EX_MEM_rd==ID_EX_rs)
begin
forward_rs=2'b10; 
end
if(EX_MEM_rd==ID_EX_rt)
begin
forward_rt=2'b10; 
end 
end
if (MEM_WB_Reg_write)
begin 
if((MEM_WB_rd==ID_EX_rs)&&(~(EX_MEM_rd==ID_EX_rs)||~EX_MEM_Reg_write))
begin
forward_rs=2'b11;
end 
if(MEM_WB_rd==ID_EX_rt  &&(~(EX_MEM_rd==ID_EX_rt)||~EX_MEM_Reg_write))
begin
forward_rt=2'b11;
end
end 
end
endmodule
