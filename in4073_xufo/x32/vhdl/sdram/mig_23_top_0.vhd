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
--  /   /        Filename	    : mig_23_top_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose     : This is main controller block. This includes the following
--               features:
--                    - The main controller state machine that controlls the 
--                    initialization process upon power up, as well as the 
--                    read, write, and refresh commands. 
--                    - handles the data path during READ and WRITEs.
--                    - Generates control signals for other modules, including 
--			the data strobe(DQS) signal
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use work.mig_23_parameters_0.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_top_0 is
  port(
    auto_ref_req          : out   std_logic;
    wait_200us            : in    std_logic;
    rst_dqs_div_in        : in    std_logic;
    rst_dqs_div_out       : out   std_logic;
    user_input_data       : in    std_logic_vector(((DATA_WIDTH*2)-1) downto 0);
    user_data_mask        : in    std_logic_vector(((DATA_MASK_WIDTH*2) -1) downto 0);
    user_output_data      : out   std_logic_vector(((DATA_WIDTH*2) -1)
                                                   downto 0) := (others => 'Z');
    user_data_valid       : out   std_logic;
    user_input_address    : in    std_logic_vector(((ROW_ADDRESS + COLUMN_ADDRESS +
                                                     BANK_ADDRESS )-1) downto 0);
    user_command_register : in    std_logic_vector(2 downto 0);
    user_cmd_ack          : out   std_logic;
    burst_done            : in    std_logic;
    init_val              : out   std_logic;
    ar_done               : out   std_logic;

    ddr_dqs               : inout std_logic_vector((DATA_STROBE_WIDTH -1) downto 0);
    ddr_dq                : inout std_logic_vector((DATA_WIDTH -1)
                                                   downto 0) := (others => 'Z');
    ddr_cke               : out   std_logic;
    ddr_cs_n              : out   std_logic;
    ddr_ras_n             : out   std_logic;
    ddr_cas_n             : out   std_logic;
    ddr_we_n              : out   std_logic;
    ddr_dm                : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
    ddr_ba                : out   std_logic_vector((BANK_ADDRESS -1) downto 0);
    ddr_a                 : out   std_logic_vector((ROW_ADDRESS -1) downto 0);
    ddr_ck                : out   std_logic_vector((CLK_WIDTH-1) downto 0);
    ddr_ck_n              : out   std_logic_vector((CLK_WIDTH-1) downto 0);
  clk_tb                : out  std_logic;
  clk90_tb              : out  std_logic;
  sys_rst_tb            : out std_logic;
  sys_rst90_tb          : out  std_logic;
  sys_rst180_tb         : out  std_logic;

    clk_int               : in    std_logic;
    clk90_int             : in    std_logic;
    delay_sel_val         : in    std_logic_vector(4 downto 0);
    sys_rst_val           : in    std_logic;
    sys_rst90_val         : in    std_logic;
    sys_rst180_val        : in    std_logic;
    -- debug signals
    dbg_delay_sel         : out std_logic_vector(4 downto 0);
    dbg_rst_calib         : out std_logic;
    vio_out_dqs            : in  std_logic_vector(4 downto 0);
    vio_out_dqs_en         : in  std_logic;
    vio_out_rst_dqs_div    : in  std_logic_vector(4 downto 0);
    vio_out_rst_dqs_div_en : in  std_logic
    );
end mig_23_top_0;

architecture arc of mig_23_top_0 is

  component mig_23_controller_0
    port(
      auto_ref_req      : out std_logic;
      wait_200us        : in  std_logic;
      clk               : in  std_logic;
      rst0              : in  std_logic;
      rst180            : in  std_logic;
      address           : in  std_logic_vector(((ROW_ADDRESS +
                                                 COLUMN_ADDRESS )-1) downto 0);
      bank_address1     : in  std_logic_vector((BANK_ADDRESS -1) downto 0);
      command_register  : in  std_logic_vector(2 downto 0);
      burst_done        : in  std_logic;
      ddr_rasb_cntrl    : out std_logic;
      ddr_casb_cntrl    : out std_logic;
      ddr_web_cntrl     : out std_logic;
      ddr_ba_cntrl      : out std_logic_vector((BANK_ADDRESS -1) downto 0);
      ddr_address_cntrl : out std_logic_vector((ROW_ADDRESS -1) downto 0);
      ddr_cke_cntrl     : out std_logic;
      ddr_csb_cntrl     : out std_logic;
      dqs_enable        : out std_logic;
      dqs_reset         : out std_logic;
      write_enable      : out std_logic;
      rst_calib         : out std_logic;
      rst_dqs_div_int   : out std_logic;
      cmd_ack           : out std_logic;
      init              : out std_logic;
      ar_done           : out std_logic;
      read_fifo_rden    : out std_logic  -- Added new signal                         
      );
  end component;

  component mig_23_data_path_0
    port(
      user_input_data    : in  std_logic_vector(((DATA_WIDTH*2) -1) downto 0);
      user_data_mask     : in  std_logic_vector(((DATA_MASK_WIDTH*2) -1) downto 0);
      clk                : in  std_logic;
      clk90              : in  std_logic;
      reset              : in  std_logic;
      reset90            : in  std_logic;
      write_enable       : in  std_logic;
      rst_dqs_div_in     : in  std_logic;
      delay_sel          : in  std_logic_vector(4 downto 0);
      dqs_int_delay_in   : in std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      dq                 : in  std_logic_vector((DATA_WIDTH -1) downto 0);
      u_data_val         : out std_logic;
      user_output_data   : out std_logic_vector(((DATA_WIDTH*2) -1) downto 0);
      write_en_val       : out std_logic;
      data_mask_f        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      data_mask_r        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      write_data_falling : out std_logic_vector((DATA_WIDTH -1) downto 0);
      write_data_rising  : out std_logic_vector((DATA_WIDTH-1) downto 0);
      read_fifo_rden     : in std_logic;  -- Added new signal
    -- debug singals
     vio_out_dqs            : in  std_logic_vector(4 downto 0);
     vio_out_dqs_en         : in  std_logic;
     vio_out_rst_dqs_div    : in  std_logic_vector(4 downto 0);
     vio_out_rst_dqs_div_en : in  std_logic
      );
  end component;

  component mig_23_infrastructure
    port(
      clk_int            : in  std_logic;
      rst_calib1         : in  std_logic;
      delay_sel_val      : in  std_logic_vector(4 downto 0);
      delay_sel_val1_val : out std_logic_vector(4 downto 0);
    -- debug signals
      dbg_delay_sel      : out std_logic_vector(4 downto 0);
      dbg_rst_calib      : out std_logic
      );
  end component;

  component mig_23_iobs_0
    port(
      clk                : in    std_logic;
      clk90              : in    std_logic;
      ddr_rasb_cntrl     : in    std_logic;
      ddr_casb_cntrl     : in    std_logic;
      ddr_web_cntrl      : in    std_logic;
      ddr_cke_cntrl      : in    std_logic;
      ddr_csb_cntrl      : in    std_logic;
      ddr_address_cntrl  : in    std_logic_vector((ROW_ADDRESS -1) downto 0);
      ddr_ba_cntrl       : in    std_logic_vector((BANK_ADDRESS -1) downto 0);
      rst_dqs_div_int    : in    std_logic;
      dqs_reset          : in    std_logic;
      dqs_enable         : in    std_logic;
      ddr_dqs            : inout std_logic_vector((DATA_STROBE_WIDTH -1) downto 0);
      ddr_dq             : inout std_logic_vector((DATA_WIDTH -1) downto 0);
      write_data_falling : in    std_logic_vector((DATA_WIDTH -1) downto 0);
      write_data_rising  : in    std_logic_vector((DATA_WIDTH -1) downto 0);
      write_en_val       : in    std_logic;
      data_mask_f        : in std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      data_mask_r        : in std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      ddr_ck             : out   std_logic_vector((CLK_WIDTH-1) downto 0);
      ddr_ck_n           : out   std_logic_vector((CLK_WIDTH-1) downto 0);
      ddr_rasb           : out   std_logic;
      ddr_casb           : out   std_logic;
      ddr_web            : out   std_logic;
      ddr_ba             : out   std_logic_vector((BANK_ADDRESS -1) downto 0);
      ddr_address        : out   std_logic_vector((ROW_ADDRESS -1) downto 0);
      ddr_cke            : out   std_logic;
      ddr_csb            : out   std_logic;
      rst_dqs_div        : out   std_logic;
      rst_dqs_div_in     : in    std_logic;
      rst_dqs_div_out    : out   std_logic;
      dqs_int_delay_in   : out std_logic_vector((DATA_STROBE_WIDTH-1) downto 0); 
      ddr_dm             : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      dq                 : out   std_logic_vector((DATA_WIDTH -1) downto 0)
      );
  end component;


  signal rst_calib          : std_logic;
  signal delay_sel          : std_logic_vector(4 downto 0);
  signal write_enable       : std_logic;
  signal dqs_div_rst        : std_logic;
  signal dqs_enable         : std_logic;
  signal dqs_reset          : std_logic;
  signal dqs_int_delay_in   : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);

  signal dq                 : std_logic_vector((DATA_WIDTH -1) downto 0);
  signal write_en_val       : std_logic;
  signal data_mask_f        : std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
  signal data_mask_r        : std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
  signal write_data_falling : std_logic_vector((DATA_WIDTH -1) downto 0);
  signal write_data_rising  : std_logic_vector((DATA_WIDTH -1) downto 0);
  signal ddr_rasb_cntrl     : std_logic;
  signal ddr_casb_cntrl     : std_logic;
  signal ddr_web_cntrl      : std_logic;
  signal ddr_ba_cntrl       : std_logic_vector((BANK_ADDRESS -1) downto 0);
  signal ddr_address_cntrl  : std_logic_vector((ROW_ADDRESS -1) downto 0);
  signal ddr_cke_cntrl      : std_logic;
  signal ddr_csb_cntrl      : std_logic;
  signal rst_dqs_div_int    : std_logic;
  signal read_fifo_rden     : std_logic;
begin 
 
  
    clk_tb            <=  clk_int after 1 ps;
  clk90_tb          <=  clk90_int after 1 ps;
  sys_rst_tb        <=  sys_rst_val;
  sys_rst90_tb      <=  sys_rst90_val;
  sys_rst180_tb     <=  sys_rst180_val;

  controller0 : mig_23_controller_0
    port map (
      auto_ref_req      => auto_ref_req,
      wait_200us        => wait_200us,
      clk               => clk_int,
      rst0              => sys_rst_val,
      rst180            => sys_rst180_val,
      address           => user_input_address(((ROW_ADDRESS + COLUMN_ADDRESS
                                                + BANK_ADDRESS )-1) downto BANK_ADDRESS),
      bank_address1     => user_input_address(BANK_ADDRESS - 1 downto 0),
      command_register  => user_command_register,
      burst_done        => burst_done,
      ddr_rasb_cntrl    => ddr_rasb_cntrl,
      ddr_casb_cntrl    => ddr_casb_cntrl,
      ddr_web_cntrl     => ddr_web_cntrl,
      ddr_ba_cntrl      => ddr_ba_cntrl,
      ddr_address_cntrl => ddr_address_cntrl,
      ddr_cke_cntrl     => ddr_cke_cntrl,
      ddr_csb_cntrl     => ddr_csb_cntrl,
      dqs_enable        => dqs_enable,
      dqs_reset         => dqs_reset,
      write_enable      => write_enable,
      rst_calib         => rst_calib,
      rst_dqs_div_int   => rst_dqs_div_int,
      cmd_ack           => user_cmd_ack,
      init              => init_val,
      ar_done           => ar_done,
      read_fifo_rden    => read_fifo_rden -- Added new signal
      );

  data_path0 : mig_23_data_path_0
    port map (
      user_input_data    => user_input_data,
      user_data_mask     => user_data_mask,
      clk                => clk_int,
      clk90              => clk90_int,
      reset              => sys_rst_val,
      reset90            => sys_rst90_val,
      write_enable       => write_enable,
      rst_dqs_div_in     => dqs_div_rst,
      delay_sel          => delay_sel,
      dqs_int_delay_in   => dqs_int_delay_in,
      dq                 => dq,
      u_data_val         => user_data_valid,
      user_output_data   => user_output_data,
      write_en_val       => write_en_val,
      data_mask_f        => data_mask_f,
      data_mask_r        => data_mask_r,
      write_data_falling => write_data_falling,
      write_data_rising  => write_data_rising,
      read_fifo_rden     => read_fifo_rden, -- Added new signal
--debug signals
      vio_out_dqs            => vio_out_dqs,
      vio_out_dqs_en         => vio_out_dqs_en,
      vio_out_rst_dqs_div    => vio_out_rst_dqs_div,
      vio_out_rst_dqs_div_en => vio_out_rst_dqs_div_en
      );

  infrastructure0 : mig_23_infrastructure
    port map(
      clk_int            => clk_int,
      rst_calib1         => rst_calib,
      delay_sel_val      => delay_sel_val,
      delay_sel_val1_val => delay_sel,
      dbg_delay_sel      => dbg_delay_sel,
      dbg_rst_calib      => dbg_rst_calib
      );

  iobs0 : mig_23_iobs_0
    port map (
      clk                => clk_int,
      clk90              => clk90_int,
      ddr_rasb_cntrl     => ddr_rasb_cntrl,
      ddr_casb_cntrl     => ddr_casb_cntrl,
      ddr_web_cntrl      => ddr_web_cntrl,
      ddr_cke_cntrl      => ddr_cke_cntrl,
      ddr_csb_cntrl      => ddr_csb_cntrl,
      ddr_address_cntrl  => ddr_address_cntrl,
      ddr_ba_cntrl       => ddr_ba_cntrl,
      rst_dqs_div_int    => rst_dqs_div_int,
      dqs_reset          => dqs_reset,
      dqs_enable         => dqs_enable,
      ddr_dqs            => ddr_dqs,
      ddr_dq             => ddr_dq,
      write_data_falling => write_data_falling,
      write_data_rising  => write_data_rising,
      write_en_val       => write_en_val,
      data_mask_f        => data_mask_f,
      data_mask_r        => data_mask_r,
      ddr_ck             => ddr_ck,
      ddr_ck_n           => ddr_ck_n,
      ddr_rasb           => ddr_ras_n,
      ddr_casb           => ddr_cas_n,
      ddr_web            => ddr_we_n,
      ddr_ba             => ddr_ba,
      ddr_address        => ddr_a,
      ddr_cke            => ddr_cke,
      ddr_csb            => ddr_cs_n,
      rst_dqs_div        => dqs_div_rst,
      rst_dqs_div_in     => rst_dqs_div_in,
      rst_dqs_div_out    => rst_dqs_div_out,
      dqs_int_delay_in   => dqs_int_delay_in,
      ddr_dm             => ddr_dm,
      dq                 => dq
      );
  
end   arc ;  
