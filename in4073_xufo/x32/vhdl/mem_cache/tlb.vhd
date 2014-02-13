-------------------------------------------------------------------------------
-- Filename:             tlb.vhd
-- Entity:               tlb
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          TLB (Translation Lookaside Buffer) module.
--                       This module contains the tags and the dirty bit
--                       for each of the sets of the cache.
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
use	work.cache_parameters.all;


entity tlb is

	Generic(
		-- Initialization tag.
		tlb_tag_init	: std_logic_vector(CACHE_TAG_BITS - 1 downto 0) := (others => '0')
	);
	
	-- Data width is the number of bits of the tag ( CACHE_TAG_BITS ) + the dirty bit.
	Port(
		-- Port A
		clockA 			: in	std_logic;
		enableA			: in 	std_logic;
		write_enableA	: in	std_logic;
		addressA		: in	std_logic_vector(CACHE_SET_BITS - 1 downto 0);
		data_inA		: in	std_logic_vector(CACHE_TAG_BITS downto 0);
		data_outA		: out	std_logic_vector(CACHE_TAG_BITS downto 0);
		
		-- Port B
		clockB 			: in	std_logic;
		enableB			: in 	std_logic;
		write_enableB	: in	std_logic;
		addressB		: in	std_logic_vector(CACHE_SET_BITS - 1 downto 0);
		data_inB		: in	std_logic_vector(CACHE_TAG_BITS downto 0);
		data_outB		: out	std_logic_vector(CACHE_TAG_BITS downto 0)
		
		);

end tlb;

architecture Behavioral of tlb is

constant address_bits : integer := CACHE_SET_BITS;
constant data_width	: integer := CACHE_TAG_BITS + 1;
constant tag_init : std_logic_vector( CACHE_TAG_BITS downto 0) := tlb_tag_init & "1";

type ram_type is array (2**address_bits-1 downto 0) of std_logic_vector (data_width-1 downto 0);
shared variable bram: ram_type := (others => tag_init);

begin

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

	process (clockB)
	begin
		if (clockB'event and clockB = '0') then
			if (enableB = '1') then
				if (write_enableB = '1') then
					bram(conv_integer(addressB)) := data_inB;
				end if;
				data_outB <= bram(conv_integer(addressB));
			end if;
		end if;
	end process;

end Behavioral;

