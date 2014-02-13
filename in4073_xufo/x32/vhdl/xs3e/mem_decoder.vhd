-------------------------------------------------------------------------------
-- Filename:             mem_decoder.vhd
-- Entity:               mem_decoder
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Memory data decoder for the X32 softcore. The
--                       component realigns memory data to execute unaligned
--                       X32 memory operations on aligned memory chips. The
--                       memory chips are assumed to be 32-bit aligned. For
--                       reading memory, the decoder takes two inputs for two
--                       32-bit words (for example, when reading 32-bit from
--                       address 3, both word at location 0, and the word of
--                       location 4 must be read, these are the two inputs),
--                       and creates one 32-bit output. For writing memory, the
--                       decoder does the opposite: it takes a single 32-bit
--                       word, and generates two words, depending on the
--                       address alignment
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

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.types.all;

entity mem_decoder is
	port (
		-- the lower two bits of the address (location offset)
		address_offset 	: in  std_logic_vector(1 downto 0);
		-- the size of the data (LCC Bytecode datatype, see types.vhd)
		mem_data_size		: in  std_logic_vector(2 downto 0);
		-- signed = 1, unsigned = 0
		mem_data_signed	: in  std_logic;
		
		-- aligned word 1 from memory (address & 0xFFFFFFFC)
		mem_data_in1		: in  std_logic_vector(31 downto 0);
		-- aligned word 2 from memory (address + 4 & 0xFFFFFFFC)
		mem_data_in2		: in  std_logic_vector(31 downto 8);
		-- aligned word to processor
		proc_data_in		: out std_logic_vector(31 downto 0);
		
		-- word from processor
		proc_data_out		: in  std_logic_vector(31 downto 0);
		-- aligned word 1 to memory (address & 0xFFFFFFFC)
		mem_data_out1		: out std_logic_vector(31 downto 0);
		-- aligned word 2 to memory (address + 4 & 0xFFFFFFFC)
		mem_data_out2		: out std_logic_vector(31 downto 0);

		-- bytes enable for word 1
		bytes_enable1		: out std_logic_vector(3 downto 0);
		-- bytes enable for word 2
		bytes_enable2		: out std_logic_vector(3 downto 0);

		-- overflow when writing large numbers to small data types, eg
		--  when the value is 10000 and mem_data_size is set to CHAR
		overflow		: out std_logic
	);
end entity;

architecture behaviour of mem_decoder is
	signal is_32, is_16, is_8 : std_logic;
	signal overflow_16, overflow_8 : std_logic;
	signal overflow_16_signed, overflow_8_signed : std_logic;
	signal ones, zeroes : std_logic_vector(23 downto 0);
begin
	is_32 <= 
		'1' when mem_data_size = VARTYPE_PTR else
		'1' when mem_data_size = VARTYPE_LNG else
		'1' when mem_data_size = VARTYPE_INT else
		'0';
	is_16 <=
		'1' when mem_data_size = VARTYPE_SHORT else
		'1' when mem_data_size = VARTYPE_INSTRUCTION else
		'0';
	is_8 <=
		'1' when mem_data_size = VARTYPE_CHAR else
		'0';

	ones <= (others => '1');
	zeroes <= (others => '0');

	overflow_16_signed <= '0' when proc_data_out(15) = proc_data_out(16) else '1';
	overflow_16 <= 
		'0' when proc_data_out(31 downto 16) = zeroes(15 downto 0) else
		'0' when proc_data_out(31 downto 16) = ones(15 downto 0) else
		'1';

	overflow_8_signed <= '0' when proc_data_out(7) = proc_data_out(8) else '1';
	overflow_8 <= 
		'0' when proc_data_out(31 downto 8) = zeroes(23 downto 0) else
		'0' when proc_data_out(31 downto 8) = ones(23 downto 0) else
		'1';

	overflow <= (overflow_16 and is_16) or (overflow_8 and is_8) or
		(overflow_16_signed and mem_data_signed and is_16) or
		(overflow_8_signed and mem_data_signed and is_8);
			
	process(is_32, is_16, is_8, address_offset, mem_data_signed, mem_data_in1, mem_data_in2, proc_data_out) begin
		if (is_32 = '1' and address_offset = "00") then
			proc_data_in <= mem_data_in1;
		
			mem_data_out1 <= proc_data_out;
			mem_data_out2 <= (others => '-');
		
			bytes_enable1 <= "0000";
			bytes_enable2 <= "1111";
		elsif (is_32 = '1' and address_offset = "01") then
			proc_data_in(31 downto 8) <= mem_data_in1(23 downto 0);
			proc_data_in(7 downto 0) <= mem_data_in2(31 downto 24);

			mem_data_out1(31 downto 24) <= (others => '-');
			mem_data_out1(23 downto 0) <= proc_data_out(31 downto 8);
			mem_data_out2(31 downto 24) <= proc_data_out(7 downto 0);
			mem_data_out2(23 downto 0) <= (others => '-');

			bytes_enable1 <= "1000";
			bytes_enable2 <= "0111";
		elsif (is_32 = '1' and address_offset = "10") then
			proc_data_in(31 downto 16) <= mem_data_in1(15 downto 0);
			proc_data_in(15 downto 0) <= mem_data_in2(31 downto 16);

			mem_data_out1(31 downto 16) <= (others => '-');
			mem_data_out1(15 downto 0) <= proc_data_out(31 downto 16);
			mem_data_out2(31 downto 16) <= proc_data_out(15 downto 0);
			mem_data_out2(15 downto 0) <= (others => '-');

			bytes_enable1 <= "1100";
			bytes_enable2 <= "0011";
		elsif (is_32 = '1' and address_offset = "11") then
			proc_data_in(31 downto 24) <= mem_data_in1(7 downto 0);
			proc_data_in(23 downto 0) <= mem_data_in2(31 downto 8);

			mem_data_out1(31 downto 8) <= (others => '-');
			mem_data_out1(7 downto 0) <= proc_data_out(31 downto 24);
			mem_data_out2(31 downto 8) <= proc_data_out(23 downto 0);
			mem_data_out2(7 downto 0) <= (others => '-');

			bytes_enable1 <= "1110";
			bytes_enable2 <= "0001";
		elsif (is_16 = '1' and address_offset = "00") then
			proc_data_in(31 downto 16) <= (others => (mem_data_signed and mem_data_in1(31)));
			proc_data_in(15 downto 0) <= mem_data_in1(31 downto 16);

			mem_data_out1(31 downto 16) <= proc_data_out(15 downto 0);
			mem_data_out1(15 downto 0) <= (others => '-');
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "0011";
			bytes_enable2 <= "1111";
		elsif (is_16 = '1' and address_offset = "01") then
			proc_data_in(31 downto 16) <= (others => (mem_data_signed and mem_data_in1(23)));
			proc_data_in(15 downto 0) <= mem_data_in1(23 downto 8);

			mem_data_out1(31 downto 24) <= (others => '-');
			mem_data_out1(23 downto 8) <= proc_data_out(15 downto 0);
			mem_data_out1(7 downto 0) <= (others => '-');
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "1001";
			bytes_enable2 <= "1111";
		elsif (is_16 = '1' and address_offset = "10") then
			proc_data_in(31 downto 16) <= (others => (mem_data_signed and mem_data_in1(15)));
			proc_data_in(15 downto 0) <= mem_data_in1(15 downto 0);

			mem_data_out1(31 downto 16) <= (others => '-');
			mem_data_out1(15 downto 0) <= proc_data_out(15 downto 0);
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "1100";
			bytes_enable2 <= "1111";
		elsif (is_16 = '1' and address_offset = "11") then
			proc_data_in(31 downto 16) <= (others => (mem_data_signed and mem_data_in1(7)));
			proc_data_in(15 downto 8) <= mem_data_in1(7 downto 0);
			proc_data_in(7 downto 0) <= mem_data_in2(31 downto 24);

			mem_data_out1(31 downto 8) <= (others => '-');
			mem_data_out1(7 downto 0) <= proc_data_out(15 downto 8);
			mem_data_out2(31 downto 24) <= proc_data_out(7 downto 0);
			mem_data_out2(23 downto 0) <= (others => '-');

			bytes_enable1 <= "1110";
			bytes_enable2 <= "0111";
		elsif (is_8 = '1' and address_offset = "00") then
			proc_data_in(31 downto 8) <= (others => (mem_data_signed and mem_data_in1(31)));
			proc_data_in(7 downto 0) <= mem_data_in1(31 downto 24);

			mem_data_out1(31 downto 24) <= proc_data_out(7 downto 0);
			mem_data_out1(23 downto 0) <= (others => '-');
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "0111";
			bytes_enable2 <= "1111";
		elsif (is_8 = '1' and address_offset = "01") then
			proc_data_in(31 downto 8) <= (others => (mem_data_signed and mem_data_in1(23)));
			proc_data_in(7 downto 0) <= mem_data_in1(23 downto 16);

			mem_data_out1(31 downto 24) <= (others => '-');
			mem_data_out1(23 downto 16) <= proc_data_out(7 downto 0);
			mem_data_out1(15 downto 0) <= (others => '-');
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "1011";
			bytes_enable2 <= "1111";
		elsif (is_8 = '1' and address_offset = "10") then
			proc_data_in(31 downto 8) <= (others => (mem_data_signed and mem_data_in1(15)));
			proc_data_in(7 downto 0) <= mem_data_in1(15 downto 8);

			mem_data_out1(31 downto 16) <= (others => '-');
			mem_data_out1(15 downto 8) <= proc_data_out(7 downto 0);
			mem_data_out1(7 downto 0) <= (others => '-');
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "1101";
			bytes_enable2 <= "1111";
		elsif (is_8 = '1' and address_offset = "11") then
			proc_data_in(31 downto 8) <= (others => (mem_data_signed and mem_data_in1(7)));
			proc_data_in(7 downto 0) <= mem_data_in1(7 downto 0);

			mem_data_out1(31 downto 8) <= (others => '-');
			mem_data_out1(7 downto 0) <= proc_data_out(7 downto 0);
			mem_data_out2(31 downto 0) <= (others => '-');

			bytes_enable1 <= "1110";
			bytes_enable2 <= "1111";
		else
			proc_data_in <= (others => '-');
			mem_data_out1 <= (others => '-');
			mem_data_out2 <= (others => '-');
			
			bytes_enable1 <= "1111";
			bytes_enable2 <= "1111";
		end if;
	end process;
end architecture;
