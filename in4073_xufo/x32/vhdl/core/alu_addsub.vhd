-------------------------------------------------------------------------------
-- Filename:             alu_addsub.vhd
-- Entity:               addsub
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          ALU component which takes care of addition and 
--                       subtraction. The component also checks for overflow.
--                       The component uses the standard vhdl libraries to
--                       implement addition/subtraction, as most synthesizers
--                       (and fpga's) have support for standard addsub
--                       components
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

entity addsub is
	generic (
		-- size of operands & result (bits)
		size			: in  positive
	);
	port (
		-- left operand (unsigned or 2's complement)
		op1				: in  std_logic_vector(size-1 downto 0);
		-- right operand (unsigned or 2's complement)
		op2				: in  std_logic_vector(size-1 downto 0);
		-- if signed is one, op1 and op2 will be treated as 2's complement, if not,
		-- they are assumed to be unsigned
		signed		: in  std_logic;
		-- if sub is high, the result is op1-op2, otherwise op1+op2
		sub				: in  std_logic;
		-- result of the operation
		result		: out std_logic_vector(size-1 downto 0);
		-- overflow occured
		overflow	: out std_logic
	);
end entity;

architecture behaviour of addsub is
	-- internal result & operand signals (1 bit larger to get the carry out, the
	-- standard addsub component of the spartan does not generate a carry out)
	signal addsub_result : std_logic_vector(size downto 0);
	signal addsub_op1 : std_logic_vector(size downto 0);
	signal addsub_op2 : std_logic_vector(size downto 0);
	-- carry out of the addsub
	signal carry_out : std_logic;
	-- signs of the operands and the result
	signal sign_op1, sign_op2, sign_result : std_logic;
	-- all overflow type signals
	signal overflow_unsigned, overflow_signed_addition, 
		overflow_signed_subtraction : std_logic;
begin
	-- sign signals
	sign_op1 <= signed and op1(size-1);
	sign_op2 <= signed and op2(size-1);

	-- generate "larger" operand signals (sign extend):
	addsub_op1(size) <= sign_op1;
	addsub_op1(size-1 downto 0) <= op1;
	addsub_op2(size) <= signed and op2(size-1);
	addsub_op2(size-1 downto 0) <= op2;
	-- the addsub component
	addsub_result <= addsub_op1+addsub_op2 when sub = '0' else 
		addsub_op1-addsub_op2;
	-- carry out is result msb
	carry_out <= addsub_result(size);
	sign_result <= signed and addsub_result(size-1);
	-- result does not contain carry out
	result <= addsub_result(size-1 downto 0);

	-- unsigned: carry out = overflow
	overflow_unsigned <= carry_out;
	-- signed: pos + pos => neg = overflow
	-- signed: neg + neg => pos = overflow
	overflow_signed_addition <= 
		(not sign_op1 and not sign_op2 and sign_result) or
		(sign_op1 and sign_op2 and not sign_result);
	-- signed: pos - neg => neg = overflow
	-- signed: neg - pos => pos = overflow
	overflow_signed_subtraction <=
		(not sign_op1 and sign_op2 and sign_result) or
		(sign_op1 and not sign_op2 and not sign_result);
	-- overflow mux
	overflow <= 
		overflow_unsigned when signed = '0' else
		overflow_signed_addition when signed = '1' and sub = '0' else
		overflow_signed_subtraction when signed = '1' and sub = '1';
end architecture;