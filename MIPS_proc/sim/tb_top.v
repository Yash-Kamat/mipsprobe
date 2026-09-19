`timescale 1ns / 1ps
`include "program.vh"

// Regression for `top`: drives the I2C bus the way src/fpga_mips.c does,
// loads mips_assembly_test/full_test, runs it and checks every result the
// "What full_test checks" table in ../README.md documents.
//
// This exists to prove the block-RAM conversion of IMEM/DMEM is behaviourally
// identical, so it deliberately only uses the external I2C interface and never
// reaches into the DUT's internals.
module tb_top;

    localparam [6:0] SLAVE = 7'h42;

    // I2C bit timing in ns. Far slower than the slave's 3-stage synchroniser
    // (3 x 20 ns) and far faster than the real 100 kHz bus -- the slave is
    // purely edge-driven, so this only shortens the simulation.
    localparam integer TQ = 200;   // SDA setup / slave response settling
    localparam integer TH = 400;   // SCL half period

    // Memory map, from ../README.md
    localparam [15:0] A_IMEM   = 16'h0000,
                      A_DMEM   = 16'h2000,
                      A_CSR    = 16'h3000,
                      A_TARGET = 16'h3004,
                      A_PC     = 16'h3008;

    reg  clk = 1'b0;
    reg  rst = 1'b1;
    reg  scl = 1'b1;
    reg  sda_low = 1'b0;            // 1 = master pulls SDA low (open drain)
    wire sda;
    wire led;

    assign sda = sda_low ? 1'b0 : 1'bz;
    pullup (sda);

    always #10 clk = ~clk;          // 50 MHz, matching constraints/const.xdc

    top DUT (.clk(clk), .rst(rst), .scl(scl), .sda(sda), .led(led));

    integer errors = 0;
    integer checks = 0;

    task expect32(input [23*8:1] what, input [31:0] got, input [31:0] want);
    begin
        checks = checks + 1;
        if (got !== want) begin
            errors = errors + 1;
            $display("  FAIL  %0s: got %08x, want %08x", what, got, want);
        end
    end
    endtask

    // ------------------------------------------------------------------
    // I2C master
    // ------------------------------------------------------------------
    // Every task is entered and left with SCL low, except start/stop.

    task send_bit(input b);
    begin
        sda_low = ~b;   #(TQ);
        scl     = 1'b1; #(TH);
        scl     = 1'b0; #(TQ);
    end
    endtask

    task recv_bit(output b);
    begin
        sda_low = 1'b0; #(TQ);       // release so the slave can drive
        scl     = 1'b1; #(TH/2);
        b       = sda;  #(TH/2);     // sample mid-way through the high phase
        scl     = 1'b0; #(TQ);
    end
    endtask

    task i2c_start;                  // SDA falls while SCL is high
    begin
        sda_low = 1'b0; scl = 1'b1; #(TH);
        sda_low = 1'b1;             #(TH);
        scl     = 1'b0;             #(TQ);
    end
    endtask

    task i2c_rstart;                 // repeated START
    begin
        scl     = 1'b0; sda_low = 1'b0; #(TQ);
        scl     = 1'b1;                 #(TH);
        sda_low = 1'b1;                 #(TH);
        scl     = 1'b0;                 #(TQ);
    end
    endtask

    task i2c_stop;                   // SDA rises while SCL is high
    begin
        scl     = 1'b0; sda_low = 1'b1; #(TQ);
        scl     = 1'b1;                 #(TH);
        sda_low = 1'b0;                 #(TH);
    end
    endtask

    task send_byte(input [7:0] b, output ack);
        integer i;
        reg     t;
    begin
        for (i = 7; i >= 0; i = i - 1) send_bit(b[i]);
        recv_bit(t);
        ack = ~t;                    // slave ACKs by pulling SDA low
    end
    endtask

    task recv_byte(input nack, output [7:0] b);
        integer i;
        reg     t;
    begin
        b = 8'h00;
        for (i = 7; i >= 0; i = i - 1) begin
            recv_bit(t);
            b = {b[6:0], t};
        end
        send_bit(nack);              // 1 = NACK, ends the read
    end
    endtask

    // One word, MSB first -- the same transaction FPGA_MIPS_write() sends.
    task mmio_write(input [15:0] a, input [31:0] d);
        reg ack;
    begin
        i2c_start;
        send_byte({SLAVE, 1'b0}, ack);
        if (!ack) begin
            errors = errors + 1;
            $display("  FAIL  no ACK on slave address, writing %04x", a);
        end
        send_byte(a[15:8],  ack);
        send_byte(a[7:0],   ack);
        send_byte(d[31:24], ack);
        send_byte(d[23:16], ack);
        send_byte(d[15:8],  ack);
        send_byte(d[7:0],   ack);
        i2c_stop;
    end
    endtask

    // Address write, then repeated START with R/W=1 -- FPGA_MIPS_read().
    task mmio_read(input [15:0] a, output [31:0] d);
        reg       ack;
        reg [7:0] b3, b2, b1, b0;
    begin
        i2c_start;
        send_byte({SLAVE, 1'b0}, ack);
        send_byte(a[15:8], ack);
        send_byte(a[7:0],  ack);
        i2c_rstart;
        send_byte({SLAVE, 1'b1}, ack);
        recv_byte(1'b0, b3);
        recv_byte(1'b0, b2);
        recv_byte(1'b0, b1);
        recv_byte(1'b1, b0);
        i2c_stop;
        d = {b3, b2, b1, b0};
    end
    endtask

    task do_reset;
    begin
        rst = 1'b1;
        repeat (10) @(posedge clk);
        #1 rst = 1'b0;
        // Long enough for a 256-word DMEM clear plus margin. On the real board
        // reset is a switch held for many milliseconds.
        repeat (400) @(posedge clk);
    end
    endtask

    // ------------------------------------------------------------------
    // Test program and expected results
    // ------------------------------------------------------------------
    reg [31:0] instr [0:`INSTR_WORDS-1];
    reg [31:0] data  [0:`DATA_WORDS-1];
    reg [31:0] want  [1:13];         // ../README.md, "What full_test checks"

    reg [31:0] v;
    integer    i;
    integer    polls;
    reg        done_seen;

    initial begin
        $readmemh("full_test_instr.hex", instr);
        $readmemh("full_test_data.hex",  data);

        want[1]  = 32'h00000005;   // addi, and lw of an I2C-preloaded word
        want[2]  = 32'hFFFFFFFD;   // negative immediate sign extension
        want[3]  = 32'h00000002;   // add
        want[4]  = 32'h00000008;   // sub
        want[5]  = 32'h12345678;   // full 32-bit sw/lw round trip
        want[6]  = 32'h00000014;   // EX/MEM forwarding
        want[7]  = 32'h00000006;   // MEM/WB forwarding
        want[8]  = 32'h00000008;   // register write-before-read bypass
        want[9]  = 32'h0000000A;   // load-use stall
        want[10] = 32'h00000001;   // beq not taken
        want[11] = 32'h00000007;   // beq taken, wrong path discarded
        want[12] = 32'h0000000F;   // backward loop: predictor + flushes + stalls
        want[13] = 32'h00000055;   // sw/lw through base register + offset

        $display("");
        $display("tb_top: %0d instruction words, %0d data words, TARGET_PC = 0x%03x",
                 `INSTR_WORDS, `DATA_WORDS, `INSTR_WORDS * 4);

        do_reset;

        // ---- load ----
        for (i = 0; i < `INSTR_WORDS; i = i + 1)
            mmio_write(A_IMEM + i*4, instr[i]);
        for (i = 0; i < `DATA_WORDS; i = i + 1)
            mmio_write(A_DMEM + i*4, data[i]);

        // ---- readback of everything just written ----
        for (i = 0; i < `INSTR_WORDS; i = i + 1) begin
            mmio_read(A_IMEM + i*4, v);
            expect32("imem readback", v, instr[i]);
        end
        for (i = 0; i < `DATA_WORDS; i = i + 1) begin
            mmio_read(A_DMEM + i*4, v);
            expect32("dmem readback", v, data[i]);
        end
        mmio_read(16'h9000, v);
        expect32("unmapped address", v, 32'hDEADBEEF);

        // ---- run ----
        mmio_write(A_TARGET, `INSTR_WORDS * 4);
        mmio_read (A_TARGET, v);
        expect32("target_pc readback", v, `INSTR_WORDS * 4);

        mmio_write(A_CSR, 32'h0000_0001);

        done_seen = 1'b0;
        for (polls = 0; polls < 40 && !done_seen; polls = polls + 1) begin
            mmio_read(A_CSR, v);
            if (v[1]) done_seen = 1'b1;
        end
        checks = checks + 1;
        if (!done_seen) begin
            errors = errors + 1;
            $display("  FAIL  DONE never set after %0d polls (CSR = %08x)", polls, v);
        end else begin
            $display("  DONE set after %0d poll(s)", polls);
        end

        // ---- results ----
        mmio_read(A_DMEM + 255*4, v);
        expect32("DM[255] pass flag", v, 32'h00000001);
        if (v !== 32'h00000001) begin
            mmio_read(A_DMEM + 254*4, v);
            $display("        DM[254] says check %0d failed", v);
        end

        for (i = 1; i <= 13; i = i + 1) begin
            mmio_read(A_DMEM + (32+i)*4, v);
            expect32("full_test check value", v, want[i]);
        end

        mmio_read(A_PC, v);
        expect32("final PC", v, `INSTR_WORDS * 4);

        // ---- reset clears DMEM ----
        mmio_write(A_DMEM + 100*4, 32'hA5A5A5A5);
        mmio_read (A_DMEM + 100*4, v);
        expect32("dmem write sticks", v, 32'hA5A5A5A5);
        do_reset;
        mmio_read(A_DMEM + 100*4, v);
        expect32("reset clears dmem", v, 32'h00000000);

        // ---- verdict ----
        $display("");
        if (errors == 0)
            $display("PASS  %0d checks, 0 errors", checks);
        else
            $display("FAIL  %0d checks, %0d errors", checks, errors);
        $display("");

        if (errors != 0) $fatal(1);
        $finish;
    end

    // Nothing here should take anywhere near this long.
    initial begin
        #200_000_000;
        $display("");
        $display("FAIL  timeout");
        $fatal(1);
    end

endmodule
