-------------------------------------------------------------------------------
-- Filename:             core.vhd
-- Entity:               x32_core
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          X32 Core toplevel. This component connects all X32
--                       sub-blocks (controller, decoder, registers, alu, etc.)
--                       together.
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
use ieee.std_logic_arith.all;
use work.types.all;

entity x32_core is
	generic (
		-- sizes on which the core operates, only 16/32/32/32 is tested.
		-- note, long>=integer>=short, and long>=16 
		size_short							: positive;
		size_integer						: positive;
		size_long								: positive;
		size_pointer						: positive;
		-- use a simple cache to hold the top of the stack (+8% performance)
		-- cache isn't really big, always keep it enabled unless for testing/
		-- debugging
		use_buffer							: boolean
	);
	port (
		-- system clock
		clk											: in	std_logic;
		-- system reset
		reset										: in	std_logic;
		
		-- memory address bus
		mem_addr								: out	std_logic_vector(size_pointer-1 downto 0);
		-- memory data input
		mem_data_in							: in	std_logic_vector(size_long-1 downto 0);
		-- memory data output
		mem_data_out 						: out	std_logic_vector(size_long-1 downto 0);
		-- memory read (high pulse starts a read operation)
		mem_read								: out	std_logic;
		-- memory write (high pulse starts a write operation)
		mem_write 							: out	std_logic;
		-- memory data is signed (1=signed, 0=unsigned)
		mem_data_signed					: out	std_logic;
		-- memory data size (see types.vhd)
		mem_data_size						: out	std_logic_vector(2 downto 0);
		-- memory operation completed (high pulse after each read/write
		-- operation)
		mem_ready 							: in	std_logic;

		-- interrupt the proc (must remain high until high pulse on interrupt_ack)
		interrupt								: in  std_logic;
		-- interrupt acknowledge (high pulse after each interrupt raise)
		interrupt_ack						: out std_logic;
		-- address of interrupt vector (must be valid between interrupt and
		-- interrupt_acl)
		interrupt_address				: in  std_logic_vector(size_pointer-1 downto 0);
		-- interrupt priority (must be valid between interrupt and interrupt_ack)
		-- note, the core does NOT check this value, if it's lower than the current
		-- priority, the current priority is simply lowered.
		interrupt_level					: in  std_logic_vector(size_long-1 downto 0);
		-- current priority (must be valid between interrupt and interrupt_ack)
		execution_level					: out std_logic_vector(size_long-1 downto 0);

		-- start executing from here (after reset)
		app_start								: in  std_logic_vector(size_pointer-1 downto 0);
		-- start stack here (after reset)
		stack_start							: in  std_logic_vector(size_pointer-1 downto 0);
		-- amount of instructions executed
		instruction_counter			: out std_logic_vector(size_long-1 downto 0);

		-- in execute stage (debug)
		execute									: out std_logic;
		-- overflow exception
		overflow								: out std_logic;
		-- division by zero exception
		division_by_zero				: out std_logic;
		-- core is trapped (won't continue unless interrupted)
		trapped									: out std_logic
	);
end entity;

architecture behaviour of x32_core is
	component decoder is
		port (
			clk							: in  std_logic;
			reset						: in  std_logic;
			
			instruction 		: in  std_logic_vector(INSTR_SIZE-1 downto 0);
	
			size						: out std_logic_vector(2 downto 0);
			signed					: out std_logic;
			
			p_descriptor		: out std_logic_vector(1 downto 0);
			o1_descriptor		: out std_logic_vector(1 downto 0);
			o2_descriptor		: out std_logic;
			g_descriptor		: out std_logic_vector(1 downto 0);
			
			is_call					: out std_logic;
			is_ret					: out std_logic;
			is_argstack			: out std_logic;
			is_varstack			: out std_logic;
			is_compare			: out std_logic;
			is_fetch				: out std_logic;
			--is_regstore			: out std_logic;
			is_conversion		: out std_logic;
			is_trap					: out std_logic;
			is_jump					: out std_logic;
			
			ex_alu_opcode		: out ALU_OPCODE_SELECT_TYPE;
			ex_alu_in_1			: out ALU_IN_1_SELECT_TYPE;
			ex_alu_in_2			: out ALU_IN_2_SELECT_TYPE;
			
			opcode					: out std_logic_vector(3 downto 0)
		);
	end component;
	component controller is
		port (
			clk										: in  std_logic;
			reset									: in  std_logic;
	
			p_descriptor					: in  std_logic_vector(1 downto 0);
			o1_descriptor					: in  std_logic_vector(1 downto 0);
			o2_descriptor					: in  std_logic;
			g_descriptor					: in  std_logic_vector(1 downto 0);
			
			is_ret								: in  std_logic;
			is_call								: in  std_logic;
			is_argstack						: in  std_logic;
			is_varstack						: in  std_logic;
			is_fetch							: in  std_logic;
			is_conversion					: in  std_logic;
			is_trap								: in  std_logic;
			is_jump								: in  std_logic;
			is_positive_compare		: in  std_logic;

			mem_addr_select				: out REG_SELECT_TYPE;
			mem_data_select				: out REG_SELECT_TYPE;
			mem_data_size_select	: out MEM_DATA_SIZE_SELECT_TYPE;
			
			cra_op1_select				: out CRA_OP1_TYPE;
			cra_op2_select				: out CRA_OP2_TYPE;
			cra_sub								: out std_logic;

			alu_ready							: in  std_logic;
	
			mem_read							: out std_logic;
			mem_write							: out std_logic;
			memory_ready					: in  std_logic;
	
			interrupt							: in  std_logic;
			start_interrupt				: out std_logic;
			finish_interrupt			: out std_logic;
			interrupting					: in  std_logic;

			validate_buffer				: out std_logic;
			invalidate_buffer			: out std_logic;
			isvalid_buffer				: in  std_logic;

			pc_store 							: out std_logic;
			sp_store 							: out std_logic;
			fp_store 							: out std_logic;
			ap_store 							: out std_logic;
			lp_store 							: out std_logic;
			el_store							: out std_logic;
			result_store					: out std_logic;
			instruction_store			: out std_logic;
			instruction_swap			: out std_logic;
			parameter_store				: out std_logic;
			operand1_store				: out std_logic;
			operand2_store				: out std_logic;
	
			executestage					: out std_logic;
			alu_execute						: out std_logic;
			internal_reset				: out std_logic;
			is_trapped						: out std_logic
		);
	end component;
	component comperator is
		generic(
			size : positive
		);
		port(
			signed	: in  std_logic;
			opcode	: in  std_logic_vector(3 downto 0);
			op1		: in  std_logic_vector(size_long-1 downto 0);
			op2		: in  std_logic_vector(size_long-1 downto 0);

			result	: out std_logic
		);
	end component;

	-- data register file signals:
	signal instruction_store	: std_logic;
	signal instruction_swap		: std_logic;
	signal parameter_store		: std_logic;
	signal operand1_store		: std_logic;
	signal operand2_store		: std_logic;
	signal instruction			: std_logic_vector(INSTR_SIZE-1 downto 0);
	signal parameter				: std_logic_vector(size_long-1 downto 0);
	signal operand1				: std_logic_vector(size_long-1 downto 0);
	signal operand2				: std_logic_vector(size_long-1 downto 0);

	signal instruction_reg_store: std_logic;
	signal instruction_reg_data	: std_logic_vector(INSTR_SIZE-1 downto 0);
	
	-- control register file signals:
-- 	signal pc_store_fsm 				: std_logic;
-- 	signal sp_store_fsm 				: std_logic;
-- 	signal fp_store_fsm 				: std_logic;
-- 	signal ap_store_fsm 				: std_logic;
-- 	signal lp_store_fsm					: std_logic;
-- 	signal el_store_fsm					: std_logic;
-- 	signal pc_store_opc 				: std_logic;
-- 	signal sp_store_opc 				: std_logic;
-- 	signal fp_store_opc 				: std_logic;
-- 	signal ap_store_opc 				: std_logic;
-- 	signal lp_store_opc					: std_logic;
-- 	signal el_store_opc					: std_logic;
	signal pc_store 						: std_logic;
	signal sp_store 						: std_logic;
	signal fp_store 						: std_logic;
	signal ap_store 						: std_logic;
	signal lp_store							: std_logic;
	signal el_store_fsm					: std_logic;
	signal el_store							: std_logic;
	signal result_store					: std_logic;
	signal pc										: std_logic_vector(size_pointer-1 downto 0);
	signal sp										: std_logic_vector(size_pointer-1 downto 0);
	signal fp										: std_logic_vector(size_pointer-1 downto 0);
	signal ap										: std_logic_vector(size_pointer-1 downto 0);
	signal lp										: std_logic_vector(size_pointer-1 downto 0);
	signal el										: std_logic_vector(size_pointer-1 downto 0);
	signal result								: std_logic_vector(size_long-1 downto 0);
	signal control_reg_in				: std_logic_vector(size_pointer-1 downto 0);
	signal el_in								: std_logic_vector(size_pointer-1 downto 0);
	
	-- internal reset signal (controlled by fsm)
	signal register_reset			: std_logic;
	signal execute_stage			: std_logic;

	signal start_interrupt		: std_logic;
	signal finish_interrupt		: std_logic;
	signal interrupting				: std_logic;

	-- buffer validation signals
	signal validate_buffer			: std_logic;
	signal invalidate_buffer		: std_logic;
	signal isvalid_buffer				: std_logic;


	-- alu signals
	signal alu_in_1				: std_logic_vector(size_long-1 downto 0);
	signal alu_in_2				: std_logic_vector(size_long-1 downto 0);
	signal alu_opcode			: std_logic_vector(3 downto 0);
	signal opcode_reg			: std_logic_vector(size_long-1 downto 0);
	signal alu_result			: std_logic_vector(size_long-1 downto 0);
	signal alu_execute		: std_logic;
	signal alu_ready			: std_logic;
	signal alu_overflow		: std_logic;
	signal alu_div0				: std_logic;

	-- decoded instruction signals
	signal opcode					: std_logic_vector(3 downto 0);
	signal signed					: std_logic;
	signal operand_size		: std_logic_vector(2 downto 0);
	signal operand_byte_size: std_logic_vector(size_long-1 downto 0);
	signal p_descriptor		: std_logic_vector(1 downto 0);
	signal o1_descriptor	: std_logic_vector(1 downto 0);
	signal o2_descriptor	: std_logic;
	signal g_descriptor		: std_logic_vector(1 downto 0);
	signal is_call				: std_logic;
	signal is_ret					: std_logic;
	signal is_argstack		: std_logic;
	signal is_varstack		: std_logic;
	signal is_compare			: std_logic;
	signal is_fetch				: std_logic;
	--signal is_regstore		: std_logic;
	signal is_conversion	: std_logic;
	signal is_trap				: std_logic;
	signal is_jump				: std_logic;
	signal is_positive_compare	: std_logic;
	signal ex_alu_opcode	: ALU_OPCODE_SELECT_TYPE;
	signal ex_alu_op1			: ALU_IN_1_SELECT_TYPE;
	signal ex_alu_op2			: ALU_IN_2_SELECT_TYPE;
	
	-- memory read/write signals
	signal i_mem_read			: std_logic;
	signal i_mem_write		: std_logic;
	signal i_mem_data_out : std_logic_vector(size_long-1 downto 0);
	signal i_mem_data_size: std_logic_vector(2 downto 0);
	signal mem_data_bus		: std_logic_vector(size_long-1 downto 0);
	
	signal compare_positive		: std_logic;

	-- neg->unsigned/unsigned->neg
	signal sign_conversion_overflow : std_logic;

	-- control register adder
	signal cra_out 						: std_logic_vector(size_pointer-1 downto 0);
	signal cra_op1 						: std_logic_vector(size_pointer-1 downto 0);
	signal cra_op2						: std_logic_vector(size_pointer-1 downto 0);

	-- mux control signals
	signal cra_op1_select			: CRA_OP1_TYPE;
	signal cra_op2_select			: CRA_OP2_TYPE;
	signal cra_sub						: std_logic;
	
	signal mem_addr_select		: REG_SELECT_TYPE;
	signal mem_data_select		: REG_SELECT_TYPE;
	signal mem_data_size_select: MEM_DATA_SIZE_SELECT_TYPE;

	signal instr_cntr					: std_logic_vector(size_long-1 downto 0);
	signal version						: std_logic_vector(size_long-1 downto 0);
	signal call_instruction		: std_logic_vector(size_long-1 downto 0);

	signal ones, zeroes 			: std_logic_vector(size_long-1 downto 0);
begin
	ones <= (others => '1');
	zeroes <= (others => '0');

	-- memory signals
	mem_read <= i_mem_read;
	mem_write <= i_mem_write;
	
	-- echo the memory from output to input when desired, allowing reading
	--		and writing at the same time (as long as the address is the same)
	--		this can be synthesized very easily when the memory bus is a real
	--		bus using tristate registers, however, unfortunately, the spartan
	--		3 does not support internal tristate registers, and the following
	--		code will therefore synthesize into a mux
	mem_data_bus <= i_mem_data_out when i_mem_read = '0' else mem_data_in;
	mem_data_out <= i_mem_data_out;
	-- take memory data signed value from the decoder, the signed signal
	--	is not valid when fetching instructions, but they may safely be
	-- sign extended since only the lower bits are used.
	mem_data_signed <= signed;

	-- interrupt acknowledge
	interrupt_ack <= finish_interrupt;
	-- execution level
	execution_level <= el;
	-- in execute stage
	execute <= execute_stage;
	-- overflow, either from the alu (only valid when alu_ready), or from conversions
	-- alu overflow only occures on "real" alu operations (ex_alu_opcode = T_OPCODE)
	overflow <= 
		alu_overflow when ex_alu_opcode = T_OPCODE and alu_ready = '1' else 
		sign_conversion_overflow when is_conversion = '1' and instruction_swap = '1' else 
		'0';

	-- division by zero from alu (only value when alu_ready)
	division_by_zero <= alu_ready and alu_div0;
	
	-- conversions from signed to unsigned generate overflow when msb is 1
	sign_conversion_overflow <= operand1(31) and (opcode(0) xor signed);

	-- store new execution level on interrupt acknowledge or when the
	-- fsm wants it to store (function returns)
 	el_store <= el_store_fsm or finish_interrupt;

	-- is executing a compare, and the compare evaluates to true
	is_positive_compare <= (is_compare and compare_positive);

	-- input of the control registers (pc, el, sp, ap, etc), comes from the CRA,
	--  unless in the execute_stage (then from alu)
	control_reg_in(size_pointer-1 downto 0) <= 
		alu_result(size_pointer-1 downto 0) when execute_stage = '1' else cra_out;
	-- execution level input may also come from the interrupt_level processor input
	el_in <= interrupt_level when finish_interrupt = '1' else 
		control_reg_in;

	-- upper bits of control_reg_in (if sizeof(long) > sizeof(void*))
	control_reg_in(size_long-1 downto size_pointer) <= 
		alu_result(size_long-1 downto size_pointer) when execute_stage = '1' else 
		(others => '0');

	-- result register
	l_result_reg: entity work.reg(behaviour)
		generic map (
			size_long
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => result_store,
			data_in => control_reg_in,
			data_out => result
		);

	-- PC register, reset to app_start input
	l_pc_reg: entity work.reg_init(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => pc_store,
			data_in => control_reg_in(size_pointer-1 downto 0),
			data_out => pc,
			init_val => app_start
		);

	-- EL register
	l_el_reg: entity work.reg(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => el_store,
			data_in => el_in,
			data_out => el
		);

	-- SP register (reset to stack_start input)
	l_sp_reg: entity work.reg_init(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => sp_store,
			data_in => control_reg_in(size_pointer-1 downto 0),
			data_out => sp,
			init_val => stack_start
		);

	-- AP register (reset to stack_start input)
	l_ap_reg: entity work.reg_init(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => ap_store,
			data_in => control_reg_in(size_pointer-1 downto 0),
			data_out => ap,
			init_val => stack_start
		);

	-- LP register (reset to stack_start input)
	l_lp_reg: entity work.reg_init(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => lp_store,
			data_in => control_reg_in(size_pointer-1 downto 0),
			data_out => lp,
			init_val => stack_start
		);

	-- FP register (reset to stack_start input)
	l_fp_reg: entity work.reg_init(behaviour)
		generic map (
			size_pointer
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => fp_store,
			data_in => control_reg_in(size_pointer-1 downto 0),
			data_out => fp,
			init_val => stack_start
		);

	-- instruction parameter register
	l_parameter_reg: entity work.reg(behaviour)
		generic map (
			size_long
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => parameter_store,
			data_in => mem_data_bus,
			data_out => parameter
		);

	-- operand 1 register
	l_operand1_reg: entity work.reg(behaviour)
		generic map (
			size_long
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => operand1_store,
			data_in => mem_data_bus,
			data_out => operand1
		);

	-- operand 2 register
	l_operand2_reg: entity work.reg(behaviour)
		generic map (
			size_long
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => operand2_store,
			data_in => mem_data_bus,
			data_out => operand2
		);

	-- instruction register
	l_instruction_reg: entity work.reg(behaviour)
		generic map (
			INSTR_SIZE
		)
		port map (
			clk => clk,
			reset => register_reset,
			store => instruction_reg_store,
			data_in => instruction_reg_data,
			data_out => instruction
		);

	-- save instruction register, either when reading the instruction from
	-- memory, or when swapping the opcode & type/size bits
	instruction_reg_store <= instruction_store or instruction_swap;
	
	-- instruction register input (either from memory, or swapped)
	instruction_reg_data(15 downto 8) <= mem_data_bus(15 downto 8) when 
		instruction_swap = '0' else instruction(15 downto 8);
	instruction_reg_data(7 downto 4) <= mem_data_bus(7 downto 4) when 
		instruction_swap = '0' else instruction(3 downto 0);
	instruction_reg_data(3 downto 0) <= mem_data_bus(3 downto 0) when 
		instruction_swap = '0' else instruction(7 downto 4);

	-- instruction decoder
	l_decoder: decoder port map(clk, reset, instruction, operand_size, signed,
											p_descriptor, o1_descriptor, o2_descriptor, g_descriptor, is_call, is_ret,
											--is_argstack, is_varstack, is_compare, is_fetch, is_regstore, is_conversion,
											is_argstack, is_varstack, is_compare, is_fetch, is_conversion,  
											is_trap, is_jump, ex_alu_opcode, ex_alu_op1, ex_alu_op2, opcode);

	-- size of operand in bytes
	operand_byte_size <= 
		conv_std_logic_vector(1, size_long) 
			when instruction(3 downto 1) = VARTYPE_CHAR else
		conv_std_logic_vector(size_short/8, size_long) 
			when instruction(3 downto 1) = VARTYPE_SHORT else
		conv_std_logic_vector(size_integer/8, size_long) 
			when instruction(3 downto 1) = VARTYPE_INT else
		conv_std_logic_vector(size_long/8, size_long) 
			when instruction(3 downto 1) = VARTYPE_LNG else
		conv_std_logic_vector(size_pointer/8, size_long)	
			when instruction(3 downto 1) = VARTYPE_PTR else
        (others => '0'); -- 20080911/MW: bugfix, for void returns, substract 0 from stack pointer

	-- ALU
	l_alu: entity work.alu(behaviour)
		generic map (
			size => size_long
		)
		port map (
			clk => clk,
			reset => reset,
			start => alu_execute,
			op1 => alu_in_1,
			op2 => alu_in_2,
			signed => signed,
			opcode => alu_opcode,
			result => alu_result,
			overflow => alu_overflow,
			div0 => alu_div0,
			ready => alu_ready
		);
	
	-- comparator
	l_comperator: comperator generic map(size_long) port map(signed, opcode, operand1, operand2, compare_positive);
	
	-- controller
	l_controller: controller port map(clk, reset, p_descriptor, o1_descriptor, 
																o2_descriptor, g_descriptor, is_ret, is_call,
																is_argstack, is_varstack,  is_fetch, is_conversion, 
																is_trap, is_jump, is_positive_compare, 
																mem_addr_select, mem_data_select, mem_data_size_select, 
																cra_op1_select, cra_op2_select, cra_sub, 
																alu_ready, i_mem_read, i_mem_write, mem_ready, 
																interrupt, start_interrupt, finish_interrupt, interrupting, 
																validate_buffer, invalidate_buffer, isvalid_buffer, 
--																pc_store_fsm, sp_store_fsm, fp_store_fsm, 
--																ap_store_fsm, lp_store_fsm, el_store_fsm, result_store,
																pc_store, sp_store, fp_store, 
																ap_store, lp_store, el_store_fsm, result_store,
																instruction_store, instruction_swap,
																parameter_store, operand1_store,
																operand2_store, execute_stage, alu_execute,
																register_reset, trapped);
	
	-- memory data size output
	mem_data_size <= i_mem_data_size;
	-- memory data size mux
	i_mem_data_size <= 
		operand_size when mem_data_size_select = T_OPERAND_SIZE else
		VARTYPE_PTR when mem_data_size_select = T_POINTER_SIZE else
		VARTYPE_CHAR when mem_data_size_select = T_CHAR_SIZE else
		VARTYPE_INSTRUCTION when mem_data_size_select = T_INSTR_SIZE else
		parameter(3 downto 1) when mem_data_size_select = T_PARAMETER else
		VARTYPE_LNG;
	

	-- memory address mux
	mem_addr <=
		pc when mem_addr_select = T_PC else
		sp when mem_addr_select = T_SP else
		lp when mem_addr_select = T_LP else
		fp when mem_addr_select = T_FP else
		ap when mem_addr_select = T_AP else
		result(size_pointer-1 downto 0) when mem_addr_select = T_RESULT else
		operand1(size_pointer-1 downto 0) when mem_addr_select = T_OPERAND1 else
		operand2(size_pointer-1 downto 0) when mem_addr_select = T_OPERAND2 else
		(others => '-');

	-- create call instruction (used when interrupting)
	call_instruction(size_long-1 downto INSTR_SIZE) <= (others => '-');
	call_instruction(INSTR_SIZE-1 downto 0) <= CALL_INSTRUCTION_OPCODE;

	-- memory data mux
	i_mem_data_out(size_pointer-1 downto 0) <=
		lp when mem_data_select = T_LP else
		fp when mem_data_select = T_FP else
		ap when mem_data_select = T_AP else
		el when mem_data_select = T_EL else
		result(size_pointer-1 downto 0) when mem_data_select = T_RESULT else
--		operand1(size_pointer-1 downto 0) when mem_data_select = T_OPERAND1 else
--		operand2(size_pointer-1 downto 0) when mem_data_select = T_OPERAND2 else
		interrupt_address(size_pointer-1 downto 0) when mem_data_select = T_INT_ADDRESS else
		call_instruction(size_pointer-1 downto 0) when mem_data_select = T_CALL else
		(others => '-');
	i_mem_data_out(size_long-1 downto size_pointer) <=
		result(size_long-1 downto size_pointer) when mem_data_select = T_RESULT else
--		operand1(size_long-1 downto size_pointer) when mem_data_select = T_OPERAND1 else
--		operand2(size_long-1 downto size_pointer) when mem_data_select = T_OPERAND2 else
		interrupt_address(size_long-1 downto size_pointer) when mem_data_select = T_INT_ADDRESS else
		call_instruction(size_long-1 downto size_pointer) when mem_data_select = T_CALL else
		(others => '0');
		
	-- alu input 1 mux
	alu_in_1(size_pointer-1 downto 0) <=
		operand1(size_pointer-1 downto 0) when ex_alu_op1 = T_OPERAND1 else
		--sp when ex_alu_op1 = T_SP else
		opcode_reg(size_pointer-1 downto 0) when ex_alu_op1 = T_REGISTER else
		(others => '0');
	alu_in_1(size_long-1 downto size_pointer) <=
		operand1(size_long-1 downto size_pointer) when ex_alu_op1 = T_OPERAND1 else
		opcode_reg(size_long-1 downto size_pointer) when ex_alu_op1 = T_REGISTER else
		(others => '0');
	alu_in_2 <=
		operand2 when ex_alu_op2 = T_OPERAND2 else
		parameter when ex_alu_op2 = T_PARAMETER else
		(others => '0');
	
	-- alu opcode mux
	alu_opcode <=
		ALU_OPCODE_ADD when ex_alu_opcode = T_ADD else opcode;
		
	-- version register is always 16 bit so it's compatible with x16 x32 x64 etc
	version(size_long-1 downto 16) <= (others => '0');
	version(15 downto 0) <= CORE_VERSION;
		
	-- opcode => register mux
	opcode_reg(size_long-1 downto size_pointer) <= (others => '0');
	opcode_reg(size_pointer-1 downto 0) <=
		sp when opcode = REG_OPCODE_SP else
		fp when opcode = REG_OPCODE_FP else
		lp when opcode = REG_OPCODE_LP else
		ap when opcode = REG_OPCODE_AP else
		pc when opcode = REG_OPCODE_PC else
		el when opcode = REG_OPCODE_EL else
		version when opcode = REG_OPCODE_VERS else
		(others => '0');

	-- control register adder
	cra_op1 <= 
		sp when cra_op1_select = T_CRA_OP1_SP else
		fp when cra_op1_select = T_CRA_OP1_FP else
		lp when cra_op1_select = T_CRA_OP1_LP else
		ap when cra_op1_select = T_CRA_OP1_AP else
		pc when cra_op1_select = T_CRA_OP1_PC else
		(others => '0');
	cra_op2 <= 
		operand_byte_size when cra_op2_select = T_CRA_OP2_OPERAND_SIZE else
		conv_std_logic_vector(INSTR_SIZE/8, size_long) when cra_op2_select = T_CRA_OP2_INSTRUCTION_SIZE else
		conv_std_logic_vector(size_pointer/8, size_long) when cra_op2_select = T_CRA_OP2_POINTER_SIZE else
		conv_std_logic_vector(size_pointer/8*5, size_long) when cra_op2_select = T_CRA_OP2_POINTER5_SIZE else
		parameter when cra_op2_select = T_CRA_OP2_PARAMETER else
		(others => '0');
	cra_out <= cra_op1 + cra_op2 when cra_sub = '0' else
		cra_op1 - cra_op2;
	
	-- instruction counter
	instruction_counter <= instr_cntr;
	process(clk, register_reset, execute_stage) begin
		if (clk'event and clk = '1') then
			if (register_reset = '1') then
				instr_cntr <= (others => '0');
			elsif (execute_stage = '1') then
				instr_cntr <= instr_cntr + 1;
			end if;
		end if;
	end process;
	
	-- buffer validation (SR-LATCH, when positive, the buffer holds
	-- the top of the stack)
	process(clk, reset, validate_buffer, invalidate_buffer) begin
		if (clk'event and clk = '1') then
			if (use_buffer) then
				if (reset = '1') then
					isvalid_buffer <= '0';
				elsif (invalidate_buffer = '1') then
					isvalid_buffer <= '0';
				elsif (validate_buffer = '1') then
					isvalid_buffer <= '1';
				end if;
			else
				isvalid_buffer <= '0';
			end if;
		end if;
	end process;

	-- busy with interrupt call (SR-LATCH):
	process(clk, reset, start_interrupt, finish_interrupt) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				interrupting <= '0';
			elsif (start_interrupt = '1') then
				interrupting <= '1';
			elsif (finish_interrupt = '1') then
				interrupting <= '0';
			end if;
		end if;
	end process;
	 
end architecture;
