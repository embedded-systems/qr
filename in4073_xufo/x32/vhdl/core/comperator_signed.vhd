-------------------------------------------------------------------------------
-- Filename:             comperator_signed.vhd
-- Entity:               signed_comperator
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Comparator (sorry for the spelling mistake...) for
--                       signed comparisons. The component uses the standard
--                       vhdl libraries to implement the comparison.
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
use ieee.std_logic_signed.all;

entity signed_comperator is
	generic (
		-- size (bits) of the operands
		size				: positive
	);
	port(
		-- operand 1
		op1		: in  std_logic_vector(size-1 downto 0);
		-- operand 2
		op2		: in  std_logic_vector(size-1 downto 0);
		-- operand 1 > operand 2
		greater	: out std_logic
	);
end entity;

architecture behaviour of signed_comperator is
begin
	-- comparator
	process(op1, op2) begin
		if (op1 > op2) then
			greater <= '1';
		else
			greater <= '0';
		end if;
	end process;
end architecture;