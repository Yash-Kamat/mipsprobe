`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Module Name: zed_top
//
// ZedBoard top level. Replaces the Tiny Tapeout wrapper (tt_um_mips_i2c.v);
// port names match constraints/const.xdc. Pure pin plumbing: the core's
// split SDA (sda_in / sda_out / sda_oe) becomes one open-drain inout pin.
//
//   clk -> Y9 (100 MHz)   rst -> SW0 (active high)   led -> LD0
//   scl -> JA1            sda -> JA2 (open-drain, internal pull-up)
//////////////////////////////////////////////////////////////////////////////////

module zed_top (
    input  clk,
    input  rst,
    input  scl,
    inout  sda,
    output led
);
    wire sda_out;
    wire sda_oe;

    top top_inst (
        .clk(clk),
        .rst(rst),
        .scl(scl),
        .sda_in(sda),
        .sda_out(sda_out),
        .sda_oe(sda_oe),
        .led(led)
    );

    assign sda = sda_oe ? sda_out : 1'bz;   // sda_out is always 0: open-drain

endmodule
