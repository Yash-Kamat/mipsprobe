`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 22.07.2026 21:01:44
// Design Name: 
// Module Name: control_unit
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


module control_unit(
input rst,
input [5:0] opcode,//comes from IF/ID.instruction_code[31:26]
output reg [3:0] EX,
output reg [1:0] M,
output reg [1:0] WB
    );
    always @ (*)
    begin
    if(rst)
    begin
    {EX,M,WB}={4'b0000,2'b00,2'b00};//{EX,MEM,WB}={{Regdst,ALUop,ALUsrc},{Memrd,Memwrite},{Regwrite,MemtoReg}} reg dst 0 means allowing ID/Ex.IC[15:11] and reg dst 1 means allowing ID/EX.rt  
    end                             // ALUsrc is 0 meaning allowing forwarded value for rt register as second operand and if 1 then allowing sign extended immediate value or IC[15:0]
    else                            // Memrd or memwrite is 1 means allowing to read and write respectively else not allowing 
    begin                            //reg write is 1 only then reg file is updated when mem to reg is 0 then Mem/wB alu result goes as write data in reg file as Mem/WB.data memory output goes 
    case(opcode)
    6'b000000:{EX,M,WB}={4'b0000,2'b00,2'b10};//alu op 00 for add this for normal add 
    6'b000001:{EX,M,WB}={4'b0100,2'b00,2'b10};//alu op 01 for sub  this for normal subtract
    6'b000010:{EX,M,WB}={4'b1001,2'b00,2'b10};//alu op 00 for add this is for addi 
    6'b000011:{EX,M,WB}={4'b1001,2'b10,2'b11};//alu op 00 for add  this is for lw 
    6'b000100:{EX,M,WB}={4'b1001,2'b01,2'b01};//alu op 00 for add   this is for sw 
    6'b000101:{EX,M,WB}={4'b0100,2'b00,2'b00};//alu op 01 for sub   this is for beq 
    default:{EX,M,WB}={4'b0000,2'b00,2'b00};
    endcase
    end
    end 
endmodule
