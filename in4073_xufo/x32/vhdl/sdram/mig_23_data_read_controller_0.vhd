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
--  /   /        Filename	    : mig_23_data_read_controller_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose     : This module has instantiation fifo_0_wr_en, fifo_1_wr_en, 
--		           dqs_delay and wr_gray_cntr.
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.mig_23_parameters_0.all;

library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_data_read_controller_0 is
  port(
    clk                   : in  std_logic;
    reset                 : in  std_logic;
    rst_dqs_div_in        : in  std_logic;
    delay_sel             : in  std_logic_vector(4 downto 0);
    dqs_int_delay_in      : in std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    fifo_0_wr_en_val      : out std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    fifo_1_wr_en_val      : out std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    fifo_0_wr_addr_val    : out std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
    fifo_1_wr_addr_val    : out std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
    dqs_delayed_col0_val  : out std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    dqs_delayed_col1_val  : out std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    -- debug signals
    vio_out_dqs           : in  std_logic_vector(4 downto 0);
    vio_out_dqs_en        : in  std_logic;
    vio_out_rst_dqs_div   : in  std_logic_vector(4 downto 0);
    vio_out_rst_dqs_div_en: in  std_logic
    );

end mig_23_data_read_controller_0;

architecture arc of mig_23_data_read_controller_0 is

  component mig_23_dqs_delay_0
    port (
      clk_in  : in  std_logic;
      sel_in  : in  std_logic_vector(4 downto 0);
      clk_out : out std_logic
      );
  end component;

-- wr_gray_cntr is a gray counter with an ASYNC reset for fifo wr_addr
  component mig_23_wr_gray_cntr
    port (
      clk      : in  std_logic;
      reset    : in  std_logic;
      cnt_en   : in  std_logic;
      wgc_gcnt : out std_logic_vector(3 downto 0)
      );
  end component;

  component mig_23_fifo_0_wr_en_0
    port (
      clk             : in  std_logic;
      reset           : in  std_logic;
      din             : in  std_logic;
      rst_dqs_delay_n : out std_logic;
      dout            : out std_logic
      );
  end component;

  component mig_23_fifo_1_wr_en_0
    port (
      clk             : in  std_logic;
      rst_dqs_delay_n : in  std_logic;
      reset           : in  std_logic;
      din             : in  std_logic;
      dout            : out std_logic
      );
  end component;

  signal fifo_0_wr_addr      : std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
  signal fifo_1_wr_addr      : std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
  signal dqs_delayed_col0    : std_logic_vector((data_strobe_width-1) downto 0);
  signal dqs_delayed_col1    : std_logic_vector((data_strobe_width-1) downto 0);

-- FIFO WRITE ENABLE SIGNALS

  signal fifo_0_wr_en : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal fifo_1_wr_en : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);

  signal rst_dqs_div        : std_logic;
  signal reset_r             : std_logic;
  signal rst_dqs_delay_0_n  : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal dqs_delayed_col0_n : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal dqs_delayed_col1_n : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal delay_sel_rst_dqs_div : std_logic_vector(4 downto 0);
  signal delay_sel_dqs         : std_logic_vector(4 downto 0);

  attribute syn_preserve  : boolean;
  attribute buffer_type  : string;
  attribute buffer_type  of  dqs_delayed_col0: signal is "none";
  attribute buffer_type  of  dqs_delayed_col1: signal is "none";
  
begin

  process(clk)
  begin
    if(clk'event and clk = '1') then
      reset_r <= reset;
    end if;
  end process;


  fifo_0_wr_addr_val   <= fifo_0_wr_addr;
  fifo_1_wr_addr_val   <= fifo_1_wr_addr;
  fifo_0_wr_en_val     <= fifo_0_wr_en;
  fifo_1_wr_en_val     <= fifo_1_wr_en;
  dqs_delayed_col0_val <= dqs_delayed_col0;
  dqs_delayed_col1_val <= dqs_delayed_col1;

  gen_asgn : for asgn_i in 0 to DATA_STROBE_WIDTH-1 generate
    dqs_delayed_col0_n(asgn_i) <= not dqs_delayed_col0(asgn_i);
    dqs_delayed_col1_n(asgn_i) <= not dqs_delayed_col1(asgn_i);
  end generate;
  
  


  debug_rst_dqs_div_ena : if (DEBUG_EN = 1) generate
    delay_sel_rst_dqs_div <= vio_out_rst_dqs_div(4 downto 0) when  (vio_out_rst_dqs_div_en = '1')
				             else delay_sel;  
  end generate; 

  debug_rst_dqs_div_dis : if (DEBUG_EN = 0) generate
    delay_sel_rst_dqs_div <= delay_sel;  
  end generate; 
-- delayed rst_dqs_div logic

  rst_dqs_div_delayed : mig_23_dqs_delay_0
    port map (
      clk_in  => rst_dqs_div_in,
      sel_in  => delay_sel_rst_dqs_div,
      clk_out => rst_dqs_div
      );

  debug_ena : if (DEBUG_EN = 1) generate
    delay_sel_dqs <= vio_out_dqs(4 downto 0) when  (vio_out_dqs_en = '1')
				             else delay_sel;  
  end generate; 

  debug_dis : if (DEBUG_EN = 0) generate
    delay_sel_dqs <= delay_sel;  
  end generate; 
--******************************************************************************
-- DQS Internal Delay Circuit implemented in LUTs
--******************************************************************************

  gen_delay : for dly_i in 0 to DATA_STROBE_WIDTH-1 generate
    attribute syn_preserve of  dqs_delay_col0: label is true;
    attribute syn_preserve of  dqs_delay_col1: label is true;
  begin
    -- Internal Clock Delay circuit placed in the first
    -- column (for falling edge data) adjacent to IOBs
    dqs_delay_col0 : mig_23_dqs_delay_0
      port map (
        clk_in  => dqs_int_delay_in(dly_i),
        sel_in  => delay_sel_dqs,
        clk_out => dqs_delayed_col0(dly_i)
        );
    -- Internal Clock Delay circuit placed in the second
    --column (for rising edge data) adjacent to IOBs
    dqs_delay_col1 : mig_23_dqs_delay_0
      port map (
        clk_in  => dqs_int_delay_in(dly_i),
        sel_in  => delay_sel_dqs,
        clk_out => dqs_delayed_col1(dly_i)
        );
  end generate;

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

  gen_wr_en : for wr_en_i in 0 to DATA_STROBE_WIDTH-1 generate
    fifo_0_wr_en_inst : mig_23_fifo_0_wr_en_0
      port map (
        clk             => dqs_delayed_col1_n (wr_en_i),
        reset           => reset_r,
        din             => rst_dqs_div,
        rst_dqs_delay_n => rst_dqs_delay_0_n(wr_en_i),
        dout            => fifo_0_wr_en(wr_en_i)
        );
    fifo_1_wr_en_inst : mig_23_fifo_1_wr_en_0
      port map (
        clk             => dqs_delayed_col0(wr_en_i),
        rst_dqs_delay_n => rst_dqs_delay_0_n(wr_en_i),
        reset           => reset_r,
        din             => rst_dqs_div,
        dout            => fifo_1_wr_en(wr_en_i)
        );
  end generate;

-------------------------------------------------------------------------------
-- write pointer gray counter instances
-------------------------------------------------------------------------------

  gen_wr_addr : for wr_addr_i in 0 to DATA_STROBE_WIDTH-1 generate
    fifo_0_wr_addr_inst : mig_23_wr_gray_cntr
      port map (
        clk      => dqs_delayed_col1(wr_addr_i),
        reset    => reset_r,
        cnt_en   => fifo_0_wr_en(wr_addr_i),
        wgc_gcnt => fifo_0_wr_addr((wr_addr_i*4-1)+4 downto wr_addr_i*4)
        );
    fifo_1_wr_addr_inst : mig_23_wr_gray_cntr
      port map (
        clk      => dqs_delayed_col0_n(wr_addr_i),
        reset    => reset_r,
        cnt_en   => fifo_1_wr_en(wr_addr_i),
        wgc_gcnt => fifo_1_wr_addr((wr_addr_i*4-1)+4 downto wr_addr_i*4)
        );
  end generate;

end arc;
