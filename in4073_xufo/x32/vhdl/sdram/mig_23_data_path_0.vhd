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
--  /   /        Filename	    : mig_23_data_path_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose     : This module has the write and read data paths for the
--               DDR1 memory interface. The write data along with write enable 
--               signals are forwarded to the DDR IOB FFs. The read data is 
--               captured in CLB FFs and finally input to FIFOs.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.mig_23_parameters_0.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_data_path_0 is
  port(
    user_input_data    : in  std_logic_vector(((DATA_WIDTH*2)-1) downto 0);
    user_data_mask     : in  std_logic_vector(((DATA_MASK_WIDTH*2) -1) downto 0);
    clk                : in  std_logic;
    clk90              : in  std_logic;
    reset              : in  std_logic;
    reset90            : in  std_logic;
    write_enable       : in  std_logic;
    rst_dqs_div_in     : in  std_logic;
    delay_sel          : in  std_logic_vector(4 downto 0);
    dq                 : in  std_logic_vector((DATA_WIDTH-1) downto 0);
    u_data_val         : out std_logic;
    user_output_data   : out std_logic_vector(((DATA_WIDTH*2)-1) downto 0);
    write_en_val       : out std_logic;
    dqs_int_delay_in   : in std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
    data_mask_f        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
    data_mask_r        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
    write_data_falling : out std_logic_vector((DATA_WIDTH-1) downto 0);
    write_data_rising  : out std_logic_vector((DATA_WIDTH-1) downto 0);
    read_fifo_rden     : in std_logic;  -- Added new signal
    -- debug signals
    vio_out_dqs            : in  std_logic_vector(4 downto 0);
    vio_out_dqs_en         : in  std_logic;
    vio_out_rst_dqs_div    : in  std_logic_vector(4 downto 0);
    vio_out_rst_dqs_div_en : in  std_logic
    );
end mig_23_data_path_0;

architecture arc of mig_23_data_path_0 is

  component mig_23_data_read_0
    port (
      clk90             : in  std_logic;
      reset90           : in  std_logic;
      ddr_dq_in         : in  std_logic_vector((DATA_WIDTH-1) downto 0);
      fifo_0_wr_en      : in  std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      fifo_1_wr_en      : in  std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      fifo_0_wr_addr    : in  std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
      fifo_1_wr_addr    : in  std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
      dqs_delayed_col0  : in  std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      dqs_delayed_col1  : in  std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      read_fifo_rden    : in std_logic;  
      user_output_data  : out std_logic_vector(((DATA_WIDTH*2)-1) downto 0);
      u_data_val        : out std_logic

      );
  end component;

  component mig_23_data_read_controller_0
    port (
      clk                   : in  std_logic;
      reset                 : in  std_logic;
      rst_dqs_div_in        : in  std_logic;
      delay_sel             : in  std_logic_vector(4 downto 0);
      dqs_int_delay_in      : in std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
      fifo_0_wr_en_val      : out std_logic_vector((DATA_STROBE_WIDTH-1)
                                                   downto 0);
      fifo_1_wr_en_val      : out std_logic_vector((DATA_STROBE_WIDTH-1)
                                                   downto 0);
      fifo_0_wr_addr_val    : out std_logic_vector((4*DATA_STROBE_WIDTH)-1
                                                   downto 0);
      fifo_1_wr_addr_val    : out std_logic_vector((4*DATA_STROBE_WIDTH)-1
                                                   downto 0);
      dqs_delayed_col0_val  : out std_logic_vector((DATA_STROBE_WIDTH-1)
                                                   downto 0);
      dqs_delayed_col1_val  : out std_logic_vector((DATA_STROBE_WIDTH-1)
                                                   downto 0);
      vio_out_dqs            : in  std_logic_vector(4 downto 0);
      vio_out_dqs_en         : in  std_logic;
      vio_out_rst_dqs_div    : in  std_logic_vector(4 downto 0);
      vio_out_rst_dqs_div_en : in  std_logic
      );
  end component;

  component mig_23_data_write_0
    port (
      user_input_data    : in  std_logic_vector(((DATA_WIDTH*2)-1) downto 0);
      user_data_mask     : in  std_logic_vector(((DATA_MASK_WIDTH*2) -1) downto 0);
      clk90              : in  std_logic;
      write_enable       : in  std_logic;
      write_data_falling : out std_logic_vector((DATA_WIDTH-1) downto 0);
      write_data_rising  : out std_logic_vector((DATA_WIDTH-1) downto 0);
      data_mask_f        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      data_mask_r        : out std_logic_vector((DATA_MASK_WIDTH-1) downto 0);
      write_en_val       : out std_logic
      );
  end component;



  signal fifo_0_wr_addr    : std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
  signal fifo_1_wr_addr    : std_logic_vector((4*DATA_STROBE_WIDTH)-1 downto 0);
  signal fifo_0_wr_en      : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal fifo_1_wr_en      : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal dqs_delayed_col0  : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);
  signal dqs_delayed_col1  : std_logic_vector((DATA_STROBE_WIDTH-1) downto 0);

begin

  data_read0 : mig_23_data_read_0
    port map (
      clk90             => clk90,
      reset90           => reset90,
      ddr_dq_in         => dq,
      fifo_0_wr_en      => fifo_0_wr_en,
      fifo_1_wr_en      => fifo_1_wr_en,
      fifo_0_wr_addr    => fifo_0_wr_addr,
      fifo_1_wr_addr    => fifo_1_wr_addr,
      dqs_delayed_col0  => dqs_delayed_col0,
      dqs_delayed_col1  => dqs_delayed_col1,
      read_fifo_rden    => read_fifo_rden,
      user_output_data  => user_output_data,
      u_data_val        => u_data_val
      );

  data_read_controller0 : mig_23_data_read_controller_0
    port map (
      clk                   => clk,
      reset                 => reset,
      rst_dqs_div_in        => rst_dqs_div_in,
      delay_sel             => delay_sel,
      dqs_int_delay_in      => dqs_int_delay_in,
      fifo_0_wr_en_val      => fifo_0_wr_en,
      fifo_1_wr_en_val      => fifo_1_wr_en,
      fifo_0_wr_addr_val    => fifo_0_wr_addr,
      fifo_1_wr_addr_val    => fifo_1_wr_addr,
      dqs_delayed_col0_val  => dqs_delayed_col0,
      dqs_delayed_col1_val  => dqs_delayed_col1,
      vio_out_dqs            => vio_out_dqs,
      vio_out_dqs_en         => vio_out_dqs_en,
      vio_out_rst_dqs_div    => vio_out_rst_dqs_div,
      vio_out_rst_dqs_div_en => vio_out_rst_dqs_div_en
      );

  data_write0 : mig_23_data_write_0
    port map (
      user_input_data    => user_input_data,
      user_data_mask     => user_data_mask,
      clk90              => clk90,
      write_enable       => write_enable,
      write_en_val       => write_en_val,
      write_data_falling => write_data_falling,
      write_data_rising  => write_data_rising,
      data_mask_f        => data_mask_f,
      data_mask_r        => data_mask_r
      );


end arc;                                                                                                                 
