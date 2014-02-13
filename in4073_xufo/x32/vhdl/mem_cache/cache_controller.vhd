-------------------------------------------------------------------------------
-- Filename:             cache_controller.vhd
-- Entity:               cache_controller
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          Cache controller module. This manages the
--                       communication between the cache and the X32.
--                       When the cache controller receives a write 
--                       or read operation reads or writes the data
--                       in the corresponding addresses. It also aligns
--                       the data. If the address of any of the words is
--                       not in the TLB, it issues a cache request to the
--                       SDRAM controller.
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
use work.cache_parameters.all;

entity cache_controller is

  port
    (
        -- system clock
        clk                         : in    std_logic;
        -- system reset
        reset                       : in    std_logic;
        
        -- memory address (unaligned)
        mem_address                 : in    std_logic_vector(PHY_MEM_ADDR_BITS - 1 downto 0);
        -- data to memory
        mem_data_in                 : in    std_logic_vector(31 downto 0);
        -- data from memory
        mem_data_out                : out   std_logic_vector(31 downto 0);
        -- size of data (see types.vhd)
        mem_data_size               : in    std_logic_vector(2 downto 0);
        -- data signedness (1=signed, 0=unsigned)
        mem_data_signed             : in    std_logic;
        -- start read operation (high = start read)
        mem_read                    : in    std_logic;
        -- start write operation (high = start write)
        mem_write                   : in    std_logic;
        -- pulses high when the operation finishes
        mem_ready                   : out   std_logic;
		-- memory overflow
        overflow                    : out   std_logic;
		  
        cache_request               : out   std_logic;
        cache_block_request         : out   std_logic_vector(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);
        cache_ready	                : in    std_logic;

        cache_enable                : out   STD_LOGIC;
        cache_write_enable			: out   STD_LOGIC;
        cache_data_mask             : out   STD_LOGIC_VECTOR (3 downto 0);
        cache_address               : out   STD_LOGIC_VECTOR (CACHE_ADDR_BITS - 1 downto 0);			  
        cache_data_in               : out   STD_LOGIC_VECTOR (31 downto 0);
        cache_data_out              : in    STD_LOGIC_VECTOR (31 downto 0);
			
        tlb_set_address             : out   std_logic_vector( CACHE_SET_BITS - 1 downto 0);
        tlb_tag_in                  : out   std_logic_vector( CACHE_TAG_BITS downto 0);
        tlb_tag_out                 : in    std_logic_vector( CACHE_TAG_BITS downto 0);
        tlb_enable                  : out   std_logic;
        tlb_write_enable            : out   std_logic
    );

end cache_controller;

architecture Behavioral of cache_controller is

component mem_decoder
	port (
		address_offset 		: in  std_logic_vector(1 downto 0);
		mem_data_size		: in  std_logic_vector(2 downto 0);
		mem_data_signed		: in  std_logic;
		
		mem_data_in1		: in  std_logic_vector(31 downto 0);
		mem_data_in2		: in  std_logic_vector(31 downto 8);
		proc_data_in		: out std_logic_vector(31 downto 0);
		
		proc_data_out		: in  std_logic_vector(31 downto 0);
		mem_data_out1		: out std_logic_vector(31 downto 0);
		mem_data_out2		: out std_logic_vector(31 downto 0);

		bytes_enable1		: out std_logic_vector(3 downto 0);
		bytes_enable2		: out std_logic_vector(3 downto 0);

		overflow			: out std_logic
	);
end component;




signal	bytes_enable			:  std_logic_vector(3 downto 0);
signal	bytes_enable1_i		:  std_logic_vector(3 downto 0);
signal	bytes_enable2_i		:  std_logic_vector(3 downto 0);
signal	bytes_enable1_o		:  std_logic_vector(3 downto 0);
signal	bytes_enable2_o		:  std_logic_vector(3 downto 0);


signal	bram_read			:  STD_LOGIC;
  

-- register store signals
signal store1, store2, store_addr, store_outputs : std_logic;

signal mem_address_offset : std_logic_vector(1 downto 0);  
signal sig_overflow : std_logic;
	
-- Address signals.
-- When the data is not aligned, 2 words may be required.
signal mem_address1 : std_logic_vector(MEM_ADDR_BITS - 1 downto 0);
signal mem_address2 : std_logic_vector(MEM_ADDR_BITS - 1 downto 0);
signal mem_tag1, mem_tag2	:	std_logic_vector( CACHE_TAG_BITS - 1 downto 0);
signal mem_set1, mem_set2	:	std_logic_vector( CACHE_SET_BITS - 1 downto 0);
	
-- data signals
signal mem_data_in1, mem_data_out1_i, mem_data_out2_i, mem_data_out1_o, mem_data_out2_o : std_logic_vector(31 downto 0);
signal mem_data_in2 : std_logic_vector(31 downto 8);
signal mem_data_out_s, mem_data_out_r : std_logic_vector(31 downto 0) := (others => '0');
  
type state_t is ( RESET_STATE, IDLE, 
                  READ1A, READ1B, READ2, ALIGN_INPUT_DATA,
                  ALIGN_OUTPUT_DATA, CHECK_WRITE1, WRITE1, WRITE2, 
                  CACHE_MISS1, CACHE_MISS2
                ); 
signal  state_i, state_o : state_t := RESET_STATE; 



begin

	-- Get the different parts of the addresses.
	mem_tag1 <= mem_address1(MEM_ADDR_BITS - 1 downto MEM_ADDR_BITS - CACHE_TAG_BITS );
	mem_tag2 <= mem_address2(MEM_ADDR_BITS - 1 downto MEM_ADDR_BITS - CACHE_TAG_BITS );

	mem_set1 <= mem_address1(MEM_ADDR_BITS - CACHE_TAG_BITS - 1 downto MEM_ADDR_BITS - CACHE_TAG_BITS - CACHE_SET_BITS  );
	mem_set2 <= mem_address2(MEM_ADDR_BITS - CACHE_TAG_BITS - 1 downto MEM_ADDR_BITS - CACHE_TAG_BITS - CACHE_SET_BITS  );

	
	-- Alignment module.
	Inst_mem_decoder: mem_decoder PORT MAP(

		address_offset  => mem_address_offset,
		mem_data_size   => mem_data_size,
		mem_data_signed => mem_data_signed,
		
		mem_data_in1    => mem_data_in1,
		mem_data_in2    => mem_data_in2,
		
		proc_data_in    => mem_data_out_s,
		
		proc_data_out   => mem_data_in,
		
		mem_data_out1   => mem_data_out1_i,
		mem_data_out2   => mem_data_out2_i,
		
		bytes_enable1   => bytes_enable1_i,
		bytes_enable2   => bytes_enable2_i,
		overflow        => sig_overflow
	);

	
	-- Data out register.
	mem_data_out <= mem_data_out_s when state_o = ALIGN_INPUT_DATA else mem_data_out_r;						 
	process(clk)
	begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_data_out_r <= x"00000000";
			elsif (state_o = ALIGN_INPUT_DATA) then
				mem_data_out_r <= mem_data_out_s;
			end if;
		end if;
	end process;

	-- overflow only on write
	overflow <= sig_overflow and mem_write;
	
	cache_data_mask <= bytes_enable;

  	process(state_o, mem_read, mem_write, bytes_enable1_o, bytes_enable2_o, 
		mem_data_out1_o, mem_data_out2_o, mem_address1, mem_address2,
		bytes_enable2_i, mem_address,cache_ready,mem_set1,mem_set2, tlb_tag_out,mem_tag1,mem_tag2) begin
		
		case state_o is
			when RESET_STATE =>
				-- reset/startup stage
				mem_ready <= '0';

				cache_data_in <= (others => '-');
				cache_data_in <= (others => '-');
				bytes_enable <= (others => '-');
				cache_address <= (others => '-');
				
				cache_enable <= '0'; 
				bram_read   <= '0';
				cache_write_enable  <= '0';
				
				cache_request <= '0';
				cache_block_request <= (others => '-');
								
				tlb_set_address <= (others => '-');
				tlb_enable <= '0';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';

				state_i <= IDLE;

			when IDLE =>
				-- idle, wait for read/write operations
				mem_ready <= '0';


				cache_enable <= '0'; 
				bram_read   <= '0';
				cache_write_enable  <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= "0000";
				cache_address <= (others => '-');

				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				-- store the address
				store1 <= '0';
				store2 <= '0';
				store_addr <= '1';
				store_outputs <= '0';
				
				tlb_set_address <= (others => '-');
				tlb_enable <= '0';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				-- Start reading or writing.
				if (mem_read = '1') then 
					state_i <= READ1A;
				elsif (mem_write = '1') then
					state_i <= ALIGN_OUTPUT_DATA;
				else
					state_i <= IDLE;
				end if;
				
			when READ1A =>
				-- Start reading the first word.
				
				mem_ready <= '0';
				
				-- Set the address for reading the first word.
				cache_enable <= '1'; 
				bram_read   <= '1';
				cache_write_enable  <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= "0000";
				cache_address <= mem_address1(CACHE_ADDR_BITS - 1 downto 0);

				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- Set the TLB address for reading the TAG corresponding to the first word.
				tlb_set_address <= mem_set1;
				tlb_enable <= '1';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				state_i <= READ1B;
				
				
			when READ1B =>
				-- Get the first word.
				mem_ready <= '0';

				-- Set the address for reading the second word.
				cache_enable <= '1'; 
				bram_read   <= '1';
				cache_write_enable  <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= "0000";
				cache_address <= mem_address2(CACHE_ADDR_BITS - 1 downto 0);
				
				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				-- Get the tag of the second word.
				tlb_set_address <= mem_set2;
				tlb_enable <= '1';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				-- Store the first word.
				store1 <= '1';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- Check wether the address matches the tag of the first word or not.
				if (tlb_tag_out(CACHE_TAG_BITS downto 1) = mem_tag1) then
					if (bytes_enable2_i = "1111") then
						-- Align the data if it is not needed to read the second word
						state_i <= ALIGN_INPUT_DATA;
					else
						-- Read second word.
						state_i <= READ2;
					end if;
				else -- If the TAG is different, request the block.
					state_i <= CACHE_MISS1;
				end if;
				
			when READ2 =>
				-- Read the second word
				mem_ready <= '0';

				cache_enable <= '1'; 
				bram_read   <= '1';
				cache_write_enable  <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= "0000";
				cache_address <= mem_address2(CACHE_ADDR_BITS - 1 downto 0);
				
				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				tlb_set_address <= mem_set2;
				tlb_enable <= '1';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				-- Store the second word
				store1 <= '0';
				store2 <= '1';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- If the tag does not match the address, issue a cache request.
				if (tlb_tag_out(CACHE_TAG_BITS downto 1) = mem_tag2) then
					state_i <= ALIGN_INPUT_DATA;
				else
					state_i <= CACHE_MISS2;
				end if;

			when ALIGN_INPUT_DATA =>
				-- align the 2 words coming from the cache into a single word
				-- End op.
				mem_ready <= '1';

				cache_enable <= '0';
				cache_write_enable  <= '0';
				bram_read   <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= (others => '-');
				cache_address <= (others => '-');
				
				cache_request <= '0';
				cache_block_request <= (others => '-');				
				
				tlb_set_address <= (others => '-');
				tlb_enable <= '0';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';

				state_i <= IDLE;

			when ALIGN_OUTPUT_DATA =>
				-- Start of the write operation.
				-- align the output data into two words
				mem_ready <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= (others => '-');
				cache_address <= (others => '-');
				
				cache_enable <= '0';
				cache_write_enable  <= '0';
				bram_read   <= '0';
				
				cache_request <= '0';
				cache_block_request <= (others => '-');				
				
				-- Set the address of the first word in the TLB
				tlb_set_address <= mem_set1;
				tlb_enable <= '1';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');
				
				-- Store the aligned words.
				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '1';

				state_i <= CHECK_WRITE1;
				
			when CHECK_WRITE1 =>
				-- Check if the word is in the cache
				
				mem_ready <= '0';

				cache_enable <= '0'; 
				bram_read   <= '0';
				cache_write_enable  <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= "0000";
				cache_address <= mem_address1(CACHE_ADDR_BITS - 1 downto 0);
				
				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				-- Set the address of the first word in the TLB
				tlb_set_address <= mem_set2;
				tlb_enable <= '1';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- If the first word is already in the cache proceed further.
				if (tlb_tag_out(CACHE_TAG_BITS downto 1) = mem_tag1) then
					state_i <= WRITE1;
				else
					state_i <= CACHE_MISS1;
				end if;
				
			when WRITE1 =>
				-- write the first word.
				mem_ready <= '0';

				cache_data_in <= mem_data_out1_o;
				bytes_enable <= bytes_enable1_o;
				cache_address <= mem_address1(CACHE_ADDR_BITS - 1 downto 0);

				cache_enable <= '1';
				cache_write_enable  <= '1';
				bram_read   <= '0';
				
				cache_request <= '0';
				cache_block_request <= (others => '-');				
				
				-- Update the dirty bit in the TLB
				tlb_set_address <= mem_set1;
				tlb_enable <= '1';
				tlb_write_enable <= '1';
				tlb_tag_in <= mem_tag1 & "1";

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';

				-- if only the lower word must be written, report ready
				if (bytes_enable2_i = "1111") then
					state_i <= IDLE;
					mem_ready <= '1';
				else
					mem_ready <= '0';
					-- If the second word is needed and it is not in the cache
					-- issue a new request.
					if (tlb_tag_out(CACHE_TAG_BITS downto 1) = mem_tag2) then
						state_i <= WRITE2;
					else
						state_i <= CACHE_MISS2;
					end if;					
				end if;
			when WRITE2 =>
				-- write word 2
				-- report ready
				mem_ready <= '1';

				-- Write the sencond word in the cache.
				cache_data_in <= mem_data_out2_o;
				bytes_enable <= bytes_enable2_o;
				cache_address <= mem_address2(CACHE_ADDR_BITS - 1 downto 0);

				cache_enable <= '1';
				cache_write_enable  <= '1';
				bram_read   <= '0';
				
				cache_request <= '0';
				cache_block_request <= (others => '-');
				
				-- Update the dirty bit of the TLB.
				tlb_set_address <= mem_set2;
				tlb_enable <= '1';
				tlb_write_enable <= '1';
				tlb_tag_in <= mem_tag2 & "1";

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';

				state_i <= IDLE;
				
			when CACHE_MISS1 =>
				-- Issue a cache request with the address of the block corresponding
				-- to the first word.
				mem_ready <= '0';

				cache_enable <= '0';
				cache_write_enable  <= '0';
				bram_read   <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= (others => '-');
				cache_address <= (others => '-');
				
				cache_request <= '1';
				cache_block_request <= mem_address1(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);				
				
				tlb_set_address <= (others => '-');
				tlb_enable <= '0';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- Wait until the cache is updated.
				if ( cache_ready = '1') then
					state_i <= IDLE;	
				else
					state_i <= CACHE_MISS1;
				end if;
				
			when CACHE_MISS2 =>
				-- Issue a cache request with the address of the block corresponding
				-- to the first word.				
				mem_ready <= '0';

				cache_enable <= '0';
				cache_write_enable  <= '0';
				bram_read   <= '0';

				cache_data_in <= (others => '-');
				bytes_enable <= (others => '-');
				cache_address <= (others => '-');
				
				cache_request <= '1';
				cache_block_request <= mem_address2(MEM_ADDR_BITS - 1 downto CACHE_BLOCK_BITS);				
				
				tlb_set_address <= (others => '-');
				tlb_enable <= '0';
				tlb_write_enable <= '0';
				tlb_tag_in <= (others => '-');

				store1 <= '0';
				store2 <= '0';
				store_addr <= '0';
				store_outputs <= '0';
				
				-- Wait until the cache is updated.
				if ( cache_ready = '1') then
					state_i <= IDLE;	
				else
					state_i <= CACHE_MISS2;
				end if;
		end case;
	end process;


	-- fsm register
	process(clk, reset, state_i) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state_o <= RESET_STATE;
			else
				state_o <= state_i;
			end if;
		end if;
	end process;

	-- (low) address register
	process(clk, reset, store_addr, mem_address) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_address1 <= (others => '0');
				mem_address_offset <= (others => '0');
			elsif (store_addr = '1') then
				mem_address1 <= mem_address(PHY_MEM_ADDR_BITS - 1 downto 2);
				mem_address_offset <= mem_address(1 downto 0);
			end if;
		end if;
	end process;	

	-- (high) address register
	process(clk, reset, mem_address1) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_address2 <= (others => '0');
			else
				mem_address2 <= mem_address1+1;
			end if;
		end if;
	end process;	

	-- low word register
	process(clk, reset, store1, mem_data_in1, cache_data_out) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_data_in1 <= (others => '0');
			elsif (store1 = '1') then
				mem_data_in1 <= cache_data_out;
			end if;
		end if;
	end process;	

	-- high word register
	process(clk, reset, store2, mem_data_in2, cache_data_out) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_data_in2 <= (others => '0');
			elsif (store2 = '1') then
				mem_data_in2 <= cache_data_out(31 downto 8);
			end if;
		end if;
	end process;	

	-- decoded data register
	process(clk, reset, store_outputs, 
		mem_data_out1_i, mem_data_out2_i, bytes_enable1_i, bytes_enable2_i,
		mem_data_out1_o, mem_data_out2_o, bytes_enable1_o, bytes_enable2_o) begin
	
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_data_out1_o <= (others => '0');
				mem_data_out2_o <= (others => '0');
				bytes_enable1_o <= (others => '0');
				bytes_enable2_o <= (others => '0');
			elsif (store_outputs = '1') then
				mem_data_out1_o <= mem_data_out1_i;
				mem_data_out2_o <= mem_data_out2_i;
				bytes_enable1_o <= bytes_enable1_i;
				bytes_enable2_o <= bytes_enable2_i;
			end if;
		end if;
	end process;	


end Behavioral;

