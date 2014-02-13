-------------------------------------------------------------------------------
-- DISCLAIMER OF LIABILITY
-- 
-- This text/file contains proprietary, confidential
-- information of Xilinx, Inc., is distributed under license
-- from Xilinx, Inc., and may be used, copied and/or
-- disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc. Xilinx hereby grants you a 
-- license to use this text/file solely for design, simulation, 
-- implementation and creation of design files limited 
-- to Xilinx devices or technologies. Use with non-Xilinx 
-- devices or technologies is expressly prohibited and 
-- immediately terminates your license unless covered by
-- a separate agreement.
--
-- Xilinx is providing this design, code, or information 
-- "as-is" solely for use in developing programs and 
-- solutions for Xilinx devices, with no obligation on the 
-- part of Xilinx to provide support. By providing this design, 
-- code, or information as one possible implementation of 
-- this feature, application or standard, Xilinx is making no 
-- representation that this implementation is free from any 
-- claims of infringement. You are responsible for 
-- obtaining any rights you may require for your implementation. 
-- Xilinx expressly disclaims any warranty whatsoever with 
-- respect to the adequacy of the implementation, including 
-- but not limited to any warranties or representations that this
-- implementation is free from claims of infringement, implied 
-- warranties of merchantability or fitness for a particular 
-- purpose.
--
-- Xilinx products are not intended for use in life support
-- appliances, devices, or systems. Use in such applications is
-- expressly prohibited.
--
-- Any modifications that are made to the Source Code are 
-- done at the user's sole risk and will be unsupported.
--
-- Copyright (c) 2006-2007 Xilinx, Inc. All rights reserved.
--
-- This copyright and support notice must be retained as part 
-- of this text at all times.
-------------------------------------------------------------------------------
--   ____  ____
--  /   /\/   /
-- /___/  \  /   Vendor		    : Xilinx
-- \   \   \/    Version	    : 2.3
--  \   \        Application	    : MIG
--  /   /        Filename	    : mig_23_fifo_1_wr_en_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose	: This module generate the write enable signal to the fifos, 
--		  which are driven by posedge of the data strobe.
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_fifo_1_wr_en_0 is
  port (
    clk             : in  std_logic;
    rst_dqs_delay_n : in  std_logic;
    reset           : in  std_logic;
    din             : in  std_logic;
    dout            : out std_logic
    );
end mig_23_fifo_1_wr_en_0;

architecture arc of mig_23_fifo_1_wr_en_0 is

  signal din_delay     : std_ulogic;
  signal tie_high      : std_ulogic;
  signal dout0         : std_ulogic;
  signal rst_dqs_delay : std_logic;

begin

  rst_dqs_delay <= not rst_dqs_delay_n;
  dout0         <= din and rst_dqs_delay_n;
  dout          <= rst_dqs_delay or din_delay;
  tie_high      <= '1';

  delay_ff_1 : FDCE
    port map (
      Q   => din_delay,
      C   => clk,
      CE  => tie_high,
      CLR => reset,
      D   => dout0
      );

end arc;
