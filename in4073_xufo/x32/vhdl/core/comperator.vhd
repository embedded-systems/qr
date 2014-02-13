-------------------------------------------------------------------------------
-- Filename:             comperator.vhd
-- Entity:               comperator
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Comparator (sorry for the spelling mistake...) file,
--                       this component instantiates an equality and signed,
--                       and unsigned comparator to create a full comparator
--                       supporting all possible binary comparisons.
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

entity comperator is
	generic (
		-- size (bits) of the operands
		size				: positive
	);
	port(
		-- operands are signed (1=signed, 0=unsigned)
		signed			: in  std_logic;
		-- type of comparison:
		--  0001 equal
		--  1000 not equal
		--  0010 greater than
		--  0011 greater than or equal
		--  0100 lesser than
		--  0101 lesser than or equal
		opcode			: in  std_logic_vector(3 downto 0);
		-- operand 1
		op1					: in  std_logic_vector(size-1 downto 0);
		-- operand 2
		op2					: in  std_logic_vector(size-1 downto 0);
		-- result (high = positive compare)
		result			: out std_logic
	);
end entity;

architecture behaviour of comperator is
	-- result of signed greater than comparison
	signal signed_greater				: std_logic;
	-- result of unsigned greater than comparison
	signal unsigned_greater			: std_logic;

	-- result of equal comparison
	signal equal								: std_logic;
	-- result of greater comparison
	signal greater							: std_logic;
	-- result of lesser comparison
	signal lesser								: std_logic;
begin
	-- signed greater than comparator
	l_signed: entity work.signed_comperator(behaviour)
		generic map (
			size => size
		)
		port map (
			op1 => op1,
			op2 => op2,
			greater => signed_greater
		);

	-- unsigned greater than comparator
	l_unsigned: entity work.unsigned_comperator(behaviour)
		generic map (
			size => size
		)
		port map (
			op1 => op1,
			op2 => op2,
			greater => unsigned_greater
		);

	-- generate greater signal depending on sign input
	greater <= signed_greater when signed = '1' else
					unsigned_greater;
	-- generate lesser signal depending on greater & equal inputs
	lesser <= not (greater or equal);

	-- create results by combining opcode with all comparison results
	result <= (opcode(3) and not equal) or 
					(opcode(2) and lesser) or
					(opcode(1) and greater) or
					(opcode(0) and equal);

	-- equality comparator
	process(op1,op2) begin
		if (op1 = op2) then
			equal <= '1';
		else
			equal <= '0';
		end if;
	end process;
end architecture;