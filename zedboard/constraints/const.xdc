# ==============================================================================
# ZEDBOARD (xc7z020clg484-1) CONSTRAINTS FILE
# ==============================================================================
# ------------------------------------------------------------------------------
# 1. SYSTEM CLOCK (100 MHz Oscillator on Pin Y9)
# ------------------------------------------------------------------------------
set_property PACKAGE_PIN Y9 [get_ports clk]
set_property IOSTANDARD LVCMOS33 [get_ports clk]
# 100 MHz clock definition (10.00ns period, 50% duty cycle), matching the
# oscillator. There is no clock divider/MMCM in the RTL, so the design really
# runs at 100 MHz: check WNS >= 0 after implementation. If it fails, add a
# Clocking Wizard (MMCM) for a slower clock instead of loosening this period.
create_clock -period 10.000 -name sys_clk -waveform {0.000 5.000} [get_ports clk]
set_clock_uncertainty 0.200 [get_clocks sys_clk]

# ------------------------------------------------------------------------------
# 2. CONTROL INPUTS & OUTPUTS
# ------------------------------------------------------------------------------
# Reset mapped to SW0 (DIP Switch 0 on Pin F22)
set_property PACKAGE_PIN F22 [get_ports rst]
set_property IOSTANDARD LVCMOS33 [get_ports rst]
# Output LED mapped to LD0 (On-board LED 0 on Pin T22)
set_property PACKAGE_PIN T22 [get_ports led]
set_property IOSTANDARD LVCMOS33 [get_ports led]

# Both rst and led are genuinely asynchronous from a timing-analysis point of
# view: rst is a push-button/switch with no clock relationship, and led is a
# human-visible indicator with no downstream device sampling it synchronously.
# false_path is the correct treatment for both -- not input/output delay.
set_false_path -from [get_ports rst]
set_false_path -to   [get_ports led]

# If Vivado flags a CLOCK_DEDICATED_ROUTE critical warning for rst's pin
# during implementation, uncomment the line below (needed on some boards/
# pins that sit near clock-dedicated routing resources; not always required):
# set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets -quiet rst_IBUF]

# ------------------------------------------------------------------------------
# 3. I2C BUS INTERFACE (Mapped to PMOD JA Header - Pins JA1 & JA2)
# ------------------------------------------------------------------------------
# JA1 (Pin Y11) -> SCL
set_property PACKAGE_PIN Y11 [get_ports scl]
set_property IOSTANDARD LVCMOS33 [get_ports scl]
set_property PULLUP true [get_ports scl]
# JA2 (Pin AA11) -> SDA
set_property PACKAGE_PIN AA11 [get_ports sda]
set_property IOSTANDARD LVCMOS33 [get_ports sda]
set_property PULLUP true [get_ports sda]

# scl/sda are driven by the external MCU at ~100 kHz -- completely
# asynchronous to sys_clk, with no real clock relationship between them.
# i2c_slave_rx already does its own multi-stage synchronizer internally
# specifically because of this; STA input/output delay constraints would
# imply a launch/capture relationship to sys_clk that does not exist, and
# would not protect against metastability anyway (only the synchronizer
# does that). false_path is the correct treatment here, exactly like rst.
set_false_path -from [get_ports {scl sda}]
set_false_path -to   [get_ports sda]