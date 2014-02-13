-------------------------------------------------------------------------------
-- Filename:             bram_dual_clock.vhd
-- Entity:               bram_dual_clock
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          Block RAM module. Fixed data width of 8bits and
--                       variable size. It is initializated by a file
--                       with the name of "cache_content.bits".  This RAM
--                       will store one byte of each word. The 
--                       ram number determines which byte of each word
--                       is going to store.
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
use IEEE.STD_LOGIC_TEXTIO.ALL;
library STD;
use STD.TEXTIO.ALL;

entity bram_dual_clock is
	Generic(
		address_bits	: integer;
		word_size		: integer := 32;
		ram_number		: integer
	);
	
	Port(
		-- Port A
		clockA 			: in  std_logic;
		enableA			: in 	std_logic;
		write_enableA	: in	std_logic;
		addressA		: in	std_logic_vector(address_bits - 1 downto 0);
		data_inA		: in	std_logic_vector(7 downto 0);
		data_outA		: out	std_logic_vector(7 downto 0);
		
		-- Port B
		clockB 			: in  std_logic;
		enableB			: in 	std_logic;
		write_enableB	: in	std_logic;
		addressB		: in	std_logic_vector(address_bits - 1 downto 0);
		data_inB		: in	std_logic_vector(7 downto 0);
		data_outB		: out	std_logic_vector(7 downto 0)
		
		);

end bram_dual_clock;

architecture Behavioral of bram_dual_clock is

type ram_type is array (2**address_bits-1 downto 0) of std_logic_vector (7 downto 0);


impure function ram_loader ( file_name : in string) return ram_type is                                                   
	FILE ram_file		: text is in file_name;                       
	variable ram_line	: line;                                 
	variable ram		: ram_type;
	variable ram_word	: bit_vector( word_size - 1 downto 0);
	variable word		: std_logic_vector(word_size - 1 downto 0);
	begin                                                        
		for I in 0 to 2 **address_bits -1 loop
			-- Read one word.
			readline (ram_file, ram_line);
			read (ram_line, ram_word);
			word := to_stdlogicvector(ram_word);
			-- Keep only the corresponding byte (ram_number).
			ram(I) := word(ram_number*8 + 7 downto ram_number*8);
		end loop;       
		return ram;                                                  
	end function;                                                




shared variable bram: ram_type  := ram_loader("cache_content.bits");

begin

	-- Port A
	process (clockA)
	begin
		if (clockA'event and clockA = '1') then
			if (enableA = '1') then
				if (write_enableA = '1') then
					bram(conv_integer(addressA)) := data_inA;
				end if;
				data_outA <= bram(conv_integer(addressA));
			end if;
		end if;
	end process;

	-- Port B
	process (clockB)
	begin
		if (clockB'event and clockB = '1') then
			if (enableB = '1') then
				if (write_enableB = '1') then
					bram(conv_integer(addressB)) := data_inB;
				end if;
				data_outB <= bram(conv_integer(addressB));
			end if;
		end if;
	end process;

end Behavioral;

