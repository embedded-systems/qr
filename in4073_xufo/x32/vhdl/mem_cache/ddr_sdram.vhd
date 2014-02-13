-------------------------------------------------------------------------------
-- Filename:             ddr_sdram.vhd
-- Entity:               ddr_sdram
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          This module integrates the cache, the
--                       cache controller, the TLB, the sdram
--                       controller and the sdram interface.
--                       Inputs port is the X32 memory bus, and
--                       the output is the DDR SDRAM signals.
--                       
--
-- 	Copyright (c) 2005-2007, Software Technology Department, TU Delft
--	All rights reserved.
--
-- 	This program is free software; you can redistribute it and/or
--	modify it under the terms of the GNU General Public License
--	as published by the Free Software Foundation; either version 2
--	of the License, or (at your option) any later version.
--	
--	This program is distributed in the hope that it will be useful,
-- 	but WITHOUT ANY WARRANTY; without even the implied warranty of
--	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--	GNU General Public License for more details.
--	
--	You should have received a copy of the GNU General Public License
--	along with this program; if not, write to the Free Software
-- 	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
--  02111-1307, USA.
--	
--  See the GNU General Public License here:
--
--  http://www.gnu.org/copyleft/gpl.html#SEC1
-------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use work.cache_parameters.ALL;


entity ddr_sdram is
	
	Generic(
		-- TLB init tag.
		address_tag_init	:	std_logic_vector( CACHE_TAG_BITS - 1 downto 0) := (others => '0')
	);
	
	Port(
	
			clk_o			: out  STD_LOGIC;
			clk90_o			: out  STD_LOGIC;
			clk_50MHz_o		: out  STD_LOGIC;
			reset_o			: out  STD_LOGIC;
			reset90_o		: out  STD_LOGIC;
			reset180_o		: out  STD_LOGIC;
	
			-- SDRAM Signals:
			cntrl0_ddr_dq                 : inout std_logic_vector(15 downto 0);
			cntrl0_ddr_a                  : out   std_logic_vector(12 downto 0);
			cntrl0_ddr_ba                 : out   std_logic_vector(1 downto 0);
			cntrl0_ddr_cke                : out   std_logic;
			cntrl0_ddr_cs_n               : out   std_logic;
			cntrl0_ddr_ras_n              : out   std_logic;
			cntrl0_ddr_cas_n              : out   std_logic;
			cntrl0_ddr_we_n               : out   std_logic;
			cntrl0_ddr_dm                 : out   std_logic_vector(1 downto 0);
			cntrl0_rst_dqs_div_in         : in    std_logic;
			cntrl0_rst_dqs_div_out        : out   std_logic;
			sys_clk_in                    : in    std_logic;
			reset_in_n                    : in    std_logic;
			cntrl0_led_error_output1      : out   std_logic;
			cntrl0_init_done              : out   std_logic;
			cntrl0_ddr_dqs                : inout std_logic_vector(1 downto 0);
			cntrl0_ddr_ck                 : out   std_logic_vector(0 downto 0);
			cntrl0_ddr_ck_n               : out   std_logic_vector(0 downto 0);
			
			-- X32 bus signals
			mem_address                 : in    std_logic_vector(PHY_MEM_ADDR_BITS - 1 downto 0);
			-- data to memory
			mem_data_in                 : in    std_logic_vector(31 downto 0);
			-- data from memory
			mem_data_out                : out   std_logic_vector(31 downto 0);
			-- size of data (see types.vhd)
			mem_data_size               : in    std_logic_vector(2 downto 0);
			--  data signedness (1=signed, 0=unsigned)
			mem_data_signed             : in    std_logic;
			-- start read operation (high = start read)
			mem_read                    : in    std_logic;
			-- start write operation (high = start write)
			mem_write                   : in    std_logic;
			-- pulses high when the operation finishes
			mem_ready                   : out   std_logic;
			-- Overflow
			overflow                    : out   std_logic;
			
			-- Cache miss ( debugging purposes ).
			cache_miss_counter          : out   std_logic_vector(31 downto 0)

);

end ddr_sdram;

architecture Behavioral of ddr_sdram is

component mig_23
  port (
  
      cntrl0_ddr_dq                 : inout std_logic_vector(15 downto 0);
      cntrl0_ddr_a                  : out   std_logic_vector(12 downto 0);
      cntrl0_ddr_ba                 : out   std_logic_vector(1 downto 0);
      cntrl0_ddr_cke                : out   std_logic;
      cntrl0_ddr_cs_n               : out   std_logic;
      cntrl0_ddr_ras_n              : out   std_logic;
      cntrl0_ddr_cas_n              : out   std_logic;
      cntrl0_ddr_we_n               : out   std_logic;
      cntrl0_ddr_dm                 : out   std_logic_vector(1 downto 0);
      cntrl0_rst_dqs_div_in         : in    std_logic;
      cntrl0_rst_dqs_div_out        : out   std_logic;
      sys_clk_in                    : in    std_logic;
      reset_in_n                    : in    std_logic;
      cntrl0_burst_done             : in    std_logic;
      cntrl0_init_val               : out   std_logic;
      cntrl0_ar_done                : out   std_logic;
      cntrl0_user_data_valid        : out   std_logic;
      cntrl0_auto_ref_req           : out   std_logic;
      cntrl0_user_cmd_ack           : out   std_logic;
      cntrl0_user_command_register  : in    std_logic_vector(2 downto 0);
      clk50MHz_o                    : out   std_logic;
      cntrl0_clk_tb                 : out   std_logic;
      cntrl0_clk90_tb               : out   std_logic;
      cntrl0_sys_rst_tb             : out   std_logic;
      cntrl0_sys_rst90_tb           : out   std_logic;
      cntrl0_sys_rst180_tb          : out   std_logic;
      cntrl0_user_data_mask         : in    std_logic_vector(3 downto 0);
      cntrl0_user_output_data       : out   std_logic_vector(31 downto 0);
      cntrl0_user_input_data        : in    std_logic_vector(31 downto 0);
      cntrl0_user_input_address     : in    std_logic_vector(24 downto 0);
      cntrl0_ddr_dqs                : inout std_logic_vector(1 downto 0);
      cntrl0_ddr_ck                 : out   std_logic_vector(0 downto 0);
      cntrl0_ddr_ck_n               : out   std_logic_vector(0 downto 0)

    );
end component;



component sdram_controller
    Port ( 
           clk              : in   STD_LOGIC;
           clk90            : in   STD_LOGIC;
           reset            : in   STD_LOGIC;
           reset90          : in   STD_LOGIC;
			  
           ddr_burst_done       	: out  STD_LOGIC;
           ddr_init_val         	: in   STD_LOGIC;
           ddr_ar_done          	: in   STD_LOGIC;
           ddr_data_valid       	: in   STD_LOGIC;
           ddr_auto_ref_req     	: in   STD_LOGIC;
           ddr_cmd_ack          	: in   STD_LOGIC;
           ddr_command_register 	: out  STD_LOGIC_VECTOR (2  downto 0);
           ddr_data_mask    		: out  STD_LOGIC_VECTOR (3  downto 0);
           ddr_data_out     		: in   STD_LOGIC_VECTOR (31 downto 0);
           ddr_data_in   	 		: out  STD_LOGIC_VECTOR (31 downto 0);
           ddr_address      		: out  STD_LOGIC_VECTOR (DDR_ADDR_BITS - 1 downto 0);
           
			  cache_request			: in  std_logic;
			  cache_block_request	: in  std_logic_vector(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);
			  cache_ready			: out std_logic;
			  cache_address			: out std_logic_vector( CACHE_ADDR_BITS - 1 downto 0);
			  cache_data_in			: out std_logic_vector( 31 downto 0);
			  cache_data_out		: in  std_logic_vector( 31 downto 0);
			  cache_enable			: out std_logic;
			  cache_write_enable	: out std_logic;
			
			  tlb_set_address		: out std_logic_vector( CACHE_SET_BITS - 1 downto 0);
			  tlb_tag_in			: out std_logic_vector( CACHE_TAG_BITS downto 0);
			  tlb_tag_out			: in  std_logic_vector( CACHE_TAG_BITS downto 0);
			  tlb_enable			: out std_logic;
			  tlb_write				: out std_logic

		);
end component;


component mem_cache

	Port(
		clockA 			: in  std_logic;
		enableA			: in 	std_logic;
		write_enableA	: in	std_logic;
		addressA		: in	std_logic_vector(CACHE_SET_BITS + CACHE_BLOCK_BITS - 1 downto 0);
		data_maskA		: in	std_logic_vector(3 downto 0);
		data_inA		: in	std_logic_vector(31 downto 0);
		data_outA		: out	std_logic_vector(31 downto 0);
		
		clockB 			: in  std_logic;
		enableB			: in 	std_logic;
		write_enableB	: in	std_logic;
		addressB		: in	std_logic_vector(CACHE_SET_BITS + CACHE_BLOCK_BITS - 1 downto 0);
		data_inB		: in	std_logic_vector(31 downto 0);
		data_outB		: out	std_logic_vector(31 downto 0)
		
		);
		
end component;


component cache_controller

  port
    (
        clk                         : in    std_logic;
        reset                       : in    std_logic;
        
        mem_address                 : in    std_logic_vector(PHY_MEM_ADDR_BITS - 1 downto 0);
        mem_data_in                 : in    std_logic_vector(31 downto 0);
        mem_data_out                : out   std_logic_vector(31 downto 0);
        mem_data_size               : in    std_logic_vector(2 downto 0);
        mem_data_signed             : in    std_logic;
        mem_read                    : in    std_logic;
        mem_write                   : in    std_logic;
        mem_ready                   : out   std_logic;
        overflow                    : out   std_logic;

		cache_request				: out std_logic;
		cache_block_request			: out std_logic_vector(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);
		cache_ready					: in	std_logic;


		cache_enable  				: out	STD_LOGIC;
		cache_write_enable			: out	STD_LOGIC;
		cache_data_mask 			: out	STD_LOGIC_VECTOR (3 downto 0);
		cache_address    			: out	STD_LOGIC_VECTOR (CACHE_ADDR_BITS - 1 downto 0);			  
		cache_data_in             	: out	STD_LOGIC_VECTOR (31 downto 0);
		cache_data_out            	: in	STD_LOGIC_VECTOR (31 downto 0);
			
			
		tlb_set_address			: out	std_logic_vector( CACHE_SET_BITS - 1 downto 0);
		tlb_tag_in				: out	std_logic_vector( CACHE_TAG_BITS downto 0);
		tlb_tag_out				: in	std_logic_vector( CACHE_TAG_BITS downto 0);
		tlb_enable				: out	std_logic;
		tlb_write_enable		: out	std_logic

	);

end component;


-- Clock / Reset signals
signal	clk              	:	STD_LOGIC;
signal	clk90            	:	STD_LOGIC;
signal	clk50MHz			:	STD_LOGIC;
signal	reset            	:	STD_LOGIC;
signal	reset90          	:	STD_LOGIC;
signal	reset180         	:	STD_LOGIC;
  
  
-- SDRAM interface to SDRAM controller signals.
signal	ddr_burst_done       :   STD_LOGIC;
signal	ddr_init_val         :   STD_LOGIC;
signal	ddr_ar_done          :   STD_LOGIC;
signal	ddr_data_valid       :   STD_LOGIC;
signal	ddr_auto_ref_req     :   STD_LOGIC;
signal	ddr_cmd_ack          :   STD_LOGIC;
signal	ddr_command_register :   STD_LOGIC_VECTOR (2  downto 0);
signal	ddr_data_mask        :   STD_LOGIC_VECTOR (3  downto 0);
signal	ddr_data_out         :   STD_LOGIC_VECTOR (31 downto 0);
signal	ddr_data_in          :   STD_LOGIC_VECTOR (31 downto 0);
signal	ddr_address          :   STD_LOGIC_VECTOR (DDR_ADDR_BITS - 1 downto 0);


-- Cache signals to SDRAM controller signals:
signal	cache_request			: std_logic;
signal	cache_block_request		: std_logic_vector( MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);
signal	cache_ready				: std_logic;
signal	cache_address			: std_logic_vector( CACHE_ADDR_BITS - 1 downto 0);
signal	cache_data_in			: std_logic_vector( 31 downto 0);
signal	cache_data_out			: std_logic_vector( 31 downto 0);
signal	cache_enable			: std_logic;
signal	cache_write_enable		: std_logic;

-- TLB to SDRAM controller signals:
signal	tlb_set_address			: std_logic_vector( CACHE_SET_BITS - 1 downto 0);
signal	tlb_tag_in				: std_logic_vector( CACHE_TAG_BITS downto 0);
signal	tlb_tag_out				: std_logic_vector( CACHE_TAG_BITS downto 0);
signal	tlb_enable				: std_logic;
signal	tlb_write				: std_logic;

-- Cache controller to cache
signal	mem_cache_enable  		:	STD_LOGIC;
signal	mem_cache_write_enable	:	STD_LOGIC;
signal	mem_cache_data_mask 	:	STD_LOGIC_VECTOR (3 downto 0);
signal	mem_cache_address    	:	STD_LOGIC_VECTOR ( CACHE_ADDR_BITS - 1 downto 0);			  
signal  mem_cache_data_in       :	STD_LOGIC_VECTOR (31 downto 0);
signal  mem_cache_data_out      :	STD_LOGIC_VECTOR (31 downto 0);

-- Cache controller to TLB
signal	mem_tlb_set_address		: std_logic_vector( CACHE_SET_BITS - 1 downto 0);
signal	mem_tlb_tag_in			: std_logic_vector( CACHE_TAG_BITS downto 0);
signal	mem_tlb_tag_out			: std_logic_vector( CACHE_TAG_BITS downto 0);
signal	mem_tlb_enable			: std_logic;
signal	mem_tlb_write			: std_logic;

signal cache_request_last 		: std_logic;
signal cache_miss_cntr			: std_logic_vector(31 downto 0);

begin

	clk_o  		<= clk;
	clk90_o  	<= clk90;
	clk_50MHz_o <= clk50MHz;
	reset_o  	<= reset;
	reset90_o  	<= reset90;
	reset180_o  <= reset180;

	cntrl0_init_done <= ddr_init_val;


	cache_miss_counter <= cache_miss_cntr;
	process(clk) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				cache_miss_cntr <= (others => '0');
			elsif (cache_request = '1' and cache_request_last = '0') then
				cache_miss_cntr <= cache_miss_cntr + 1;
			end if;
			cache_request_last <= cache_request;
		end if;
	end process;

	I_cache_controller: cache_controller PORT MAP(
		clk 					=> clk50MHz,
		reset 				=> reset,
		mem_address 		=> mem_address,
		mem_data_in 		=> mem_data_in,
		mem_data_out		=> mem_data_out,
		mem_data_size 		=> mem_data_size,
		mem_data_signed 	=> mem_data_signed,
		mem_read 			=> mem_read,
		mem_write 			=> mem_write,
		mem_ready 			=> mem_ready,
		overflow 			=> overflow,
		
		cache_request			=> cache_request,
		cache_block_request		=> cache_block_request,
		cache_ready				=> cache_ready,

		
		cache_enable 			=> mem_cache_enable,
		cache_write_enable 		=> mem_cache_write_enable,
		cache_data_mask 		=> mem_cache_data_mask,
		cache_address 			=> mem_cache_address,
		cache_data_in 			=> mem_cache_data_in,
		cache_data_out 			=> mem_cache_data_out,
		
		tlb_enable			=> mem_tlb_enable,
		tlb_write_enable	=> mem_tlb_write,
		tlb_set_address		=> mem_tlb_set_address,
		tlb_tag_in			=> mem_tlb_tag_in,
		tlb_tag_out			=> mem_tlb_tag_out
		
	);


	I_sdram_controller: sdram_controller PORT MAP(
	
		clk 		=> clk,
		clk90		=> clk90,
		reset 		=> reset,
		reset90 	=> reset90,
		
		ddr_burst_done 			=> ddr_burst_done,
		ddr_init_val 			=> ddr_init_val,
		ddr_ar_done 			=> ddr_ar_done,
		ddr_data_valid 			=> ddr_data_valid,
		ddr_auto_ref_req	 	=> ddr_auto_ref_req,
		ddr_cmd_ack 			=> ddr_cmd_ack,
		ddr_command_register 	=> ddr_command_register,
		ddr_data_mask 			=> ddr_data_mask,
		ddr_data_out 			=> ddr_data_out,
		ddr_data_in 			=> ddr_data_in,
		ddr_address 			=> ddr_address,
		
		cache_request			=> cache_request,	
		cache_block_request 	=> cache_block_request,	
		cache_ready				=> cache_ready,
		cache_address			=> cache_address,
		cache_data_in			=> cache_data_in,
		cache_data_out			=> cache_data_out,
		cache_enable			=> cache_enable,
		cache_write_enable		=> cache_write_enable,
			
		tlb_set_address			=> tlb_set_address,
		tlb_tag_in				=> tlb_tag_in,
		tlb_tag_out				=> tlb_tag_out,
		tlb_enable				=> tlb_enable,
		tlb_write				=> tlb_write

	);

	I_sdram_interface: mig_23 PORT MAP(
	
		cntrl0_ddr_dq 			=> cntrl0_ddr_dq,
		cntrl0_ddr_a 			=> cntrl0_ddr_a,
		cntrl0_ddr_ba 			=> cntrl0_ddr_ba,
		cntrl0_ddr_cke 			=> cntrl0_ddr_cke,
		cntrl0_ddr_cs_n 		=> cntrl0_ddr_cs_n,
		cntrl0_ddr_ras_n 		=> cntrl0_ddr_ras_n,
		cntrl0_ddr_cas_n 		=> cntrl0_ddr_cas_n,
		cntrl0_ddr_we_n 		=> cntrl0_ddr_we_n,
		cntrl0_ddr_dm 			=> cntrl0_ddr_dm,
		cntrl0_rst_dqs_div_in 	=> cntrl0_rst_dqs_div_in,
		cntrl0_rst_dqs_div_out 	=> cntrl0_rst_dqs_div_out,
		
		sys_clk_in 		=> sys_clk_in,
		reset_in_n 		=> reset_in_n,
		
		cntrl0_burst_done 				=> ddr_burst_done,
		cntrl0_init_val 				=> ddr_init_val,
		cntrl0_ar_done 					=> ddr_ar_done,
		cntrl0_user_data_valid 			=> ddr_data_valid,
		cntrl0_auto_ref_req 			=> ddr_auto_ref_req,
		cntrl0_user_cmd_ack 			=> ddr_cmd_ack,
		cntrl0_user_command_register 	=> ddr_command_register,
		
		clk50MHz_o 				=> clk50MHz,
		cntrl0_clk_tb 			=> clk,
		cntrl0_clk90_tb 		=> clk90,
		cntrl0_sys_rst_tb 		=> reset,
		cntrl0_sys_rst90_tb 	=> reset90,
		cntrl0_sys_rst180_tb 	=> reset180,
		
		cntrl0_user_data_mask 		=> ddr_data_mask,
		cntrl0_user_output_data 	=> ddr_data_out,
		cntrl0_user_input_data 		=> ddr_data_in,
		cntrl0_user_input_address 	=> ddr_address,
		
		cntrl0_ddr_dqs 	=> cntrl0_ddr_dqs,
		cntrl0_ddr_ck 	=> cntrl0_ddr_ck,
		cntrl0_ddr_ck_n => cntrl0_ddr_ck_n
	);

	cntrl0_led_error_output1 <= ddr_cmd_ack;

	cache : mem_cache
	Port map(
		clockA 			=> clk50MHz,
		enableA			=> mem_cache_enable,
		write_enableA	=> mem_cache_write_enable,
		addressA		=> mem_cache_address,
		data_maskA		=> mem_cache_data_mask,
		data_inA		=> mem_cache_data_in,
		data_outA		=> mem_cache_data_out,
		
		clockB 			=> clk90,
		enableB			=> cache_enable,
		write_enableB	=> cache_write_enable,
		addressB		=> cache_address,
		data_inB		=> cache_data_in,
		data_outB		=> cache_data_out
		
		);

	I_tlb : entity work.tlb(Behavioral)
	Generic map(
		tlb_tag_init	=> address_tag_init
	)
	Port map(
		clockA 			=> clk50MHz,
		enableA			=> mem_tlb_enable,
		write_enableA	=> mem_tlb_write,
		addressA		=> mem_tlb_set_address,
		data_inA		=> mem_tlb_tag_in,
		data_outA		=> mem_tlb_tag_out,
		
		clockB 			=> clk,
		enableB			=> tlb_enable,
		write_enableB	=> tlb_write,
		addressB		=> tlb_set_address,
		data_inB		=> tlb_tag_in,
		data_outB		=> tlb_tag_out
	);


end Behavioral;

