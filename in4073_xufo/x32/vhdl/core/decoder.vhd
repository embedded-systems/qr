-------------------------------------------------------------------------------
-- Filename:             decoder.vhd
-- Entity:               decoder
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          LCC Bytecode decoder, decodes binary instructions into
--                       the signals required by the X32 (controller).
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
use work.types.all;

entity decoder is
	port (
		-- system clock
		clk							: in  std_logic;
		-- system reset
		reset						: in  std_logic;
		-- binary instruction to decode (16 bits), format:
		--		bit			description
		--		15-12		forbidden bits, any one of them being one causes a trap int
		--		11-8		index of decoder ROM (defines the instruction class)
		--		7-4			instruction opcode (eg add, sub, greater than, etc)
		--		3-1			instruction operand size, see types.vhd for valid values
		--		0				instruction operand signedness (1=signed, 0=unsigned)
		instruction 		: in  std_logic_vector(INSTR_SIZE-1 downto 0);

		-- decoded instruction operand size (see types.vhd for valid values)
		size						: out std_logic_vector(2 downto 0);
		-- decoded instruction operand signedness
		signed					: out std_logic;

		-- decoded parameter type (see below)
		p_descriptor		: out std_logic_vector(1 downto 0);
		-- decoded operand 1 type (see below)
		o1_descriptor		: out std_logic_vector(1 downto 0);
		-- decoded operand 2 type (see below)
		o2_descriptor		: out std_logic;
		-- decoded result type (see below)
		g_descriptor		: out std_logic_vector(1 downto 0);
		
		-- instruction class (only one can be '1')
		is_call					: out std_logic;
		is_ret					: out std_logic;
		is_argstack			: out std_logic;
		is_varstack			: out std_logic;
		is_compare			: out std_logic;
		is_fetch 				: out std_logic;
		is_conversion		: out std_logic;
		is_trap					: out std_logic;
		is_jump					: out std_logic;

		-- alu control during execute stage (see below)
		ex_alu_opcode		: out ALU_OPCODE_SELECT_TYPE;
		ex_alu_in_1			: out ALU_IN_1_SELECT_TYPE;
		ex_alu_in_2			: out ALU_IN_2_SELECT_TYPE;
		
		-- decoded instruction opcode
		opcode					: out std_logic_vector(3 downto 0)
	);
end entity;

architecture behaviour of decoder is
	-- the alu signals are buffered to improve clock freq 
	--	(the execute stage never directly follows the decode stage,
	--	thus buffering is possible)
	signal	ex_alu_opcode_i		: ALU_OPCODE_SELECT_TYPE;
	signal	ex_alu_in_1_i			: ALU_IN_1_SELECT_TYPE;
	signal	ex_alu_in_2_i			: ALU_IN_2_SELECT_TYPE;
	
	-- output of the ROM containing more detailed instruction description
	--		signals, this rom is indexed by a part of the binary instruction
	signal decoder_rom 				: std_logic_vector(20 downto 0);
begin
	-- DECODER TABLE OUTPUT (describe statemachine flow):
	-- bit    description
	-- 20-19	p (see below)
	-- 18-17	o1 (see below)
	-- 16			o2 (see below)
	-- 15-14	g (see below)
	-- 13-12	alu_in_2 (see below)
	-- 11-10	alu_in_1 (see below)
	-- 09			alu_opcode (see below)
	-- 08			is call instruction
	-- 07			is return instruction
	-- 06			is argstack instruction
	-- 05			is varstack instruction
	-- 04			is compare instruction
	-- 03			is fetch instruction
	-- 02			NO LONGER USED instruction
	-- 01			is conversion instruction 
	-- 00			is jump instruction
	--
	-- p/o1/o2/g fields (describe memory read/writes)
	-- p: instruction parameter descriptor
	--		00	no instruction parameter
	--		01	parameter with type/size defined by the instruction
	--		10	pointer parameter 
	--		11	<not used>
	-- o1: stack operand 1 descriptor
	--		00	no 1st operand (o2 must be 0 as well)
	--		01	1st operand with type/suze defined by the instruction
	--		10	pointer 1st operand
	--		11	<not used>
	-- o2: stack operand 2 descriptor
	--		0		no 2nd operand
	--		1		2nd operand with type/soze defined by the instruction
	-- g: instruction parameter descriptor
	--		00	do not save the result
	--		01	push the result to the stack
	--		10	write the result to a memory location (ASGN only)
	--		11	<not used>
	--
	-- alu control signals (used during execute stage)
	-- alu_in_1: controls the first alu operand input mux
	--		00	operand 1 register
	--		01	register defined by instruction opcode
	--		10	zero
	--		11	zero
	-- alu_in_2: controls the second alu operand input mux
	--		00	operand 2 register
	--		01	register defined by instruction opcode
	--		10	zero
	--		11	zero
	-- alu_opcode: controls the alu opcode input mux
	--		0		instruction opcode
	--		1		forced addition

	decoder_rom <= 
		-- COMPUTE ADDRESS (ADDRG, ADDRF, ADDRL, CNST)
		"010000101011000000000" when instruction(15 downto 8) = x"01" else
		-- ARITHMETIC (ADD, SUB, MUL, ...)
		"000110100000000000000" when instruction(15 downto 8) = x"03" else
		-- ONE OPERAND ARITHMETIC (COMP, NEG)
		"000100100000000000000" when instruction(15 downto 8) = x"04" else
		-- FUNCTION CALL (CALL, SYSCALL)
		"001000110001100000000" when instruction(15 downto 8) = x"05" else
		-- JUMP
		"001000000000000000001" when instruction(15 downto 8) = x"06" else
		-- FUNCTION RETURN (RET)
		"000100100000010000000" when instruction(15 downto 8) = x"07" else
		-- FETCH (INDIR)
		"001000100000000001000" when instruction(15 downto 8) = x"08" else
		-- STACKFRAME (ARGSTACK)
		"010000001011001000000" when instruction(15 downto 8) = x"09" else
		-- STACKFRAME (VARSTACK)
		"010000001011000100000" when instruction(15 downto 8) = x"0A" else
		-- COMPARE (EQ, GE, GT, LT, LE, NE)
		"010110001101000010000" when instruction(15 downto 8) = x"0C" else
		-- ASGN
		"001011000000000000000" when instruction(15 downto 8) = x"0D" else
		-- CV*
		"000100110001000000010" when instruction(15 downto 8) = x"0F" else
		-- others
		(others => '-');

	-- is_trap (or invalid opcode)
	-- this only leaves the 0x0B instruction as undetected invalid
	is_trap <= instruction(15) or instruction(14) or instruction(13) or instruction(12);

	opcode <= instruction(7 downto 4);
	size <= instruction(3 downto 1);
	signed <= instruction(0);

	is_call <= decoder_rom(8);
	is_ret <= decoder_rom(7);
	is_argstack <= decoder_rom(6);
	is_varstack <= decoder_rom(5);
	is_compare <= decoder_rom(4);
	is_fetch <= decoder_rom(3);
	--is_regstore <= decoder_rom(2);
	is_conversion <= decoder_rom(1);
	is_jump <= decoder_rom(0);

	p_descriptor <= decoder_rom(20 downto 19);
	o1_descriptor <= decoder_rom(18 downto 17);
	o2_descriptor <= decoder_rom(16);
	g_descriptor <= decoder_rom(15 downto 14);

	ex_alu_opcode_i <= T_OPCODE when decoder_rom(9) = '0' else T_ADD;
	ex_alu_in_1_i <= 
		T_OPERAND1 when decoder_rom(11 downto 10) = "00" else
		T_REGISTER when decoder_rom(11 downto 10) = "01" else
		T_ZERO; 
	ex_alu_in_2_i <= 
		T_OPERAND2 when decoder_rom(13 downto 12) = "00" else
		T_PARAMETER when decoder_rom(13 downto 12) = "01" else
		T_ZERO when decoder_rom(13 downto 12) = "10" else
		T_ZERO when decoder_rom(13 downto 12) = "11";
	
	-- buffer for alu opcode/operand signals to allow higher clock speeds
	--		(the ex_alu_* signals are not needed directly. the descriptor
	--		signals however, do need to be computed in the same cycle).
	process(clk, reset, ex_alu_opcode_i, ex_alu_in_1_i, ex_alu_in_2_i) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				ex_alu_opcode <= T_OPCODE;
				ex_alu_in_1 <= T_OPERAND1;
				ex_alu_in_2 <= T_OPERAND2;
			else
				ex_alu_opcode <= ex_alu_opcode_i;
				ex_alu_in_1 <= ex_alu_in_1_i;
				ex_alu_in_2 <= ex_alu_in_2_i;
			end if;
		end if;
	end process;							
end behaviour;
