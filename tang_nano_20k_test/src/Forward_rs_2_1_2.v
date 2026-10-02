`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 11:39:33
// Design Name: 
// Module Name: Forward_rs
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


module Forward_rs(
input [1:0] forward_rs, //this will come from forwarding unit 
input [31:0] D_1,
input [31:0] Mem_WB_write_data, //this will come from result of the mux of mem to reg in write back stage that is conncted to the write data of reg file 
input [31:0] EX_MEM_R, // this is the output of alu from execution flowing in memory stage  
 output reg [31:0] operand_1);//this is input to alu 
 always @ (*)
 begin
 case (forward_rs)
 2'b10:operand_1=EX_MEM_R;
 2'b11:operand_1=Mem_WB_write_data;
 2'b00:operand_1=D_1;
 default:operand_1={32{1'b0}};
 endcase
 end
endmodule
