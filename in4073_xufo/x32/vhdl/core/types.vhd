-------------------------------------------------------------------------------
-- Filename:             types.vhd
-- Entity:               -
-- Architectures:        -
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Library containing all X32 constants/enumerations
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

-- types package, contains all constants used in the X32
package types is
	-- constant placed in the core version register, which can be read using the
	--  SAVEVR instruction (SAVE means SAVE on stack, such that it can be read
	--  from the stack with a second instruction).
	-- 0100 is consideren 1.0
	constant CORE_VERSION 					: std_logic_vector(15 downto 0) := x"0100";
	-- call instruction (used for interrupts)
	constant CALL_INSTRUCTION_OPCODE: std_logic_vector(15 downto 0) := x"050C";
	-- size of an instruction. THIS MUST BE 16!!
	constant INSTR_SIZE							: positive := 16;

	-- ALU opcodes for each alu operation
	constant ALU_OPCODE_ADD		: std_logic_vector(3 downto 0) := "0001";
	constant ALU_OPCODE_BAND	: std_logic_vector(3 downto 0) := "0010";
	constant ALU_OPCODE_BOR		: std_logic_vector(3 downto 0) := "0011";
	constant ALU_OPCODE_BXOR	: std_logic_vector(3 downto 0) := "0100";
	constant ALU_OPCODE_DIV		: std_logic_vector(3 downto 0) := "0101";
	constant ALU_OPCODE_LSH		: std_logic_vector(3 downto 0) := "0110";
	constant ALU_OPCODE_MOD		: std_logic_vector(3 downto 0) := "0111";
	constant ALU_OPCODE_MUL		: std_logic_vector(3 downto 0) := "1000";
	constant ALU_OPCODE_RSH		: std_logic_vector(3 downto 0) := "1001";
	constant ALU_OPCODE_SUB		: std_logic_vector(3 downto 0) := "1010";
	constant ALU_OPCODE_NEG		: std_logic_vector(3 downto 0) := "1011";
	constant ALU_OPCODE_NOT		: std_logic_vector(3 downto 0) := "1100";
	constant ALU_OPCODE_OP1		: std_logic_vector(3 downto 0) := "1101";
	constant ALU_OPCODE_OP2		: std_logic_vector(3 downto 0) := "1110";

	-- comperator opcodes
	constant COMP_OPCODE_EQ		: std_logic_vector(3 downto 0) := "0001";
	constant COMP_OPCODE_GT		: std_logic_vector(3 downto 0) := "0010";
	constant COMP_OPCODE_GE		: std_logic_vector(3 downto 0) := "0011";
	constant COMP_OPCODE_LT		: std_logic_vector(3 downto 0) := "0100";
	constant COMP_OPCODE_LE		: std_logic_vector(3 downto 0) := "0101";
	constant COMP_OPCODE_NE		: std_logic_vector(3 downto 0) := "1000";
	
	-- register opcodes
	constant REG_OPCODE_SP		: std_logic_vector(3 downto 0) := "0001";
	constant REG_OPCODE_AP		: std_logic_vector(3 downto 0) := "0010";
	constant REG_OPCODE_LP		: std_logic_vector(3 downto 0) := "0100";
	constant REG_OPCODE_FP		: std_logic_vector(3 downto 0) := "1000";
	constant REG_OPCODE_NIL		: std_logic_vector(3 downto 0) := "0000";
	constant REG_OPCODE_GLBL	: std_logic_vector(3 downto 0) := "1100";
	constant REG_OPCODE_PC		: std_logic_vector(3 downto 0) := "0011";
	constant REG_OPCODE_VERS	: std_logic_vector(3 downto 0) := "0110";
	constant REG_OPCODE_EL		: std_logic_vector(3 downto 0) := "1010";
	
	-- variable types
	constant VARTYPE_VOID			: std_logic_vector(2 downto 0) := "000";
	constant VARTYPE_CHAR			: std_logic_vector(2 downto 0) := "001";
	constant VARTYPE_SHORT		: std_logic_vector(2 downto 0) := "010";
	constant VARTYPE_INT			: std_logic_vector(2 downto 0) := "011";
	constant VARTYPE_LNG			: std_logic_vector(2 downto 0) := "100";
	constant VARTYPE_PTR			: std_logic_vector(2 downto 0) := "110";
	constant VARTYPE_INSTRUCTION	: std_logic_vector(2 downto 0) := "111";

	-- mux input to overrule the alu opcode with addition 
	type ALU_OPCODE_SELECT_TYPE is (
		T_OPCODE, 	-- use opcode from instruction
		T_ADD				-- overrule with addition
	);
	-- alu left operand input
	type ALU_IN_1_SELECT_TYPE is (
		T_OPERAND1, 		-- use operand 1 register
		--T_SP, 					-- use stack pointer
		T_REGISTER, 		-- use a register (the instruction opcode denotes which one)
		T_ZERO					-- use zero
	);
	-- alu right operand input
	type ALU_IN_2_SELECT_TYPE is (
		T_OPERAND2, 		-- use operand 2 register
		T_PARAMETER,		-- use (instruction) parameter register
		T_ZERO					-- use zero
	);
	-- register selection
	type REG_SELECT_TYPE is (
		T_RESULT, 			-- result register
		T_SP, 					-- stack pointer register
		T_PC, 					-- program counter register
		T_AP, 					-- argument pointer register
		T_FP, 					-- frame pointer register
		T_LP, 					-- local pointer register
		T_EL, 					-- execution level register
		T_OPERAND1, 		-- operand 1 register
		T_OPERAND2, 		-- operand 2 register
		T_CALL, 				-- call instruction 
										--	(to force a call instruction on interrupt)
		T_INT_ADDRESS		-- interrupt address
	);
	-- memory bus data size
	type MEM_DATA_SIZE_SELECT_TYPE is (
		T_LNG, 					-- sizeof(long)
		T_INSTR_SIZE, 	-- size of an instruction (INSTR_SIZE)
		T_POINTER_SIZE, -- sizeof(char*)
		T_CHAR_SIZE, 		-- 8
		T_OPERAND_SIZE, -- size denoted by the instruction (eg 4*8 on "ADDI4")
		T_PARAMETER			-- size placed in parameter field of the instruction
	);
	-- control register adder left operand
	type CRA_OP1_TYPE is (
		T_CRA_OP1_PC, 							-- program counter
		T_CRA_OP1_SP, 							-- stack pointer
		T_CRA_OP1_AP, 							-- argument pointer
		T_CRA_OP1_LP, 							-- local pointer
		T_CRA_OP1_FP, 							-- frame pointer
		T_CRA_OP1_ZERO);						-- zero
	-- control register adder right operand
	type CRA_OP2_TYPE is (
		T_CRA_OP2_PARAMETER, 				-- parameter register
		T_CRA_OP2_INSTRUCTION_SIZE, -- size of an instruction (INSTR_SIZE)
		T_CRA_OP2_POINTER_SIZE, 		-- size of a pointer (sizeof(char*))
		T_CRA_OP2_POINTER5_SIZE, 		-- 5*sizeof(char*)
		T_CRA_OP2_ZERO, 						-- zero
		T_CRA_OP2_OPERAND_SIZE			-- size denoted by the instruction 
	);

	-- log2 (ceiling) function, used to compute the number of bits required to
	--	represent a constant number
	function log2_ceil(N: natural) return positive;
end package;

package body types is
	--- find minimum number of bits required to
	--- represent N as an unsigned binary number
	--- (written by Jonathan Bromley)
	function log2_ceil(N: natural) return positive is	begin
		if N < 2 then
			-- at least on bit
			return 1;
		else
			-- recursive
			return 1 + log2_ceil(N/2);
		end if;
	end;
end package body;
