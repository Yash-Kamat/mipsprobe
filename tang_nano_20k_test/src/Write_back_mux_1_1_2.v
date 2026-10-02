`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 23.07.2026 18:54:51
// Design Name: 
// Module Name: Write_back_mux
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


module Write_back_mux(
input [31:0] MEM_WB_Rd_data,
input [31:0] MEM_WB_R,
input Mem_to_Reg,//connected to Mem/WB.WB[0]
output [31:0] write_data
    );
 assign write_data = Mem_to_Reg? MEM_WB_Rd_data:MEM_WB_R;  
    
endmodule
