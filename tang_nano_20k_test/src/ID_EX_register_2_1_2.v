`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 23:07:49
// Design Name: 
// Module Name: ID_EX_register
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


module ID_EX_register(
input rst,
input clk,
input ID_EX_flush, //will be connected to the output of flushing unit 
input ID_EX_stall,//will be connected to the output of stalling unit 
input freeze,
input [3:0] EX,  // will come from control signal output ex  
input [1:0] M, //will come from control signal output m
input [1:0] WB,//will be connected from control signal output wb
input [4:0] rs,//will be connected from IF/ID.IC[25:21]
input [4:0] rt,//will be connected from IF/ID.IC[20:16]
input [31:0] D_1,//will be connected from reg file output d_1
input [31:0] D_2,//will be connected from reg file output d_2
input  comparator,//will be connected from IF/ID.comparator
input  [4:0] rd_maybe,//will be connected from IF/ID.IC[15:11]
input  [31:0] immediate,//will be connected from sign extendor output imemdiate 
input  [31:0] PC_out,//will be connected from IF/ID.PC_out
input  [3:0] BHT_rd_addr,//will be connected from IF/ID.BHT_rd_addr
output reg [3:0] ID_EX_EX, 
output reg [1:0] ID_EX_M,
output reg [1:0] ID_EX_WB,
output reg [4:0] ID_EX_rs,
output reg [4:0] ID_EX_rt,
output reg [31:0] ID_EX_D_1,
output reg [31:0] ID_EX_D_2,
output reg  ID_EX_comparator,
output reg  [4:0] ID_EX_rd_maybe,
output reg  [31:0] ID_EX_immediate,
output reg  [31:0] ID_EX_PC_out,
output reg  [3:0] ID_EX_BHT_rd_addr
);
always @ (posedge clk or posedge rst)
begin
if(rst)
begin
 ID_EX_EX<=4'b0000; 
 ID_EX_M<=2'b00;
 ID_EX_WB<=2'b00;
 ID_EX_rs<=5'b00000;
 ID_EX_rt<=5'b00000;
 ID_EX_D_1<={32{1'b0}};
 ID_EX_D_2<={32{1'b0}};
 ID_EX_comparator<=1'b1;
 ID_EX_rd_maybe<=5'b00000;
  ID_EX_immediate<={32{1'b0}};
  ID_EX_PC_out<={32{1'b0}};
 ID_EX_BHT_rd_addr<=4'b0000;
end 
else if (freeze)
begin
 // Hold the entire pipeline register during LOAD/HALT.
 ID_EX_EX<=ID_EX_EX;
 ID_EX_M<=ID_EX_M;
 ID_EX_WB<=ID_EX_WB;
 ID_EX_rs<=ID_EX_rs;
 ID_EX_rt<=ID_EX_rt;
 ID_EX_D_1<=ID_EX_D_1;
 ID_EX_D_2<=ID_EX_D_2;
 ID_EX_comparator<=ID_EX_comparator;
 ID_EX_rd_maybe<=ID_EX_rd_maybe;
 ID_EX_immediate<=ID_EX_immediate;
 ID_EX_PC_out<=ID_EX_PC_out;
 ID_EX_BHT_rd_addr<=ID_EX_BHT_rd_addr;
end
else if (ID_EX_flush)
begin
 ID_EX_EX<=4'b0000; 
 ID_EX_M<=2'b00;
 ID_EX_WB<=2'b00;
 ID_EX_rs<=5'b00000;
 ID_EX_rt<=5'b00000;
 ID_EX_D_1<={32{1'b0}};
 ID_EX_D_2<={32{1'b0}};
 ID_EX_comparator<=1'b1;
 ID_EX_rd_maybe<=5'b00000;
  ID_EX_immediate<={32{1'b0}};
  ID_EX_PC_out<={32{1'b0}};
 ID_EX_BHT_rd_addr<=4'b0000;
end 
else if (ID_EX_stall)
begin
 ID_EX_EX<=4'b0000; 
 ID_EX_M<=2'b00;
 ID_EX_WB<=2'b00;
 ID_EX_rs<=5'b00000;
 ID_EX_rt<=5'b00000;
 ID_EX_D_1<={32{1'b0}};
 ID_EX_D_2<={32{1'b0}};
 ID_EX_comparator<=1'b1;
 ID_EX_rd_maybe<=5'b00000;
  ID_EX_immediate<={32{1'b0}};
  ID_EX_PC_out<={32{1'b0}};
 ID_EX_BHT_rd_addr<=4'b0000;
end
else 
begin
 ID_EX_EX<=EX ;
 ID_EX_M<=M;
 ID_EX_WB<=WB;
 ID_EX_rs<=rs;
 ID_EX_rt<=rt;
 ID_EX_D_1<=D_1;
 ID_EX_D_2<=D_2;
 ID_EX_comparator<=comparator;
 ID_EX_rd_maybe<=rd_maybe;
  ID_EX_immediate<=immediate;
  ID_EX_PC_out<=PC_out;
 ID_EX_BHT_rd_addr<=BHT_rd_addr;
 end
end 
endmodule
