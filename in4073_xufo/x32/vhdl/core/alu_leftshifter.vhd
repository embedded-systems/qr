-------------------------------------------------------------------------------
-- Filename:             alu_leftshifter.vhd
-- Entity:               left_shifter
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          ALU component which takes care of left shifting
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
use ieee.std_logic_unsigned.all;

entity left_shifter is
	generic (
		-- size of the operands/result (in bits)
		size		: in  positive
	);
	port (
		-- input
		data		: in  std_logic_vector(size-1 downto 0);
		-- amount of bits to shift
		shift		: in  std_logic_vector(size-1 downto 0);
		-- input/output is signed (1=signed, 0=unsigned)
		signed	: in  std_logic;
		-- shifted result
		result	: out std_logic_vector(size-1 downto 0)
	);
end entity;
	
architecture behaviour of left_shifter is
	-- array containing all possible shift solutions
	type BLOCK_TYPE is array (0 to size-1) of std_logic_vector(size-1 downto 0);
	signal all_results : BLOCK_TYPE;
begin
	process(data, shift) begin
		-- "compute" all possible solutions
		for i in 0 to size-1 loop
			all_results(i)(size-1 downto i) <= data(size-1-i downto 0);
			all_results(i)(i-1 downto 0) <= (others => '0');
		end loop;
		
		-- result mux, force output zero when the 2nd operand is larger
		--  than the amount of available bits
		if (shift(size-1) = '0' and conv_integer(shift) < size) then -- MW: sim fix
			result <= all_results(conv_integer(shift));
		else
			result <= (others => '0');
		end if;
	end process;
end architecture;
