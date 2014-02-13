-------------------------------------------------------------------------------
-- Filename:             mem_cache.vhd
-- Entity:               mem_cache
-- Architectures:        Behavioral
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          Cache module. Fixed data width of 32bits and
--                       variable size. This block instantiates four
--                       brams, each of them with data width of 8bits.
--                       In this way it is possible to provide masking
--                       by enabling each cache independently.
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

entity mem_cache is

	Port(
		clockA 			: in	std_logic;
		enableA			: in 	std_logic;
		write_enableA	: in	std_logic;
		addressA		: in	std_logic_vector(CACHE_SET_BITS + CACHE_BLOCK_BITS - 1 downto 0);
		data_maskA		: in	std_logic_vector(3 downto 0);
		data_inA		: in	std_logic_vector(31 downto 0);
		data_outA		: out	std_logic_vector(31 downto 0);
		
		clockB 			: in	std_logic;
		enableB			: in 	std_logic;
		write_enableB	: in	std_logic;
		addressB		: in	std_logic_vector(CACHE_SET_BITS + CACHE_BLOCK_BITS - 1 downto 0);
		data_inB		: in	std_logic_vector(31 downto 0);
		data_outB		: out	std_logic_vector(31 downto 0)
		
		);
		
end mem_cache;

architecture Behavioral of mem_cache is

component bram_dual_clock
	Generic(
		address_bits	: integer := 10;
		word_size		: integer := 32;
		ram_number		: integer
	);
	
	Port(
		clockA 			: in	std_logic;
		enableA			: in 	std_logic;
		write_enableA	: in	std_logic;
		addressA		: in	std_logic_vector(address_bits - 1 downto 0);
		data_inA		: in	std_logic_vector(7 downto 0);
		data_outA		: out	std_logic_vector(7 downto 0);
		
		clockB 			: in	std_logic;
		enableB			: in 	std_logic;
		write_enableB	: in	std_logic;
		addressB		: in	std_logic_vector(address_bits - 1 downto 0);
		data_inB		: in	std_logic_vector(7 downto 0);
		data_outB		: out	std_logic_vector(7 downto 0)
		
		);

end component;

	signal write_mask	:	std_logic_vector(3 downto 0);

begin


write_mask <= not data_maskA when write_enableA = '1' else "0000";

BRAMS : for I in 3 downto 0 generate
begin

   	
	bram : bram_dual_clock
	Generic map(
		address_bits	=> CACHE_ADDR_BITS,
		word_size		=> 32,
		ram_number		=> I
	)
	Port map(
		clockA 			=> clockA,
		enableA			=> enableA,
		write_enableA	=> write_mask(I),
		addressA		=> addressA,
		data_inA		=> data_inA(I*8 + 7 downto I*8),
		data_outA		=> data_outA(I*8 + 7 downto I*8),
		
		clockB 			=> clockB,
		enableB			=> enableB,
		write_enableB	=> write_enableB,
		addressB		=> addressB,
		data_inB		=> data_inB(I*8 + 7 downto I*8),
		data_outB		=> data_outB(I*8 + 7 downto I*8)
	);

end generate;


end Behavioral;

