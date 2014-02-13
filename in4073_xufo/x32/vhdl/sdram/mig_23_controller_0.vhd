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
--  /   /        Filename	    : mig_23_controller_0.vhd
-- /___/   /\    Date Last Modified : $Date: 2008/08/12 15:32:22 $
-- \   \  /  \   Date Created       : Mon May 2 2005
--  \___\/\___\
-- Device      : Spartan-3/3E/3A/3A-DSP
-- Design Name : DDR SDRAM
-- Purpose     : This is main controller block. This includes the following
--                features:
--                - The controller state machine that controls the
--                  initialization process upon power up, as well as the
--                  read, write, and refresh commands.
--                - Accepts and decodes the user commands.
--                - Generates the address and Bank address and control signals
--                  to the memory    
--                - Generates control signals for other modules.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.mig_23_parameters_0.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

entity mig_23_controller_0 is
  generic
    (
     COL_WIDTH : integer := COLUMN_ADDRESS;
     ROW_WIDTH : integer := ROW_ADDRESS
    );
  port(
    clk               : in  std_logic;
    rst0              : in  std_logic;
    rst180            : in  std_logic;
    address           : in  std_logic_vector(((ROW_ADDRESS + COLUMN_ADDRESS)-1)
                                            downto 0);
    bank_address1     : in  std_logic_vector((BANK_ADDRESS-1) downto 0);
    command_register  : in  std_logic_vector(2 downto 0);
    burst_done        : in  std_logic;
    ddr_rasb_cntrl    : out std_logic;
    ddr_casb_cntrl    : out std_logic;
    ddr_web_cntrl     : out std_logic;
    ddr_ba_cntrl      : out std_logic_vector((BANK_ADDRESS-1) downto 0);
    ddr_address_cntrl : out std_logic_vector((ROW_ADDRESS-1) downto 0);
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
    wait_200us        : in  std_logic;
    auto_ref_req      : out std_logic;
    read_fifo_rden    : out std_logic -- Read Enable signal for read fifo(to data_read)
    );
end mig_23_controller_0;


architecture arc of mig_23_controller_0 is


  type s_m is (
    IDLE, PRECHARGE, PRECHARGE_WAIT, AUTO_REFRESH, AUTO_REFRESH_WAIT, ACTIVE,
    FIRST_WRITE, WRITE_WAIT, BURST_WRITE, PRECHARGE_AFTER_WRITE,
    PRECHARGE_AFTER_WRITE_2, READ_WAIT, BURST_READ, ACTIVE_WAIT
    );

  signal next_state, current_state : s_m;

  type s_m1 is (
    INIT_IDLE, INIT_PRECHARGE, INIT_LOAD_MODE_REG, INIT_AUTO_REFRESH
    );

  signal init_next_state, init_current_state : s_m1;

  signal ack_reg              : std_logic;
  signal ack_o                : std_logic;
  signal address_reg          : std_logic_vector((ROW_ADDRESS -1) downto 0);
  signal auto_ref             : std_logic;
  signal auto_ref1            : std_logic;
  signal autoref_value        : std_logic;
  signal auto_ref_detect1     : std_logic;
  signal autoref_count        : std_logic_vector(10 downto 0);
  signal autoref_cnt_val      : std_logic_vector(10 downto 0);
  signal ar_done_p            : std_logic;
  signal ar_done_reg          : std_logic;
  signal auto_ref_issued      : std_logic;
  signal auto_ref_issued_p    : std_logic;
  signal ba_address_reg1      : std_logic_vector((BANK_ADDRESS-1) downto 0);
  signal ba_address_reg2      : std_logic_vector((BANK_ADDRESS-1) downto 0);
  signal burst_len            : std_logic_vector(2 downto 0);
  signal burst_cnt_max        : std_logic_vector(2 downto 0);
  signal cas_count            : std_logic_vector(2 downto 0);
  signal cas_latency          : std_logic_vector(2 downto 0);
  signal column_address_reg   : std_logic_vector((ROW_ADDRESS -1) downto 0);
  signal column_address1      : std_logic_vector((ROW_ADDRESS -1) downto 0);
  signal ddr_rasb1            : std_logic;
  signal ddr_casb1            : std_logic;
  signal ddr_web1             : std_logic;
  signal ddr_rasb2            : std_logic;
  signal ddr_casb2            : std_logic;
  signal ddr_web2             : std_logic;
  signal ddr_rst_dqs_rasb4    : std_logic;
  signal ddr_rst_dqs_casb4    : std_logic;
  signal ddr_rst_dqs_web4     : std_logic;
  signal ddr_ba1              : std_logic_vector((BANK_ADDRESS-1) downto 0);
  signal ddr_address1         : std_logic_vector((ROW_ADDRESS-1) downto 0);
  signal dll_rst_count        : std_logic_vector(7 downto 0);
  signal init_count           : std_logic_vector(2 downto 0);
  signal init_done            : std_logic;
  signal init_done_r1         : std_logic;
  signal init_done_dis        : std_logic;
  signal init_done_value      : std_logic;
  signal init_memory          : std_logic;
  signal init_mem             : std_logic;
  signal initialize_memory    : std_logic;
  signal read_cmd             : std_logic;
  signal read_cmd1            : std_logic;
  signal rcdr_count           : std_logic_vector(1 downto 0);
  signal rcdw_count           : std_logic_vector(1 downto 0);
  signal rfc_count_reg        : std_logic;
  signal rdburst_end_1        : std_logic;
  signal rdburst_end          : std_logic;
  signal rp_count             : std_logic_vector(2 downto 0);
  signal rfc_count            : std_logic_vector(5 downto 0);
  signal row_address1         : std_logic_vector((ROW_ADDRESS-1) downto 0);
  signal row_address_reg      : std_logic_vector((ROW_ADDRESS-1) downto 0);
  signal rst_dqs_div_r        : std_logic;
  signal wrburst_end_cnt      : std_logic_vector(2 downto 0);
  signal wrburst_end          : std_logic;
  signal wrburst_end_1        : std_logic;
  signal wrburst_end_2        : std_logic;
  signal wr_count             : std_logic_vector(1 downto 0);

  signal write_cmd_in         : std_logic;
  signal write_cmd1           : std_logic;
  signal go_to_active_value   : std_logic;
  signal go_to_active         : std_logic;
  signal accept_cmd_in        : std_logic;
  signal dqs_div_cascount     : std_logic_vector(2 downto 0);
  signal dqs_div_rdburstcount : std_logic_vector(2 downto 0);
  signal dqs_enable1          : std_logic;
  signal dqs_enable2          : std_logic;
  signal dqs_enable3          : std_logic;
  signal dqs_reset1_clk0      : std_logic;
  signal dqs_reset2_clk0      : std_logic;
  signal dqs_reset3_clk0      : std_logic;
  signal dqs_enable_int       : std_logic;
  signal dqs_reset_int        : std_logic;
  signal rst180_r             : std_logic;
  signal rst0_r               : std_logic;
  signal low                  : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal rpcnt0               : std_logic;
  signal write_enable_r       : std_logic;
  signal write_enable_r1      : std_logic;
  signal auto_ref_wait        : std_logic;
  signal auto_ref_wait1       : std_logic;
  signal auto_ref_wait2       : std_logic;
  signal clk180               : std_logic;
  signal lmr                  : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal emr                  : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal emr_1                : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal lmr_dll_rst          : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal lmr_dll_set          : std_logic_vector((ROW_ADDRESS - 1) downto 0);
  signal count5               : std_logic_vector(4 downto 0);
  signal rst_dqs_div_r1       : std_logic;
  signal rst_dqs_div_r2       : std_logic;
  signal rst_dqs_div_d        : std_logic;

  constant CNT_NEXT          : std_logic_vector(4 downto 0) := "11000";
  constant LMR1              : std_logic_vector((ROW_ADDRESS - 1) downto 0) 
				:= LOAD_MODE_REGISTER; 
  constant CAS_LAT_VAL       : std_logic_vector(2 downto 0) := LMR1(6 downto 4);
  constant BURST_LEN_VAL     : std_logic_vector(2 downto 0) := LMR1(2 downto 0);

  attribute iob			: string;
  attribute syn_useioff		: boolean;
  attribute syn_preserve	: boolean;

  attribute syn_preserve of low			: signal is true;
  attribute syn_preserve of rst_dqs_div_r	: signal is true;
  
begin

-- Input : COMMAND REGISTER FORMAT
-- 000 - NOP
-- 010 - initialize memory
-- 100 - write request
-- 110 - Read request

-- Input : Address format
-- row address = address((ROW_ADDRESS+ COLUMN_ADDRESS) -1 downto COLUMN_ADDRESS)
-- column address = address( COLUMN_ADDRESS-1 downto 0)

  clk180            <= not clk;
  low               <= (others => '0');
  lmr               <= LOAD_MODE_REGISTER;
  emr               <= EXT_LOAD_MODE_REGISTER;
  ddr_csb_cntrl     <= '0';
  ddr_cke_cntrl     <= not wait_200us;
  row_address1      <= address(((ROW_ADDRESS + COLUMN_ADDRESS)-1)
                               downto COLUMN_ADDRESS);
--  column_address1   <= address_reg((ROW_ADDRESS-1) downto 11) & '0' & address_reg(9 downto 0);
  init              <= init_done;
  ddr_rasb_cntrl    <= ddr_rasb2;
  ddr_casb_cntrl    <= ddr_casb2;
  ddr_web_cntrl     <= ddr_web2;
  ddr_address_cntrl <= ddr_address1;
  ddr_ba_cntrl      <= ddr_ba1;


  -- turn off auto-precharge when issuing read/write commands (A10 = 0)
  -- mapping the column address for linear addressing.
  gen_ddr_addr_col_0: if (COL_WIDTH = ROW_WIDTH-1) generate
    column_address1 <= (address_reg(COL_WIDTH-1 downto 10) & '0' &
                        address_reg(9 downto 0));
  end generate;

  gen_ddr_addr_col_1: if ((COL_WIDTH > 10) and
                          not(COL_WIDTH = ROW_WIDTH-1)) generate
    column_address1(ROW_WIDTH-1 downto COL_WIDTH+1) <= (others => '0');
    column_address1(COL_WIDTH downto 0) <=
      (address_reg(COL_WIDTH-1 downto 10) & '0' & address_reg(9 downto 0));
  end generate;

  gen_ddr_addr_col_2: if (not((COL_WIDTH > 10) or
                              (COL_WIDTH = ROW_WIDTH-1))) generate
    column_address1(ROW_WIDTH-1 downto COL_WIDTH+1) <= (others => '0');
    column_address1(COL_WIDTH downto 0) <=
      ('0' & address_reg(COL_WIDTH-1 downto 0));
  end generate;


  process(clk)
  begin
    if(clk'event and clk = '0') then
      rst180_r <= rst180;
    end if;
  end process;

  process(clk)
  begin
    if clk'event and clk = '1' then
      rst0_r <= rst0;
    end if;
  end process;

--******************************************************************************
-- Register user address
--******************************************************************************

  process(clk)
  begin
    if(clk'event and clk = '0') then
      row_address_reg    <= row_address1;
      column_address_reg <= column_address1;
      ba_address_reg1    <= bank_address1;
      ba_address_reg2    <= ba_address_reg1;
      address_reg        <= address((ROW_ADDRESS-1) downto 0);
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        burst_len    <= "000";
        cas_latency  <= "000";
      else
        burst_len    <= lmr(2 downto 0);
        cas_latency  <= lmr(6 downto 4);
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        accept_cmd_in <= '0';
      elsif (current_state = IDLE and ((rpcnt0 and rfc_count_reg and
                                        not(auto_ref_wait) and
                                        not(auto_ref_issued)) = '1')) then
        accept_cmd_in <= '1';
      else
        accept_cmd_in <= '0';
      end if;
    end if;
  end process;

--******************************************************************************
-- Commands from user.
--******************************************************************************

  initialize_memory <= '1' when (command_register = "010") else '0';
  write_cmd_in      <= '1' when (command_register = "100"
                                 and accept_cmd_in = '1')  else '0';
  read_cmd          <= '1' when (command_register = "110"
                                 and accept_cmd_in = '1')  else '0';

--******************************************************************************
-- write_cmd1 is asserted when user issued write command and the controller s/m
-- is in idle state and AUTO_REF is not asserted.
--******************************************************************************

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        write_cmd1   <= '0';
      else
        if (accept_cmd_in = '1') then
          write_cmd1 <= write_cmd_in;
        end if;
      end if;
    end if;
  end process;

--******************************************************************************
-- read_cmd1 is asserted when user issued read command and the controller s/m
-- is in idle state and AUTO_REF is not asserted.
--******************************************************************************

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        read_cmd1   <= '0';
      else
        if (accept_cmd_in = '1') then
          read_cmd1 <= read_cmd;
        end if;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if(rst180_r = '1') then
        count5 <= "00000";
      elsif(init_current_state = INIT_LOAD_MODE_REG or init_current_state = INIT_PRECHARGE or
            init_current_state = INIT_AUTO_REFRESH) then
        count5 <= CNT_NEXT;
      elsif(count5 /= "00001") then
        count5 <= count5 - "00001";
      else
        count5 <= count5;
      end if;
    end if;
  end process;

--******************************************************************************
-- rfc_count
-- An executable command can be issued only after Trfc period after a AUTOREFRESH
-- command is issued. rfc_count_value is set in the parameter file depending on
-- the memory device speed grade and the selected frequency.For example for 75
-- speed grade trfc=75, at 133Mhz, rfc_counter_value = 6'b001010.
-- ( Trfc/clk_period= 75/7.5= 10)
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        rfc_count <= "000000";
      elsif current_state = AUTO_REFRESH then
        rfc_count <= rfc_count_value;
      elsif rfc_count /= "000000" then
        rfc_count <= rfc_count - "000001";
      else
        rfc_count <= rfc_count;
      end if;
    end if;
  end process;

--******************************************************************************
-- rp_count
-- An executable command can be issued only after Trp period after a PRECHARGE
-- command is issued. rp_count value is fixed to support all memory speed grades.
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        rp_count <= "000";
      elsif current_state = PRECHARGE then
        rp_count <= "100";
      elsif rp_count /= "000" then
        rp_count <= rp_count - "001";
      else
        rp_count <= rp_count;
      end if;
    end if;
  end process;

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        rpcnt0 <= '1';
      elsif (rp_count > "001") then
        rpcnt0 <= '0';
      else
        rpcnt0 <= '1';
      end if;
    end if;
  end process;

--******************************************************************************
-- ACTIVE to READ/WRITE counter
--
-- rcdr_count
-- ACTIVE to READ delay - Minimum interval between ACTIVE and READ command.
-- rcdr_count value is fixed to support all memory speed grades.
--
-- rcdw_count
-- ACTIVE to WRITE delay - Minimum interval between ACTIVE and WRITE command.
-- rcdw_count value is fixed to support all memory speed grades.
--
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        rcdr_count <= "00";
      elsif current_state = ACTIVE then
        rcdr_count <= "10";
      elsif rcdr_count /= "01" then
        rcdr_count <= rcdr_count - "01";
      else
        rcdr_count <= rcdr_count;
      end if;
    end if;
  end process;

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        rcdw_count <= "00";
      elsif current_state = ACTIVE then
        rcdw_count <= "10";
      elsif rcdw_count /= "01" then
        rcdw_count <= rcdw_count - "01";
      else
        rcdw_count <= rcdw_count;
      end if;
    end if;
  end process;

--******************************************************************************
-- WR Counter
-- a PRECHARGE command can be applied only after 3 cycles after a WRITE
-- command has finished executing
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if (rst180_r = '1')then
        wr_count <= "00";
      elsif dqs_enable_int = '1' then
        wr_count <= "11";
      elsif wr_count /= "00" then
        wr_count <= wr_count - "01";
      else
        wr_count <= wr_count;
      end if;
    end if;
  end process;

--******************************************************************************
-- autoref_count
-- The DDR SDRAM requires AUTO REFRESH cycles at an average interval of 7.8125us.
-- MAX_REF_CNT is set to 7.7us in the parameter file depending on the selected
-- frequency. For example 166MHz frequency,
-- The autoref_count = refresh_time_period/clock_period = 7.7us/6.02ns = 1278
--******************************************************************************

  autoref_value   <= '1' when (autoref_count = MAX_REF_CNT) else '0';

  autoref_cnt_val <= "00000000000" when ((autoref_count = MAX_REF_CNT) or
                                         (init_done = '0')) else
                     autoref_count + "00000000001";

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        autoref_count <= "00000000000";
      else
        autoref_count <= autoref_cnt_val;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        auto_ref_detect1 <= '0';
        auto_ref1        <= '0';
      else
        auto_ref_detect1 <= autoref_value and init_done;
        auto_ref1        <= auto_ref_detect1;
      end if;
    end if;
  end process;

  ar_done_p <= '1' when ar_done_reg = '1' else '0';

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        auto_ref_wait   <= '0';
        ar_done         <= '0';
        auto_ref_issued <= '0';
      else
        if (auto_ref1 = '1' and auto_ref_wait = '0') then
          auto_ref_wait <= '1';
        elsif (auto_ref_issued_p = '1') then
          auto_ref_wait <= '0';
        else
          auto_ref_wait <= auto_ref_wait;
        end if;
        ar_done         <= ar_done_p;
        auto_ref_issued <= auto_ref_issued_p;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        auto_ref_wait1 <= '0';
        auto_ref_wait2 <= '0';
        auto_ref       <= '0';
      else
        if (auto_ref_issued_p = '1') then
          auto_ref_wait1 <= '0';
          auto_ref_wait2 <= '0';
          auto_ref       <= '0';
        else
          auto_ref_wait1 <= auto_ref_wait;
          auto_ref_wait2 <= auto_ref_wait1;
          auto_ref       <= auto_ref_wait2;
        end if;
      end if;
    end if;
  end process;

  auto_ref_req      <= auto_ref_wait;
  auto_ref_issued_p <= '1' when(current_state = AUTO_REFRESH) else '0';

--******************************************************************************
-- While doing consecutive READs or WRITEs, the burst_cnt_max value determines
-- when the next READ or WRITE command should be issued. burst_cnt_max shows the
-- number of clock cycles for each burst. 
-- e.g burst_cnt_max = 1 for a burst length of 2
--                   = 2 for a burst length of 4
--                   = 4 for a burst length of 8
--******************************************************************************
  burst_cnt_max <= "001" when burst_len = "001" else  
                   "010" when burst_len = "010" else  
                   "100" when burst_len = "011" else  
                   "000";


  process (clk)
  begin
    if (clk'event and clk = '0') then
      if (rst180_r = '1') then
        cas_count <= "000";
      elsif (current_state = BURST_READ) then
        cas_count <= burst_cnt_max - '1';
      elsif (cas_count /= "000") then
        cas_count <= cas_count - '1';
      end if;
    end if;
  end process;


  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        rdburst_end_1   <= '0';
      else
        if(burst_done = '1') then
          rdburst_end_1 <= '1';
        else
          rdburst_end_1 <= '0';
        end if;
      end if;
    end if;
  end process;

  rdburst_end <= rdburst_end_1;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        wrburst_end_1   <= '0';
      else
        if (burst_done = '1') then
          wrburst_end_1 <= '1';
        else
          wrburst_end_1 <= '0';
        end if;
        wrburst_end_2   <= wrburst_end_1;
      end if;
    end if;
  end process;

 wrburst_end <= wrburst_end_2 when burst_len = "011" else wrburst_end_1; 

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        wrburst_end_cnt <= "000";
      elsif((current_state = FIRST_WRITE) or (current_state = BURST_WRITE)) then
        wrburst_end_cnt <= burst_cnt_max;
      elsif(wrburst_end_cnt /= "000") then
        wrburst_end_cnt <= wrburst_end_cnt - '1';
      end if;
    end if;
  end process;

--******************************************************************************
-- dqs_enable and dqs_reset signals are used to generate DQS signal during write
-- data.
--******************************************************************************

  dqs_enable     <= dqs_enable2;
  dqs_reset      <= dqs_reset2_clk0;

  dqs_enable_int <= '1' when ((current_state = FIRST_WRITE) or
                              (current_state = BURST_WRITE) or
                              (wrburst_end_cnt /= "000")) else '0';

  dqs_reset_int  <= '1' when (current_state = FIRST_WRITE) else '0';

  process(clk)
  begin
    if(clk'event and clk = '1') then
      if rst0_r = '1' then
        dqs_enable1     <= '0';
        dqs_enable2     <= '0';
        dqs_enable3     <= '0';
        dqs_reset1_clk0 <= '0';
        dqs_reset2_clk0 <= '0';
        dqs_reset3_clk0 <= '0';
      else
        dqs_enable1     <= dqs_enable_int;
        dqs_enable2     <= dqs_enable1;
        dqs_enable3     <= dqs_enable2;
        dqs_reset1_clk0 <= dqs_reset_int;
        dqs_reset2_clk0 <= dqs_reset1_clk0;
        dqs_reset3_clk0 <= dqs_reset2_clk0;
      end if;
    end if;
  end process;

--******************************************************************************
--Write Enable signal to the datapath
--******************************************************************************

  process (clk)
  begin
    if (clk'event and clk = '0') then
      if (rst180_r = '1') then
        write_enable_r <= '0';
      elsif((current_state = FIRST_WRITE) or (current_state = BURST_WRITE)
            or (wrburst_end_cnt > "001")) then
        write_enable_r <= '1';
      else
        write_enable_r <= '0';
      end if;
    end if;
  end process;

  process (clk)
  begin
    if (clk'event and clk = '0') then
      if (rst180_r = '1') then
        write_enable_r1 <= '0';
      else
        write_enable_r1 <= write_enable_r;
      end if;
    end if;
  end process;

  write_enable <= write_enable_r;
  cmd_ack     <= ack_reg;

  ACK_REG_INST1 : FD
    port map (
      Q => ack_reg,
      D => ack_o,
      C => clk180
      );

  ack_o <= '1' when ((write_cmd_in = '1') or (write_cmd1 = '1') or
                     (read_cmd = '1') or (read_cmd1 = '1')) else '0';

--******************************************************************************
-- init_done will be asserted when initialization sequence is complete
--******************************************************************************
  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        init_memory  <= '0';
        init_done    <= '0';
        init_done_r1 <= '0';
      else
        init_memory  <= init_mem;
        init_done    <= init_done_value;
        init_done_r1 <= init_done;
      end if;
    end if;
  end process;

  init_done_dis <= '1' when ( init_done = '1' and init_done_r1 = '0') else '0';

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '0' then
        --synthesis translate_off
        assert (init_done_dis = '0') report "INITIALIZATION_DONE" severity note;
        --synthesis translate_on
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        init_mem <= '0';
      elsif initialize_memory = '1' then
        init_mem <= '1';
      elsif init_count = "111" and count5 = "00001" then
        init_mem <= '0';
      else
        init_mem <= init_mem;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        init_count <= "000";
      elsif(((init_current_state = INIT_PRECHARGE) or 
		(init_current_state = INIT_LOAD_MODE_REG) or
		(init_current_state = INIT_AUTO_REFRESH)) and init_memory = '1') then
        init_count <= init_count + '1';
      else
        init_count <= init_count;
      end if;
    end if;
  end process;
		     
  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        dll_rst_count <= "00000000";
      elsif(init_count = "010") then
        dll_rst_count <= "11001000";
      elsif(dll_rst_count /= "00000001") then
        dll_rst_count <= dll_rst_count - '1';
      else
        dll_rst_count <= dll_rst_count;
      end if;
    end if;
  end process;

  init_done_value    <= '1' when ((init_count = "111") and
                                 (dll_rst_count = "00000001")) else '0';

  go_to_active_value <= '1' when (((write_cmd_in = '1') and (accept_cmd_in = '1'))
                                  or ((read_cmd = '1') and (accept_cmd_in = '1')))
				  else '0';

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        go_to_active <= '0';
      else
        go_to_active <= go_to_active_value;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        rfc_count_reg <= '0';
        ar_done_reg   <= '0';
      else
        if(rfc_count = "000010") then
          ar_done_reg <= '1';
        else
          ar_done_reg <= '0';
        end if;
        if(ar_done_reg = '1') then
          rfc_count_reg <= '1';
        elsif(init_done = '1' and init_mem = '0' and rfc_count = "000000") then
          rfc_count_reg <= '1';
        elsif (auto_ref_issued = '1') then
          rfc_count_reg <= '0';
        else
          rfc_count_reg <= rfc_count_reg;
        end if;
      end if;
    end if;
  end process;

--******************************************************************************
-- Initialization state machine
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        init_current_state <= INIT_IDLE;
      else
        init_current_state <= init_next_state;
      end if;
    end if;
  end process;

  process(rst180_r, init_memory, init_count, count5, dll_rst_count, init_current_state)
  begin
    if rst180_r = '1' or init_memory = '0' then
      init_next_state <= INIT_IDLE;
    else
      case init_current_state is
        when INIT_IDLE          =>
          if init_memory = '1' then
            case init_count is
              when "000"        =>
                init_next_state <= INIT_PRECHARGE;
              when "001"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_LOAD_MODE_REG;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when "010"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_LOAD_MODE_REG;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when "011"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_PRECHARGE;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when "100"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_AUTO_REFRESH;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when "101"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_AUTO_REFRESH;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when "110"        =>
                if count5 = "00001" then
                  init_next_state <= INIT_LOAD_MODE_REG;
                else
                  init_next_state <= INIT_IDLE;
                end if;
              when others       =>
                init_next_state <= INIT_IDLE;
            end case;
          else
            init_next_state <= INIT_IDLE;
          end if;
        when INIT_PRECHARGE     =>
          init_next_state <= INIT_IDLE;
        when INIT_LOAD_MODE_REG =>
          init_next_state <= INIT_IDLE;
        when INIT_AUTO_REFRESH  =>
          init_next_state <= INIT_IDLE;
        when others             =>
          init_next_state <= INIT_IDLE;
      end case;
    end if;
  end process;

--******************************************************************************
-- Main state machine
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        current_state <= IDLE;
      else
        current_state <= next_state;
      end if;
    end if;
  end process;

  process(rst180_r, rp_count, rfc_count, cas_count, wr_count,
          go_to_active, write_cmd1, read_cmd1,
	  rcdw_count, rcdr_count, burst_len, wrburst_end , wrburst_end_cnt, 
          rdburst_end, rfc_count_reg, auto_ref, current_state, init_done)
  begin
    if rst180_r = '1' then
      next_state <= IDLE;
    elsif init_done = '1' then
      next_state <= IDLE;
      case current_state is
        when IDLE =>
          if(auto_ref = '1' and rfc_count_reg = '1' and rp_count = "000") then
            next_state <= AUTO_REFRESH;
          elsif go_to_active = '1' then
            next_state <= ACTIVE;
          else
            next_state <= IDLE;
          end if;
        when PRECHARGE =>
          next_state   <= PRECHARGE_WAIT;
        when PRECHARGE_WAIT =>
          if rp_count = "000" then
            next_state <= IDLE;
          else
            next_state <= PRECHARGE_WAIT;
          end if;
        when AUTO_REFRESH =>
          next_state   <= AUTO_REFRESH_WAIT;
        when AUTO_REFRESH_WAIT =>
          if rfc_count = "000000" then
            next_state <= IDLE;
          else
            next_state <= AUTO_REFRESH_WAIT;
          end if;
        when ACTIVE =>
          next_state   <= ACTIVE_WAIT;
        when ACTIVE_WAIT =>
          if (rcdw_count = "01" and write_cmd1 = '1') then
            next_state <= FIRST_WRITE;
          elsif (rcdr_count = "01" and read_cmd1 = '1') then
            next_state <= BURST_READ;
          else
            next_state <= ACTIVE_WAIT;
          end if;
        when FIRST_WRITE =>
          -- To meet the write preamble
          if ((burst_len = "001") and (wrburst_end = '0')) then
            next_state <= BURST_WRITE;
          elsif ((burst_len = "001") and (wrburst_end = '1')) then
            next_state <= PRECHARGE_AFTER_WRITE;
          else
            next_state <= WRITE_WAIT;
          end if;
        when WRITE_WAIT =>
          case wrburst_end is
            when '1' =>
              next_state   <= PRECHARGE_AFTER_WRITE;
            when '0' =>
              if wrburst_end_cnt = "010" then
                next_state <= BURST_WRITE;
              else
                next_state <= WRITE_WAIT;
              end if;
            when others =>
              next_state   <= WRITE_WAIT;
          end case;
        when BURST_WRITE =>
          if ((burst_len = "001") and (wrburst_end = '0')) then
            next_state <= BURST_WRITE;
          elsif ((burst_len = "001") and (wrburst_end = '1')) then
            next_state <= PRECHARGE_AFTER_WRITE;
          else
            next_state <= WRITE_WAIT;
          end if;
        when PRECHARGE_AFTER_WRITE =>
          next_state <= PRECHARGE_AFTER_WRITE_2;
        when PRECHARGE_AFTER_WRITE_2 =>
          if(wr_count = "00") then
            next_state <= PRECHARGE;
          else
            next_state <= PRECHARGE_AFTER_WRITE_2;
          end if;
        when READ_WAIT =>
          case rdburst_end is
            when '1' =>
              next_state   <= PRECHARGE_AFTER_WRITE;
            when '0' =>
              if cas_count = "001" then
                next_state <= BURST_READ;
              else
                next_state <= READ_WAIT;
              end if;
            when others =>
              next_state   <= READ_WAIT;
          end case;
        when BURST_READ =>
          if ((burst_len = "001") and (rdburst_end = '0')) then
            next_state <= BURST_READ;
          elsif ((burst_len = "001") and (rdburst_end = '1')) then
            next_state <= PRECHARGE_AFTER_WRITE;
          else
            next_state <= READ_WAIT;
          end if;
        when others =>
          next_state <= IDLE;
      end case;
    else
      next_state <= IDLE;
    end if;
  end process;

--******************************************************************************
-- Address generation logic
--******************************************************************************

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        emr_1       <= (others => '0');
        lmr_dll_rst <= (others => '0');
        lmr_dll_set <= (others => '0');
      else
        lmr_dll_rst <= lmr(ROW_ADDRESS -1 downto 9) & '1' & lmr(7 downto 0);
        lmr_dll_set <= lmr(ROW_ADDRESS -1 downto 9) & '0' & lmr(7 downto 0);
        emr_1       <= emr;
      end if;
    end if;
  end process;

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        ddr_address1     <= (others => '0');
      elsif (init_memory = '1') then
        case init_count is
          when "000" | "011" =>
            ddr_address1 <= low(ROW_ADDRESS - 12 downto 0) & "10000000000";
          when "001" =>
            ddr_address1 <= emr_1;
          when "010" =>
            ddr_address1 <= lmr_dll_rst;
          when "110" =>
            ddr_address1 <= lmr_dll_set;
          when others =>
            ddr_address1 <= low(ROW_ADDRESS - 1 downto 0);
        end case;
      elsif(current_state = PRECHARGE) then
        ddr_address1 <= low(ROW_ADDRESS - 12 downto 0) & "10000000000";
      elsif(current_state = ACTIVE) then
        ddr_address1 <= row_address_reg;
      elsif(current_state = FIRST_WRITE or current_state = BURST_WRITE or
            current_state = BURST_READ) then
        ddr_address1 <= column_address_reg;
      else
        ddr_address1 <= low(ROW_ADDRESS - 1 downto 0);
      end if;
    end if;
  end process;

  process (clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        ddr_ba1 <= (others => '0');
      elsif((init_count = "001") )then
        ddr_ba1 <= low(BANK_ADDRESS-2 downto 0) & '1';
      elsif(current_state = ACTIVE or current_state = FIRST_WRITE
            or current_state = BURST_WRITE or current_state = BURST_READ) then
        ddr_ba1 <= ba_address_reg2;
      else
        ddr_ba1 <= (others => '0');
      end if;
    end if;
  end process;

--******************************************************************************
-- control signals to the Memory
--******************************************************************************

  ddr_rasb1 <= '0' when (init_current_state = INIT_PRECHARGE or init_current_state =
                         INIT_LOAD_MODE_REG or init_current_state = INIT_AUTO_REFRESH or
                         current_state = ACTIVE or current_state = PRECHARGE or
                         current_state = AUTO_REFRESH ) else '1';

  ddr_casb1 <= '0' when (init_current_state = INIT_LOAD_MODE_REG or init_current_state =
                         INIT_AUTO_REFRESH or current_state = BURST_READ or
                         current_state = BURST_WRITE or current_state = FIRST_WRITE or
                         current_state = AUTO_REFRESH) else '1';

  ddr_web1  <= '0' when (init_current_state = INIT_PRECHARGE or init_current_state =
                         INIT_LOAD_MODE_REG or current_state = BURST_WRITE or
                         current_state = FIRST_WRITE or
                         current_state = PRECHARGE) else '1';

--******************************************************************************
-- Register CONTROL SIGNALS outputs
--******************************************************************************
  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        ddr_rasb2 <= '1';
        ddr_casb2 <= '1';
        ddr_web2  <= '1';
      else
        ddr_rasb2 <= ddr_rasb1;
        ddr_casb2 <= ddr_casb1;
        ddr_web2  <= ddr_web1;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        ddr_rst_dqs_rasb4 <= '1';
        ddr_rst_dqs_casb4 <= '1';
        ddr_rst_dqs_web4  <= '1';
      else
        if(cas_latency = "011") then
          ddr_rst_dqs_rasb4 <= ddr_rasb1;
          ddr_rst_dqs_casb4 <= ddr_casb1;
          ddr_rst_dqs_web4  <= ddr_web1;
        else
          ddr_rst_dqs_rasb4 <= ddr_rst_dqs_rasb4;
          ddr_rst_dqs_casb4 <= ddr_rst_dqs_casb4;
          ddr_rst_dqs_web4  <= ddr_rst_dqs_web4;
        end if;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        dqs_div_cascount     <= "000";
      else
        if(ddr_rasb1 = '1' and ddr_casb1 = '0' and ddr_web1 = '1' and
           (cas_latency = "110" or cas_latency = "010")) then
          dqs_div_cascount   <= burst_cnt_max;
        elsif(ddr_rst_dqs_rasb4 = '1' and ddr_rst_dqs_casb4 = '0' and
              ddr_rst_dqs_web4 = '1') then
          dqs_div_cascount   <= burst_cnt_max;
        else
          if dqs_div_cascount /= "000" then
            dqs_div_cascount <= dqs_div_cascount - "001";
          else
            dqs_div_cascount <= dqs_div_cascount;
          end if;
        end if;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        dqs_div_rdburstcount <= "000";
      else
        if (dqs_div_cascount = "001" and burst_len = "010") then  
          dqs_div_rdburstcount <= "010";                             
        elsif (dqs_div_cascount = "011" and burst_len = "011") then 
          dqs_div_rdburstcount <= "100";                             
        elsif (dqs_div_cascount = "001" and burst_len = "001") then 
          dqs_div_rdburstcount <= "001";
        else
          if dqs_div_rdburstcount /= "000" then
            dqs_div_rdburstcount <= dqs_div_rdburstcount - "001";
          else
            dqs_div_rdburstcount <= dqs_div_rdburstcount;
          end if;
        end if;
      end if;
    end if;
  end process;

  process(clk)
  begin
    if(clk'event and clk = '0') then
      if rst180_r = '1' then
        rst_dqs_div_r <= '0';
      else
        if (dqs_div_cascount = "001" and burst_len = "010")then   
          rst_dqs_div_r <= '1';
        elsif (dqs_div_cascount = "011" and burst_len = "011")then
          rst_dqs_div_r <= '1';
        elsif (dqs_div_cascount = "001" and burst_len = "001")then
          rst_dqs_div_r <= '1';
        else
          if (dqs_div_rdburstcount = "001" and dqs_div_cascount = "000") then
            rst_dqs_div_r <= '0';
          else
            rst_dqs_div_r <= rst_dqs_div_r;
          end if;
        end if;
      end if;
    end if;
  end process;

   BL_INST1 : if(BURST_LEN_VAL = "001") generate
  begin
    rst_dqs_div_d <= rst_dqs_div_r1; 
  end generate BL_INST1;

  BL_INST2 : if(BURST_LEN_VAL = "010" or BURST_LEN_VAL = "011") generate
  begin
    rst_dqs_div_d <= rst_dqs_div_r; 
  end generate BL_INST2;

  process(clk)
  begin
    if clk'event and clk = '0' then
      rst_dqs_div_r1 <= rst_dqs_div_r;
      rst_dqs_div_r2 <= rst_dqs_div_d;
    end if;
  end process;
--Read fifo read enable logic, this signal is delayed one clock wrt 
-- rst_dqs_div_int signal. 
  process(clk)
  begin
    if clk'event and clk = '0' then
      read_fifo_rden <= rst_dqs_div_r2;
    end if;
  end process;


  process (clk)
  begin
    if (clk'event and clk = '0') then
      if (dqs_div_cascount /= "000" or dqs_div_rdburstcount /= "000") then
        rst_calib <= '1';
      else
        rst_calib <= '0';
      end if;
    end if;
  end process;

  CL_INST1 : if(CAS_LAT_VAL = "110") generate
    attribute IOB of rst_iob_out         : label is "true";
    attribute syn_useioff of rst_iob_out : label is true;
  begin
    rst_iob_out : FD
      port map (
        Q => rst_dqs_div_int,
        D => rst_dqs_div_d,
        C => clk180
        );
  end generate CL_INST1;
  
  CL_INST2 : if(CAS_LAT_VAL = "011" or CAS_LAT_VAL = "010") generate
    attribute IOB of rst_iob_out         : label is "true";
    attribute syn_useioff of rst_iob_out : label is true;
  begin
    rst_iob_out : FD
      port map (
        Q => rst_dqs_div_int,
        D => rst_dqs_div_d,
        C => clk
        );
  end generate CL_INST2;



end arc;
