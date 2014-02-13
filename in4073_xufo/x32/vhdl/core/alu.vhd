-------------------------------------------------------------------------------
-- Filename:             alu.vhd
-- Entity:               alu
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          ALU top level file, instantiates all alu sub-
--                       components such as divider, multiplier etc.
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
use work.types.all;

entity alu is
	generic (
		-- operand & result size (bits)
		size				: positive
	);
	port (
		-- system clock signal
		clk					: in  std_logic;
		-- system reset signal
		reset				:	in  std_logic;
		-- start doing arithmetic operation, a pulse on start will cause a pulse 
		-- on ready. From the moment start is high, op1 and op2 must contain valid 
		-- data until ready is made high
		start				: in  std_logic;
		-- left operand (unsigned or 2's complement)
		op1					: in  std_logic_vector(size-1 downto 0);
		-- right operand (unsigned or 2's complement)
		op2					: in  std_logic_vector(size-1 downto 0);
		-- if signed is one, op1 and op2 will be treated as 2's complement, if not,
		-- they are assumed to be unsigned
		signed			: in  std_logic;
		-- type of arithmetic operation, see types.vhd (ALU_OPCODE_*)
		opcode			: in  std_logic_vector(3 downto 0);
		-- the result of the arithetic operation, the result is only valid at the
		-- moment ready is high
		result			: out std_logic_vector(size-1 downto 0);
		-- overflow occured (valid when ready is high)
		overflow		: out std_logic;
		-- division by zero occured (valid when ready is high)
		div0				: out std_logic;
		-- operation is ready (1 cycle pulse)
		ready 			: out std_logic
	);
end entity;

architecture behaviour of alu is
	-- different results of different opcodes
	signal multresult 		: std_logic_vector(size-1 downto 0);
	signal divresult 			: std_logic_vector(size-1 downto 0);
	signal modresult 			: std_logic_vector(size-1 downto 0);
	signal lshiftresult 	: std_logic_vector(size-1 downto 0);
	signal rshiftresult 	: std_logic_vector(size-1 downto 0);
	signal addsub_result : std_logic_vector(size-1 downto 0);
	signal zero 					: std_logic_vector(size-1 downto 0);
	
	-- different start & ready signals for different opcodes
	signal multready, divready : std_logic;
	signal multstart, divstart : std_logic;
	-- overflow & div0 for multiplier & divider
	signal multoverflow : std_logic;
	signal div_div0 : std_logic;
	
	-- buffered operand signals (spartan 3 is not able to do most alu operations
	-- + routing within one clockcycle, the inputs are buffered
	signal operand1, operand2 : std_logic_vector(size-1 downto 0);
	-- buffered starting signal (see above)
	signal delayed_start : std_logic;	
	-- addsub (used as one component signals, the spartan 3 supports addsub 
	-- components), if addsub_sub is high, the addsub component is used as
	-- subtractor, otherwise as adder. The addsub_overflow indicates an 
	-- overflow
	signal addsub_sub, addsub_overflow : std_logic;
begin
	-- Component instances:

	-- Adder/subtractor:
	addsub_sub <= '1' when opcode = ALU_OPCODE_SUB else '0';
	l_addsub: entity work.addsub(behaviour)
		generic map (
			size => size
		)
		port map (
			op1 => operand1,
			op2	=> operand2,
			signed => signed,
			sub => addsub_sub,
			result => addsub_result,
			overflow => addsub_overflow
		);

	-- Multiplier:
	l_multiplier: entity work.multiplier(behaviour)
		generic map (
			size => size
		)
		port map (
			clk => clk,
			reset => reset,
			start => multstart,
			op1 => operand1,
			op2 => operand2,
			signed => signed,
			result => multresult,
			overflow => multoverflow,
			ready => multready
		);

	-- Divider:
	l_divider: entity work.divider(behaviour)
		generic map (
			size => size
		)
		port map (
			clk => clk,
			reset => reset,
			start => divstart,
			op1 => operand1,
			op2 => operand2,
			signed => signed,
			result => divresult,
			remainder => modresult,
			div0 => div_div0,
			ready => divready
		);

	-- Left shifter		
	l_leftshifter: entity work.left_shifter(behaviour)
		generic map (
			size => size
		)
		port map (
			data => operand1,
			shift => operand2,
			signed => signed,
			result => lshiftresult
		);

	-- Right shifter
	l_rightshifter: entity work.right_shifter(behaviour)
		generic map (
			size => size
		)
		port map (
			data => operand1,
			shift => operand2,
			signed => signed,
			result => rshiftresult
		);

	-- result MUX
	result <= 	
		addsub_result 				when opcode = ALU_OPCODE_ADD else
		addsub_result 				when opcode = ALU_OPCODE_SUB else
		operand1 and operand2 when opcode = ALU_OPCODE_BAND else
		operand1 or operand2 	when opcode = ALU_OPCODE_BOR else
		operand1 xor operand2 when opcode = ALU_OPCODE_BXOR else
		operand1 							when opcode = ALU_OPCODE_OP1 else
		operand2 							when opcode = ALU_OPCODE_OP2 else
		-- TODO: route through addsub component (may overflow)
		zero-operand1 				when opcode = ALU_OPCODE_NEG else
		not operand1 					when opcode = ALU_OPCODE_NOT else
		lshiftresult 					when opcode = ALU_OPCODE_LSH else
		rshiftresult 					when opcode = ALU_OPCODE_RSH else
		multresult 						when opcode = ALU_OPCODE_MUL else
		divresult 						when opcode = ALU_OPCODE_DIV else
		modresult 						when opcode = ALU_OPCODE_MOD else
		(others => '-');
	
	-- ready signal MUX
	ready <= 
		multready 						when opcode = ALU_OPCODE_MUL else
		divready 							when opcode = ALU_OPCODE_DIV else
		divready 							when opcode = ALU_OPCODE_MOD else
		delayed_start;

	-- division by zero mux
	div0 <= 
		div_div0 							when opcode = ALU_OPCODE_DIV else
		div_div0 							when opcode = ALU_OPCODE_MOD else
		'0';

	-- overflow mux
	overflow <= 
		addsub_overflow 			when opcode = ALU_OPCODE_ADD else
		addsub_overflow 			when opcode = ALU_OPCODE_SUB else
		multoverflow 					when opcode = ALU_OPCODE_MUL else
		'0';

	-- start division signal		
	divstart <= '1' when delayed_start = '1' and 
		(opcode = ALU_OPCODE_DIV or opcode = ALU_OPCODE_MOD) else '0';
	-- start multiplication signal
	multstart <= '1' when delayed_start = '1' and 
		opcode = ALU_OPCODE_MUL else '0';
	
	-- zero constant
	zero <= (others => '0');
	
	-- buffer register for start, operand1 and operand2, these signals are 
	-- buffered, since the spartan 3 is not fast enough to do the routing + alu 
	-- operations within one clockcycle. The buffer is placed on "main" alu 
	-- component to simplify the sub blocks (multiplier, shifter, etc) and is 
	-- placed on the input to make sure the alu timing does not depend on any
	-- logic on the alu input.
	process(clk, reset, op1, op2, start) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				operand1 <= (others => '0');
				operand2 <= (others => '0');
				delayed_start <= '0';
			else
				operand1 <= op1;
				operand2 <= op2;
				delayed_start <= start;
			end if;
		end if;
	end process;
end architecture;
