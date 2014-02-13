-------------------------------------------------------------------------------
-- Filename:             sdram_controller.vhd
-- Entity:               sdram_controller
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          SDRAM controller module. This manages the
--                       communication between the cache and the
--                       SDRAM interface. When the cache controller
--                       wants to access a block that is not in the
--                       cache, this module replaces the corresponding
--                       cache block with the correct one, and updates
--                       the TLB.
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


entity sdram_controller is
    Port ( 
			clk              : in   STD_LOGIC;
			clk90            : in   STD_LOGIC;
			reset            : in   STD_LOGIC;
			reset90          : in   STD_LOGIC;
			
			-- From/To DDR Interface.
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
			ddr_address      		: out  STD_LOGIC_VECTOR (24 downto 0);
           
			-- From/To Cache:
			  
			-- Asserted when a cache miss occurs.  
			cache_request 			: in  std_logic;
			-- Address of the block requested.
			cache_block_request		: in  std_logic_vector(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);  
			-- Controller asserts when the block is loaded in the cache.
			cache_ready				: out std_logic;
			  
			-- Signals for the Cache BRAM.
			cache_address			: out std_logic_vector( CACHE_ADDR_BITS - 1 downto 0);
			cache_data_in			: out std_logic_vector( 31 downto 0);
			cache_data_out			: in  std_logic_vector( 31 downto 0);
			cache_enable			: out std_logic;
			cache_write_enable		: out std_logic;
			
			-- Signals for the TLB
			tlb_set_address			: out std_logic_vector( CACHE_SET_BITS - 1 downto 0);
			tlb_tag_in				: out std_logic_vector( CACHE_TAG_BITS downto 0);
			tlb_tag_out				: in  std_logic_vector( CACHE_TAG_BITS downto 0);
			tlb_enable				: out std_logic;
			tlb_write				: out std_logic

		);

end sdram_controller;

architecture Behavioral of sdram_controller is


-- FSM signals
type state_t is ( 	RESET_STATE, INIT, STARTUP, IDLE, 
					GET_BLOCK_ADDR, GET_TAG,
					CACHE2SDRAM, WAIT_CACHE2SDRAM, COUNT_CACHE2SDRAM, CACHE2SDRAM_BURST_DONE, END_CACHE2SDRAM,
					SDRAM2CACHE, WAIT_SDRAM2CACHE, COUNT_SDRAM2CACHE, SDRAM2CACHE_BURST_DONE, END_SDRAM2CACHE,
					END_OP
				); 
signal  state_i, state_o : state_t := RESET_STATE; 

-- register store signals
signal store1, store_block_addr, store_tag : std_logic;
signal write1 : std_logic;


signal tag 			: std_logic_vector(CACHE_TAG_BITS - 1 downto 0);

-- address signals
signal ddr_address_tmp1, ddr_address_tmp2 : STD_LOGIC_VECTOR (DDR_ADDR_BITS - 1 downto 0);
signal block_address 	: std_logic_vector(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);
signal set_address		: std_logic_vector(CACHE_SET_BITS - 1 downto 0);

-- counter signals
signal addr_counter 		: std_logic_vector(CACHE_BLOCK_BITS - 1 downto 0);
signal data_counter 		: std_logic_vector(CACHE_BLOCK_BITS - 1 downto 0);
signal addr_counter_en 		: std_logic;
signal data_counter_en 		: std_logic;
signal addr_counter_reset 	: std_logic;
signal data_counter_reset 	: std_logic;
constant end_counter		: std_logic_vector(CACHE_BLOCK_BITS - 1 downto 0) := (others => '1');


signal 	endop, last_endop 	: 	std_logic;
signal 	data_valid 			: std_logic;
signal	reset_r				:	std_logic;
signal 	refreshing			: 	std_logic;
signal 	cache_write_enable_s :	std_logic;

begin




 	process(state_o, ddr_init_val, refreshing, ddr_cmd_ack, 
				ddr_address_tmp1, ddr_address_tmp2, tlb_tag_out,tag,
				addr_counter, block_address, set_address, cache_request) 
	
	begin
		
		case state_o is
			when RESET_STATE =>
				-- RESET
				endop <= '0';

				ddr_address 		<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';
				
				tlb_set_address<= (others => '-');
				tlb_write		<= '0';
				tlb_enable		<= '0';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';				
				store_block_addr 	<= '0';

				write1		<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';

				state_i <= INIT;
				
			when INIT =>
				-- SDRAM interface initialization.
				endop <= '0';

				ddr_address 		<= (others => '-');
				-- Init command.
				ddr_command_register 	<= "010";
				ddr_burst_done 			<= '0';
				
				tlb_set_address<= (others => '-');
				tlb_write		<= '0';
				tlb_enable		<= '0';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';				
				store_block_addr 	<= '0';
				
				write1		<= '0';
				
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				state_i <= STARTUP;

			when STARTUP =>
				-- Wait for the SDRAM interface initialization.
				endop <= '0';

				ddr_address 		<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';
				
				tlb_set_address<= (others => '-');
				tlb_write		<= '0';
				tlb_enable		<= '0';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr 	<= '0';

				write1		<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';

				if (ddr_init_val = '1') then
					state_i <= IDLE;
				else
					state_i <= STARTUP;
				end if;
				
			when IDLE =>
				-- Wait for cache request.
				endop <= '0';

				ddr_address 		<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';

				tlb_set_address<= (others => '-');
				tlb_write		<= '0';
				tlb_enable		<= '0';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';	
				-- Fetching the address. 
				store_block_addr 	<= '1';

				write1		<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				if (cache_request ='1' and refreshing = '0' and ddr_cmd_ack = '0') then
					state_i <= GET_BLOCK_ADDR;
				else
					state_i <= IDLE;
				end if;
				
			when GET_BLOCK_ADDR =>
				-- The block address is already in the register.
				-- Get the set address and request the tag from the TLB.
				endop <= '0';

				ddr_address 		<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';
		
				-- Resquest the tag from the TLB.
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '1';
				store_block_addr 	<= '0';

				write1		<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				state_i <= GET_TAG;
				
			when GET_TAG =>
			
				endop <= '0';

				ddr_address 		<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';
		
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				-- Store the tag that comes from the TLB.
				store_tag			<= '1';
				store_block_addr	<= '0';

				write1		<= '0';

				-- Reset the counters
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '1';
				data_counter_reset	<= '1';
				
				-- If the dirty bit is 1, first store the block.
				if (tlb_tag_out(0) = '1') then
					state_i <= CACHE2SDRAM;
				else -- Fetch the block from the SDRAM.
					state_i <= SDRAM2CACHE;
				end if;
				
			when CACHE2SDRAM =>
				
				-- Start storing the block in the SDRAM.
				endop <= '0';

				-- SDRAM write command.
				ddr_address 			<= ddr_address_tmp2;
				ddr_command_register 	<= "100";
				ddr_burst_done 			<= '0';
				
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				-- Write enable
				write1		<= '1';
		
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				-- Wait for command ACK.
				if (ddr_cmd_ack = '0') then
					state_i <= CACHE2SDRAM;
				else
					state_i <= WAIT_CACHE2SDRAM;
				end if;
				
			when WAIT_CACHE2SDRAM => 
				
				-- Wait one cycle after the command ACK.
				endop <= '0';

				ddr_address 			<= ddr_address_tmp2;
				ddr_command_register 	<= "100";
				ddr_burst_done 			<= '0';

				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';
	
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '1';
				
				state_i <= COUNT_CACHE2SDRAM;
				
			when COUNT_CACHE2SDRAM => 
				-- Start incrementing the counter for the data and the address.
				endop <= '0';

				ddr_address 			<= ddr_address_tmp2;
				ddr_command_register 	<= "100";
				ddr_burst_done 			<= '0';
				
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '1';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '1';
				
				-- If the counter reaches the 32 words
				-- go to the next state.
				if (addr_counter = end_counter) then
					state_i <= CACHE2SDRAM_BURST_DONE;
				--elsif (refreshing = '1')and(addr_counter(0) = '1') then
					--state_i <= CACHE2SDRAM_BURST_DONE;
				else
					state_i <= COUNT_CACHE2SDRAM;
				end if;
				
			when CACHE2SDRAM_BURST_DONE => 
				
				-- Stop the command.
				endop <= '0';

				ddr_address 				<= ddr_address_tmp2;
				ddr_command_register 	<= "100";
				-- Command finish
				ddr_burst_done 			<= '1';

				-- Clean the dirty bit of the TLB.
				tlb_set_address <= set_address;
				tlb_write		<= '1';
				tlb_enable		<= '1';
				tlb_tag_in		<= tag(CACHE_TAG_BITS - 1 downto 0) & "0";
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				write1		<= '1';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				--if (refreshing = '0') then
					state_i <= END_CACHE2SDRAM;
				--else
					--state_i <= IDLE;
				--end if;
				
			when END_CACHE2SDRAM => 
				-- Reset the counters
				-- Reset the command

				endop <= '0';

				ddr_address 				<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';

				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '1';
				data_counter_reset	<= '1';
				
				write1		<= '0';
				
				-- Wait for the command ACK				
				if ( ddr_cmd_ack = '0') then
					state_i <= SDRAM2CACHE;
				else
					state_i <= END_CACHE2SDRAM;
				end if;

			
			when SDRAM2CACHE =>
				
				-- Copy the data from the SDRAM to the cache.
				endop <= '0';

				-- Read command.
				ddr_address 			<= ddr_address_tmp1;
				ddr_command_register 	<= "110";
				ddr_burst_done 			<= '0';
				
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '0';
				
				-- Wait for the command ACK.
				if (ddr_cmd_ack = '0') then
					state_i <= SDRAM2CACHE;
				else
					state_i <= WAIT_SDRAM2CACHE;
				end if;
				
			when WAIT_SDRAM2CACHE => 
				-- Wait one cycle.
				
				endop <= '0';

				ddr_address 			<= ddr_address_tmp1;
				ddr_command_register 	<= "110";
				ddr_burst_done 			<= '0';

				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';
				
				write1		<= '0';
				
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				state_i <= COUNT_SDRAM2CACHE;
				
			when COUNT_SDRAM2CACHE => 
				-- Enable the counters.
				
				endop <= '0';

				ddr_address 			<= ddr_address_tmp1;
				ddr_command_register 	<= "110";
				ddr_burst_done 			<= '0';
				
				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '1';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '0';
				
				-- Repeat for 32 words.
				if (addr_counter = end_counter) then
					state_i <= SDRAM2CACHE_BURST_DONE;
				--elsif (refreshing = '1')and(addr_counter(0) = '1') then
					--state_i <= SDRAM2CACHE_BURST_DONE;
				else
					state_i <= COUNT_SDRAM2CACHE;
				end if;
				
			
			when SDRAM2CACHE_BURST_DONE => 
				-- End burst.
				endop <= '0';

				-- Command ends.
				ddr_address 			<= ddr_address_tmp1;
				ddr_command_register 	<= "110";
				ddr_burst_done 			<= '1';

				-- Update the TLB with the new block address.
				tlb_set_address <= set_address;
				tlb_write		<= '1';
				tlb_enable		<= '1';
				tlb_tag_in		<= block_address(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS + CACHE_SET_BITS) & "0";
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '0';
				
				--if (refreshing = '0') then
					state_i <= END_SDRAM2CACHE;
				--else
					--state_i <= IDLE;
				--end if;
				
			when END_SDRAM2CACHE => 
				-- Wait until command ACK is zero.
				
				endop <= '0';

				ddr_address 				<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';

				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				addr_counter_en		<= '0';
				addr_counter_reset 	<= '0';
				data_counter_reset	<= '0';
				
				write1		<= '0';
				
				if ( ddr_cmd_ack = '0') then
					state_i <= END_OP;
				else
					state_i <= END_SDRAM2CACHE;
				end if;
				
			when END_OP => 
				-- Assert the ready signal and wait for cache request to
				-- go to zero.

				endop <= '1';

				ddr_address 			<= (others => '-');
				ddr_command_register 	<= "000";
				ddr_burst_done 			<= '0';

				tlb_set_address <= set_address;
				tlb_write		<= '0';
				tlb_enable		<= '1';
				tlb_tag_in		<= (others => '-');
				
				store_tag			<= '0';
				store_block_addr	<= '0';

				-- Reset the counters
				addr_counter_en		<= '0';
				addr_counter_reset 	<= '1';
				data_counter_reset	<= '1';
				
				write1		<= '0';
				if ( cache_request =  '0' ) then
					state_i <= IDLE;
				else 
					state_i <= END_OP;
				end if;
	
		end case;
	end process;


	-- reset synch
	process(clk, reset) begin
		if ( clk'event and clk = '0') then
			reset_r <= reset;
		end if;
	end process;

	-- FSM register
	process(clk, reset_r, state_i) begin
		if (clk'event and clk = '0') then
			if (reset_r = '1') then
				state_o <= RESET_STATE;
			else
				state_o <= state_i;
			end if;
		end if;
	end process;


	cache_ready <= endop;

	-- refresh register.
	process(clk)
	begin
		if falling_edge(clk) then
			if reset = '1' then
				refreshing <= '0';
			elsif (ddr_auto_ref_req = '1') then
				refreshing <= '1';
			elsif (ddr_ar_done = '1') then
				refreshing <= '0';
			end if;
		end if;
	end process;



	-- ddr_address(1 downto 0) select the bank. 
	-- In the same operation only one bank can be accessed.
	--
	-- ddr_address(2) is for the 16 word, but ddr_address(2)='0' and ddr_address(2)='1' access 
	-- the same 32bit word:
	-- ddr_address <= x"00112233"; 
	--
	--  If ddr_address(2) = '0' then
	-- 	     ddr_data(ddr_address) = x"00112233";
	--	Else
	-- 	     ddr_data(ddr_address) = x"22330011";

	-- When reading from sdram to cache.
	ddr_address_tmp1	<=	block_address(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS + 2 ) 
								& addr_counter 
								& "0" 
								& block_address(CACHE_BLOCK_BITS + 1 downto CACHE_BLOCK_BITS);

	-- When writing to sdram from cache.
	ddr_address_tmp2	<=	tag( CACHE_TAG_BITS-1 downto 0)
								& set_address(CACHE_SET_BITS - 1 downto 2)
								& addr_counter 
								& "0" 
								& set_address(1 downto 0);

	
	set_address <= block_address(CACHE_BLOCK_BITS + CACHE_SET_BITS - 1 downto CACHE_BLOCK_BITS);
	
	-- Block address register
	process(clk, reset, store_block_addr) begin
		if (clk'event and clk = '0') then
			if (reset = '1') then
				block_address <= (others => '0');
			elsif (store_block_addr = '1') then
				block_address <= cache_block_request;
			end if;
		end if;
	end process;
	
	-- TAG register
	process(clk, reset, store_tag) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				tag <= (others => '0');
			elsif (store_tag = '1') then
				tag <= tlb_tag_out(CACHE_TAG_BITS downto 1);
			end if;
		end if;
	end process;
	


	------------------
	---- COUNTERS ----
	------------------
	
	-- Address counter.
	process(clk, reset, addr_counter_en) begin
		if (clk'event and clk = '0') then
			if (reset = '1') then
				addr_counter <= (others => '0');
			elsif (addr_counter_reset = '1') then
				addr_counter <= (others => '0');
			elsif (addr_counter_en = '1') then
				addr_counter <= addr_counter + 1;
			end if;
		end if;
	end process;

	-- Data counter.
	process(clk90, reset, data_counter_en) begin
		if (clk90'event and clk90 = '1') then
			if (reset90 = '1') then
				data_counter <= (others => '0');
			elsif (data_counter_reset = '1') then
				data_counter <= (others => '0');
			elsif (data_counter_en = '1') then
				data_counter <= data_counter + 1;
			end if;
		end if;
	end process;
	
	-- Start counting when the data comming from the SDRAM ( data_valid ) or writing data to the SDRAM ( write1 ).
	data_counter_en <= (data_valid  or  write1 ) and ddr_cmd_ack;
	
	
	------------------
	-- DATA READING --
	------------------
	
	-- Data valid register (coming from the SDRAM).
	process( clk90, reset90 ) begin
		if (clk90'event and clk90 = '1') then
			if (reset90 = '1') then
				data_valid <= '0';
			elsif (ddr_data_valid = '1') then
				data_valid <= '1';
			else
				data_valid <= '0';
			end if;
		end if;
	end process;

	-- Cache write control register.
	process(clk90, reset90) begin
		if (clk90'event and clk90 = '1') then
			if (reset90 = '1') then
				cache_data_in <= (others => '0');
				cache_write_enable_s <= '0';
			elsif (store1 = '1') then
				cache_data_in <= ddr_data_out;
				cache_write_enable_s <= '1';
			else
				cache_write_enable_s <= '0';
			end if;
		end if;
	end process;	
	
	store1 <= ddr_data_valid;
	
	-- Cache enable when reading or writing to the SDRAM.
	cache_enable <= cache_write_enable_s or write1;
	
	-- Cache write enable when data_valid coming from the SDRAM.
	cache_write_enable <= cache_write_enable_s;
	
	-- Address to the cache.
	cache_address <=  block_address(CACHE_ADDR_BITS - 1 downto CACHE_BLOCK_BITS) & data_counter;
	
	-- Data to SDRAM
	ddr_data_in <= cache_data_out;
	ddr_data_mask <= "0000";

	


end Behavioral;

