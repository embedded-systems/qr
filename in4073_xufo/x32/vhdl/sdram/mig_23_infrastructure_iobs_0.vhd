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
--  /   /        Filename	    : mig_23_infrastructure_iobs_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose     : This module has the FDDRRSE instantiations for the clocks.
-------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.mig_23_parameters_0.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_infrastructure_iobs_0 is
  port(
    ddr_ck   : out std_logic_vector((CLK_WIDTH-1) downto 0);
    ddr_ck_n : out std_logic_vector((CLK_WIDTH-1) downto 0);
    clk0     : in  std_logic
    );
end mig_23_infrastructure_iobs_0;

architecture arc of mig_23_infrastructure_iobs_0 is

  signal vcc        : std_logic;
  signal gnd        : std_logic;
  signal clk180     : std_logic;
  signal ddr1_clk_q : std_logic_vector((CLK_WIDTH-1) downto 0);

begin

  gnd    <= '0';
  vcc    <= '1';
  clk180 <= not clk0;

--- ***********************************
---- This includes instantiation of the output DDR flip flop
---- for ddr clk's and dimm clk's
---- Ouput BUffers for ddr clk's and dimm clk's
---- ***********************************************************

  gen_clk : for i in 0 to CLK_WIDTH-1 generate
    U0 : FDDRRSE
      port map (
        Q  => ddr1_clk_q(i),
        C0 => clk0,
        C1 => clk180,
        CE => vcc,
        D0 => vcc,
        D1 => gnd,
        R  => gnd,
        S  => gnd
        );
    
    r0  : OBUFDS
      port map (
        I  => ddr1_clk_q(i),
        O  => ddr_ck(i),
        OB => ddr_ck_n(i)
        );
  end generate gen_clk;

end arc;
