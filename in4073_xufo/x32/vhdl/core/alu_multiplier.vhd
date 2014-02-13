-------------------------------------------------------------------------------
-- Filename:             alu_multiplier.vhd
-- Entity:               multiplier
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          ALU component which takes care of multiplying. The
--                       component uses the standard library to do
--                       multiplication, as most FPGA's contain multipliers.
--                       The component also checks for overflow.
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

entity multiplier is
	generic (
		-- amount of bits used by the operands & result
		size		: in  positive
	);
	port (
		-- system clock
		clk			: in  std_logic;
		-- system reset
		reset		: in  std_logic;
		-- start signal: a pulse on this signal initiates a multiplication
		--  operation (the multiplier responds with a pulse on ready when
		--  the operation is completed
		start		: in  std_logic;
		-- operand 1
		op1			: in  std_logic_vector(size-1 downto 0);
		-- operand 2
		op2			: in  std_logic_vector(size-1 downto 0);
		-- operands/result are signed (1=signed, 0=unsigned)
		signed	: in  std_logic;
		-- multiplier result (valid when ready is high)
		result	: out std_logic_vector(size-1 downto 0);
		-- overflow occured on multiplication (valid when ready is high)
		overflow: out std_logic;
		-- ready, multiplier generates a pulse on the ready signal when
		--  the multiplication is completed
		ready		: out std_logic
	);
end entity;

architecture behaviour of multiplier is
	-- result of the multiplication
	signal mult_result : std_logic_vector(2*size-1 downto 0);
	-- all ones or zeroes, to detect overflow
	signal zeroes, ones : std_logic_vector(size downto 0);

	-- overflow subsignals
	signal of_32bit_signed, of_32bit_unsigned : std_logic;
begin
	result <= mult_result(size-1 downto 0);

	-- check if high 32 bit range is used
	zeroes <= (others => '0');
	ones <= (others => '1');

	-- when unsigned multiplication: all upper word bits must be 0
	of_32bit_unsigned <= 
		'0' when (mult_result(2*size-1 downto size) = zeroes(size-1 downto 0)) else '1';
	-- when signed multiplication: the upper word bits must be equal to the
	--		sign bit (result msb)
	of_32bit_signed <= 
		'0' when (mult_result(2*size-1 downto size-1) = zeroes) else 
		'0' when (mult_result(2*size-1 downto size-1) = ones) else 
		'1';

	-- combined overflow signal
	overflow <= of_32bit_unsigned when signed = '0' else
		of_32bit_signed;

	-- multiplier pipeline
	process(clk, reset, start, mult_result, op1, op2) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				ready <= '0';
				mult_result <= (others => '0');
			else
				mult_result <= op1*op2;
				ready <= start;
			end if;
		end if;
	end process;

end architecture;