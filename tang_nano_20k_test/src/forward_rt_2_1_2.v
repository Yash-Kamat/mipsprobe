`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 11:57:47
// Design Name: 
// Module Name: forward_rt
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


module forward_rt(
input [1:0] forward_rt, //this will come from forwarding unit 
input [31:0] D_2,
input [31:0] Mem_WB_write_data, //this will come from result of the mux of mem to reg in write back stage that is conncted to the write data of reg file 
input [31:0] EX_MEM_R, // this is the output of alu from execution flowing in memory stage  
 output reg [31:0] rt_f//this is input to next mux in which this or immediate value is selcted for operand 2 
    );
    always @ (*)
    begin
    case (forward_rt)
    2'b10:rt_f=EX_MEM_R;
    2'b11:rt_f=Mem_WB_write_data;
    2'b00:rt_f=D_2;
    default:rt_f={32{1'b0}};
    endcase
    end 
endmodule
