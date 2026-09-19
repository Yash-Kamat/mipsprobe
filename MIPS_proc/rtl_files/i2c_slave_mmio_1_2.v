`timescale 1ns / 1ps

// Extended version of the original working top_i2c_v2.
// IMPORTANT: the original I2C physical/state-machine behavior is preserved:
// - 3-stage SCL/SDA synchronization
// - START/STOP detection
// - sample received bits on SCL rising edge
// - assert ACK on the falling edge after the 8th bit
// - release ACK on the following SCL falling edge
// - open-drain SDA (slave only pulls LOW)
//
// Only the payload handling is extended so one transaction carries:
//   [7-bit slave address + W] [ACK]
//   [addr15:8] [ACK] [addr7:0] [ACK]
//   [data31:24] [ACK] [data23:16] [ACK]
//   [data15:8] [ACK] [data7:0] [ACK] STOP
//
// Uses slave address 0x42. After the final data byte is received, mmio_wr is
// a one-system-clock pulse.
//
// Readback (added): write only the two address bytes, then a repeated START
// with R/W=1. The slave snapshots mmio_rdata when it ACKs the read address and
// shifts the 32-bit word out MSB first, changing SDA only on SCL falling edges.
//   [0x42 + W] [ACK] [addr15:8] [ACK] [addr7:0] [ACK]
//   Sr [0x42 + R] [ACK] [data31:24] [ACK] ... [data7:0] [NACK] STOP

module i2c_slave_mmio #(
    parameter [6:0] SLAVE_ADDR = 7'h42
)(
    input  wire        clk,
    input  wire        rst,
    input  wire        scl,
    inout  wire        sda,
    output reg         mmio_wr,
    output reg [15:0]  mmio_addr,
    output reg [31:0]  mmio_wdata,
    input  wire [31:0] mmio_rdata   // readback value for mmio_addr (mmio_decoder)
);

    // ---------------------------------------------------------------
    // Same open-drain SDA behavior as original
    // ---------------------------------------------------------------
    reg sda_oe;
    assign sda = sda_oe ? 1'b0 : 1'bz;

    // ---------------------------------------------------------------
    // Same 3-stage synchronizers as original
    // ---------------------------------------------------------------
    reg [2:0] scl_s;
    reg [2:0] sda_s;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            scl_s <= 3'b111;
            sda_s <= 3'b111;
        end else begin
            scl_s <= {scl_s[1:0], scl};
            sda_s <= {sda_s[1:0], sda};
        end
    end

    wire scl_now    = scl_s[1];
    wire scl_prev   = scl_s[2];
    wire sda_now    = sda_s[1];
    wire sda_prev   = sda_s[2];

    wire scl_rising  = (scl_prev == 1'b0) && (scl_now == 1'b1);
    wire scl_falling = (scl_prev == 1'b1) && (scl_now == 1'b0);

    wire start_cond = (sda_prev == 1'b1) && (sda_now == 1'b0) && scl_now;
    wire stop_cond  = (sda_prev == 1'b0) && (sda_now == 1'b1) && scl_now;

    // ---------------------------------------------------------------
    // Same basic state machine as original, with byte_index added
    // ---------------------------------------------------------------
    localparam S_IDLE     = 3'd0,
               S_ADDR     = 3'd1,
               S_ACK_ADDR = 3'd2,
               S_DATA     = 3'd3,
               S_ACK_DATA = 3'd4,
               S_TX       = 3'd5,   // readback: shifting data bits out
               S_ACK_TX   = 3'd6;   // readback: master's ACK/NACK

    reg [2:0] state;
    reg [3:0] bit_cnt;
    reg [7:0] shifter;
    reg [2:0] byte_index;
    reg       addr_ok;
    reg       rw;        // R/W bit of the current transaction (1 = read)
    reg [31:0] tx_word;  // word being shifted out during a read

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state      <= S_IDLE;
            bit_cnt    <= 4'd0;
            shifter    <= 8'd0;
            byte_index <= 3'd0;
            addr_ok    <= 1'b0;
            sda_oe     <= 1'b0;
            mmio_wr    <= 1'b0;
            mmio_addr  <= 16'd0;
            mmio_wdata <= 32'd0;
            rw         <= 1'b0;
            tx_word    <= 32'd0;
        end else begin
            // One-clock MMIO write pulse.
            mmio_wr <= 1'b0;

            // START/STOP retain priority from the original.
            if (stop_cond) begin
                state      <= S_IDLE;
                bit_cnt    <= 4'd0;
                byte_index <= 3'd0;
                addr_ok    <= 1'b0;
                sda_oe     <= 1'b0;
            end else if (start_cond) begin
                state      <= S_ADDR;
                bit_cnt    <= 4'd0;
                shifter    <= 8'd0;
                byte_index <= 3'd0;
                addr_ok    <= 1'b0;
                sda_oe     <= 1'b0;
            end else begin
                case (state)

                    S_IDLE: begin
                        sda_oe <= 1'b0;
                    end

                    // Exactly the original address receive behavior:
                    // sample 8 bits on rising edges, ACK on following fall.
                    S_ADDR: begin
                        if (scl_rising) begin
                            shifter <= {shifter[6:0], sda_now};
                            bit_cnt <= bit_cnt + 1'b1;
                        end

                        if (scl_falling && bit_cnt == 4'd8) begin
                            if (shifter[7:1] == SLAVE_ADDR) begin
                                sda_oe  <= 1'b1;
                                addr_ok <= 1'b1;
                                state   <= S_ACK_ADDR;
                                rw      <= shifter[0];
                                tx_word <= mmio_rdata;  // snapshot, used only by a read
                            end else begin
                                sda_oe  <= 1'b0;
                                addr_ok <= 1'b0;
                                state   <= S_IDLE;
                            end
                            bit_cnt <= 4'd0;
                        end
                    end

                    // ACK is held low through the ACK clock and released
                    // on SCL falling edge, exactly as in original.
                    S_ACK_ADDR: begin
                        if (scl_falling) begin
                            sda_oe     <= 1'b0;
                            shifter    <= 8'd0;
                            bit_cnt    <= 4'd0;
                            byte_index <= 3'd0;
                            state      <= S_DATA;

                            // Read: put the word's MSB on SDA instead.
                            if (rw) begin
                                sda_oe <= ~tx_word[31];
                                state  <= S_TX;
                            end
                        end
                    end

                    // Receive one payload byte. The first two are address,
                    // next four are data.
                    S_DATA: begin
                        if (scl_rising) begin
                            shifter <= {shifter[6:0], sda_now};
                            bit_cnt <= bit_cnt + 1'b1;
                        end

                        if (scl_falling && bit_cnt == 4'd8) begin
                            case (byte_index)
                                3'd0: mmio_addr[15:8]   <= shifter;
                                3'd1: mmio_addr[7:0]    <= shifter;
                                3'd2: mmio_wdata[31:24] <= shifter;
                                3'd3: mmio_wdata[23:16] <= shifter;
                                3'd4: mmio_wdata[15:8]  <= shifter;
                                3'd5: begin
                                    mmio_wdata[7:0] <= shifter;
                                    mmio_wr <= 1'b1;
                                end
                                default: begin end
                            endcase

                            sda_oe  <= 1'b1;
                            bit_cnt <= 4'd0;
                            state   <= S_ACK_DATA;
                        end
                    end

                    // Release ACK on falling edge. After final byte,
                    // return to IDLE and wait for STOP.
                    S_ACK_DATA: begin
                        if (scl_falling) begin
                            sda_oe  <= 1'b0;
                            shifter <= 8'd0;
                            bit_cnt <= 4'd0;

                            if (byte_index == 3'd5) begin
                                byte_index <= 3'd0;
                                state      <= S_IDLE;
                            end else begin
                                byte_index <= byte_index + 1'b1;
                                state      <= S_DATA;
                            end
                        end
                    end

                    // Read: the bit on SDA was sampled by the master on the
                    // rising edge; on the falling edge put the next one out.
                    // After 8 bits, release SDA for the master's ACK/NACK.
                    S_TX: begin
                        if (scl_falling) begin
                            tx_word <= {tx_word[30:0], 1'b1};
                            if (bit_cnt == 4'd7) begin
                                sda_oe  <= 1'b0;
                                bit_cnt <= 4'd0;
                                state   <= S_ACK_TX;
                            end else begin
                                sda_oe  <= ~tx_word[30];
                                bit_cnt <= bit_cnt + 1'b1;
                            end
                        end
                    end

                    // NACK ends the read. ACK sends the next byte; after the
                    // 4th byte the slave stays released (extra bytes read 0xFF).
                    S_ACK_TX: begin
                        if (scl_rising && sda_now) begin
                            state <= S_IDLE;
                        end else if (scl_falling) begin
                            if (byte_index == 3'd3) begin
                                byte_index <= 3'd0;
                                state      <= S_IDLE;
                            end else begin
                                byte_index <= byte_index + 1'b1;
                                sda_oe     <= ~tx_word[31];
                                state      <= S_TX;
                            end
                        end
                    end

                    default: begin
                        state      <= S_IDLE;
                        bit_cnt    <= 4'd0;
                        byte_index <= 3'd0;
                        sda_oe     <= 1'b0;
                    end
                endcase
            end
        end
    end
endmodule
