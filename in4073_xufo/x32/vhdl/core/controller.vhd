-------------------------------------------------------------------------------
-- Filename:             controller.vhd
-- Entity:               controller
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          X32 controller, guides the X32 core through each 
--                       instruction. The controller controls the memory bus,
--                       and creates all register write enable signals, as well
--                       as most mux indices. The controller also handles
--                       function calls and interrupts.
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

entity controller is
	port (
		-- system clock
		clk										: in  std_logic;
		-- system reset
		reset									: in  std_logic;

		-- memory read/write description for the current instruction,
		--	see decoder.vhd for more information
		p_descriptor					: in  std_logic_vector(1 downto 0);
		o1_descriptor					: in  std_logic_vector(1 downto 0);
		o2_descriptor					: in  std_logic;
		g_descriptor					: in  std_logic_vector(1 downto 0);

		-- the current instruction is a return
		is_ret								: in  std_logic;
		-- the current instruction is a function call
		is_call								: in  std_logic;
		-- the current instruction is an argument stack creation
		is_argstack						: in  std_logic;
		-- the current instruction is an variable stack creation
		is_varstack						: in  std_logic;
		-- the current instruction is a memory fetch
		is_fetch							: in  std_logic;
		-- the current instruction is a variable conversion
		is_conversion					: in  std_logic;
		-- the current instruction is a trap
		is_trap								: in  std_logic;
		-- the current instruction is a jump
		is_jump								: in  std_logic;
		-- the current instruction is a compare and the result is positive
		--		(note, this signal is only valid in the EXECUTE stage)
		is_positive_compare		: in  std_logic;

		-- memory address source selection (see types.vhd)
		mem_addr_select				: out REG_SELECT_TYPE;
		-- memory data source selection (only when writing memory) (see types.vhd)
		mem_data_select				: out REG_SELECT_TYPE;
		-- memory data size source selection (see types.vhd)
		mem_data_size_select	: out MEM_DATA_SIZE_SELECT_TYPE;

		-- control register opcode 1 source (see types.vhd)
		cra_op1_select				: out CRA_OP1_TYPE;
		-- control register opcode 2 source (see types.vhd)
		cra_op2_select				: out CRA_OP2_TYPE;
		-- control register subtract (high = sub, low = add)
		cra_sub								: out std_logic;
		
		-- alu ready signal (pulses high when alu finishes)
		alu_ready							: in  std_logic;

		-- memory read (pulse high to start memory read cycle)
		mem_read							: out std_logic;
		-- memory write (pulse high to start memory write cycle)
		mem_write							: out std_logic;
		-- memory ready (pulses high when memory in is valid)
		memory_ready					: in  std_logic;

		-- interrupt request (generated by the interrupt controller, must remain
		--  high until start_interrupt goes high)
		interrupt							: in  std_logic;
		-- controller starts to handle the interrupt (which should be considered
		--  as an acknowledge by the interrupt controller
		start_interrupt				: out std_logic;
		-- controller finishes the "special" interrupt action, this means the
		--	contoller enters the interrupt handler. The hardware interrupt handler
		--	stops here. This does NOT mean the software interrupt handler is
		--	finished (the interrupt handler function). The hardware handler calls
		--	the software handler, and finishes after the call.
		finish_interrupt			: out std_logic;
		-- this should be high between the pulses of start_interrupt and
		--	finish_interrupt, it means the hardware is servicing the hardware
		--	part of the interrupt handler (again, NOT the software handler)
		interrupting					: in  std_logic;
		
		-- buffer signals, the buffer holds the top of the stack, which improves
		--	the speed of the processor. the valid register is high when the buffer
		--	holds the top of the stack, or low otherwise
		-- validate the buffer (set the valid register)
		validate_buffer				: out std_logic;
		-- invalidate the buffer (reset the valid register)
		invalidate_buffer			: out std_logic;
		-- current value of the valid register
		isvalid_buffer				: in  std_logic;

		-- store signals for all registers
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

		-- the following signals are high whenever the processor is in the execute
		--	stage. The difference is that execute_stage is only high the last cycle
		--  of the execute stage, while alu_execute is high during the entire stage
		executestage					: out std_logic;
		alu_execute						: out std_logic;
		-- internal reset => controller generated reset, does only require a reset
		--	on all registers, the system does not need a full reset (happens on
		--	code errors)
		internal_reset				: out std_logic;
		-- the processor is trapped (after a trap instruction). An interrupt must
		--	occure to "untrap" the processor.
		is_trapped						: out std_logic
	);
end entity;

architecture behaviour of controller is
	-- states of the controller:
	type CONTROLLER_STATES is (
		RESET_STATE, 					-- startup (reset) stage, resets all internal 
													--	registers
		INSTRUCTION_FETCH, 		-- reads an instruction from the memory
		INSTRUCTION_DECODE, 	-- decodes a read instruction
		PARAM_FETCH, 					-- reads a parameter from the memory
		OPERAND2_FETCH, 			-- reads 2nd operand from the stack
		OPERAND1_FETCH, 			-- reads 1st operand from the stack
		FETCH,								-- reads a value from the memory
		EXECUTE, 							-- executes an instruction (alu, comperator, etc)
		PUSH_RESULT, 					-- pushes the result to the stack
		WRITE_RESULT, 				-- writes the result to a memory address
		INTERRUPT_1, 					-- first interrupt stage
		INTERRUPT_2, 					-- second interrupt stage
		TRAPPED, 							-- trapped stage, when the processor is trapped,
													--	it can only continue when it is interrupted
		RESTORE, 							-- restore state (return from function)
		RESTORE_PC1, 					-- restore program counter (stage 1)
		RESTORE_PC2, 					-- restore program counter (stage 2)
		RESTORE_EL1, 
		RESTORE_EL2, 
		RESTORE_AP1, 
		RESTORE_AP2, 
		RESTORE_LP1, 
		RESTORE_LP2, 
		RESTORE_FP1, 
		RESTORE_FP2, 
		SAVERA, 							-- save return address (when saving processor
													--	state on function call)
		SAVEEL, 
		SAVEAP, 
		SAVELP, 
		SAVEFP,
		NEWFRAME1,						-- create new stackframe (update fp, ap, lp, etc), 
		NEWFRAME2							--	executed at the end of an interrupt
	);
	signal state_i, state_o : CONTROLLER_STATES;	
begin
	-- reset registers on RESET_STATE
	internal_reset <= '1' when state_o = RESET_STATE else '0';
	-- executestage on last cycle of execute stage
	executestage <= '1' when state_o = EXECUTE and alu_ready = '1' else '0';
	-- alu_execute during execute stage
	alu_execute <= '1' when state_o = EXECUTE else '0';
	-- instruction swap: swap opcode with operand type/size during a conversion
	--	instruction (opcode contains the new variable type/size)
	instruction_swap <= '1' when state_o = EXECUTE and is_conversion = '1' and 
		alu_ready = '1' else '0';
	-- is_trapped when TRAPPED
	is_trapped <= '1' when state_o = TRAPPED else '0';
	-- have the system know an interrupt is started when the processor enters the
	--	interrupt_2 stage
	start_interrupt <= '1' when state_o = INTERRUPT_2 else '0';
	-- have the system know an interrupt is finished when the processor enters the
	--	newframe state. Note that this is NOT at the end of an ISR, but at the end
	--	of the call to the isr
	finish_interrupt <= '1' when interrupting = '1' and 
		state_o = NEWFRAME1 else '0';
	
	-- fsm
	process(state_o, memory_ready, p_descriptor, g_descriptor, o1_descriptor, 
					o2_descriptor, is_call, is_trap, is_fetch, is_argstack, 
					is_varstack, is_ret, is_jump, is_positive_compare,
					alu_ready, interrupt, isvalid_buffer) begin
		case state_o is
			when RESET_STATE =>
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';
				
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';
				
				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				state_i <= INSTRUCTION_FETCH;
			when INSTRUCTION_FETCH =>
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read instruction from memory
				instruction_store <= '1';
				mem_addr_select <= T_PC;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_INSTR_SIZE;
				mem_read <= '1';
				mem_write <= '0';

				-- update PC
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_INSTRUCTION_SIZE;
				cra_sub <= '0';
				--pc_store <= memory_ready and not interrupt;
				pc_store <= memory_ready;

				if (memory_ready = '1') then
					if (interrupt = '1') then
						state_i <= INTERRUPT_1;
					else
						state_i <= INSTRUCTION_DECODE;
					end if;
				else 
					state_i <= INSTRUCTION_FETCH;
				end if;
			when INTERRUPT_1 =>
				-- CALLV => instruction register
				-- PC => result register
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				instruction_store <= '1';	-- store call in instruction
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';
				
				-- ra (=pc-2) => result register
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_INSTRUCTION_SIZE;
				cra_sub <= '1';
				
				result_store <= '1';
				
				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_CALL;
				mem_data_size_select <= T_LNG;

				state_i <= INTERRUPT_2;
			when INTERRUPT_2 =>
				-- isr addres => operand 1 register
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';	
				parameter_store <= '0';
				operand1_store <= '1'; -- store address in operand1
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';
				
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_INSTRUCTION_SIZE;
				cra_sub <= '0';
				
				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_INT_ADDRESS;
				mem_data_size_select <= T_LNG;

				-- proceed as normal
				state_i <= EXECUTE;
			when INSTRUCTION_DECODE =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- no memory reading done in this cycle
				mem_addr_select <= T_PC;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;
				mem_read <= '0';
				mem_write <= '0';

				-- calculate the new SP for popping operand1/2, save only
				-- when the operands actually need to be read (see below)
				cra_op1_select <= T_CRA_OP1_SP;
				if (is_fetch = '1') then
					cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				else
					cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
				end if;
				
				cra_sub <= '1';
			
				--check for trap & interrupt
				if (interrupt = '1') then
					state_i <= INTERRUPT_1;
					sp_store <= '0';
				else
					if (is_trap = '1') then
						state_i <= TRAPPED;
						sp_store <= '0';
					elsif (p_descriptor = "00") then
						-- no parameter
						if (o2_descriptor = '1') then
							-- operand 2
							state_i <= OPERAND2_FETCH;
							
							-- update sp
							sp_store <= '1';
						else 
							if (o1_descriptor = "00") then
								-- no operand 1
								state_i <= EXECUTE;
	
								-- no sp update required
								sp_store <= '0';
							else
								-- operand 1
								state_i <= OPERAND1_FETCH;
	
								-- update sp
								sp_store <= '1';
							end if;
						end if;
					else
						-- parameter
						state_i <= PARAM_FETCH;
						
						if (o1_descriptor = "00" and o2_descriptor = '0') then
							-- no sp update required
							sp_store <= '0';
						else
							-- update sp for operand1/2
							sp_store <= '1';
						end if;
					end if;
				end if;
			when PARAM_FETCH =>
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read parameter from memory
				parameter_store <= '1';
				mem_addr_select <= T_PC;
				mem_data_select <= T_RESULT;
				case p_descriptor is
					when "01" =>
						mem_data_size_select <= T_OPERAND_SIZE;
						cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
					when "10" =>
						mem_data_size_select <= T_POINTER_SIZE;
						cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
					when others =>
						-- error!
						mem_data_size_select <= T_OPERAND_SIZE;
						cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
				end case;
				mem_read <= '1';
				mem_write <= '0';
				
				-- update PC
				cra_op1_select <= T_CRA_OP1_PC;
				cra_sub <= '0';
				pc_store <= memory_ready;

				if (memory_ready = '1') then
					if (o2_descriptor = '1') then
						state_i <= OPERAND2_FETCH;
					else 
						if (o1_descriptor = "00") then
							state_i <= EXECUTE;
						else
							state_i <= OPERAND1_FETCH;
						end if;
					end if;
				else
					state_i <= PARAM_FETCH;
				end if;
			when OPERAND2_FETCH =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				-- read operand2 from stack
				operand2_store <= not isvalid_buffer;
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_OPERAND_SIZE;
				mem_read <= not isvalid_buffer;
				mem_write <= '0';
				
				-- may need to update sp (see if statement below)
				cra_op1_select <= T_CRA_OP1_SP;
				cra_sub <= '1';

				case o1_descriptor is
					when "01" =>
						cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
						sp_store <= memory_ready or isvalid_buffer;
						if (memory_ready = '1' or isvalid_buffer = '1') then
							state_i <= OPERAND1_FETCH;
						else
							state_i <= OPERAND2_FETCH;
						end if;
					when "10" =>
						cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
						sp_store <= memory_ready or isvalid_buffer;
						if (memory_ready = '1' or isvalid_buffer = '1') then
							state_i <= OPERAND1_FETCH;
						else
							state_i <= OPERAND2_FETCH;
						end if;
					when "11" =>
						cra_op2_select <= T_CRA_OP2_PARAMETER;
						sp_store <= memory_ready or isvalid_buffer;
						if (memory_ready = '1' or isvalid_buffer = '1') then
							state_i <= OPERAND1_FETCH;
						else
							state_i <= OPERAND2_FETCH;
						end if;
					when others =>
						cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
						sp_store <= '0';
						-- can't need operand2 but not operand1
						state_i <= RESET_STATE;
				end case;

			when OPERAND1_FETCH =>
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				-- forward current PC sto the result register,
				--  only on JUMP/CALL will the result register NOT be overwritten
				--  during the execute stage, thus CALL can use PUSH_RESULT to
				--  store the previous PC to stack
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';
				result_store <= '1';
					
				-- read operand1 from stack
				operand1_store <= not isvalid_buffer;
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				if (o1_descriptor = "10") then
					mem_data_size_select <= T_POINTER_SIZE;
				elsif (o1_descriptor = "11") then
					mem_data_size_select <= T_PARAMETER;
				else
					mem_data_size_select <= T_OPERAND_SIZE;
				end if;
				mem_read <= not isvalid_buffer;
				mem_write <= '0';

				if (memory_ready = '1' or isvalid_buffer = '1') then
					if (is_fetch = '1') then
						state_i <= FETCH;
					else
						state_i <= EXECUTE;
					end if;
				else
					state_i <= OPERAND1_FETCH;
				end if;
			when FETCH =>
				-- read value from memory and store in operand1
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- cra not used
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';
					
				operand1_store <= memory_ready;
				mem_addr_select <= T_OPERAND1;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_OPERAND_SIZE;
				mem_read <= '1';
				mem_write <= '0';

				if (memory_ready = '1') then
					state_i <= EXECUTE;
				else
					state_i <= FETCH;
				end if;
			when EXECUTE =>
				sp_store <= (is_argstack or is_varstack) and alu_ready;
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= is_argstack and alu_ready;
				el_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- compute
				if (is_call = '1' or is_jump = '1' or is_positive_compare = '1') then
					result_store <= '0';
					pc_store <= '1';
				else
					result_store <= '1';
					pc_store <= '0';
				end if;

				-- cra not used
				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';
				
				-- no memory
				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				if (alu_ready = '1') then
					if (is_ret = '1') then
						state_i <= RESTORE;
					elsif (is_call = '1') then
						state_i <= SAVERA;
					else
						case g_descriptor is
							when "01" =>
								state_i <= PUSH_RESULT;
							when "10" =>
								state_i <= WRITE_RESULT;
							when others =>
								state_i <= INSTRUCTION_FETCH;
						end case;
					end if;
				else
					state_i <= EXECUTE;
				end if;
			when PUSH_RESULT =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				-- store the result in the operand registers
				operand1_store <= '1';
				operand2_store <= '1';

				invalidate_buffer <= is_ret;	
				validate_buffer <= '1';

				-- update sp
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_OPERAND_SIZE;
				cra_sub <= '0';
				sp_store <= memory_ready;

				-- write result
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_OPERAND_SIZE;
				mem_read <= '0';
				mem_write <= '1';
				
				if (memory_ready = '1') then
					state_i <= INSTRUCTION_FETCH;
				else
					state_i <= PUSH_RESULT;
				end if;
			when WRITE_RESULT =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				sp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';

				-- write result
				mem_addr_select <= T_OPERAND1;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_OPERAND_SIZE;
				mem_read <= '0';
				mem_write <= '1';
				
				if (memory_ready = '1') then
					state_i <= INSTRUCTION_FETCH;
				else
					state_i <= WRITE_RESULT;
				end if;
			when RESTORE =>
				pc_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				-- decrease FP with 5x pointer size (which is also the new sp)
				fp_store <= '1';
				sp_store <= '1';
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER5_SIZE;
				cra_sub <= '1';

				state_i <= RESTORE_PC1;
			when RESTORE_PC1 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read PC
				mem_read <= '1';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- increase FP
				fp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= RESTORE_PC2;
				else
					state_i <= RESTORE_PC1;
				end if;
			when RESTORE_PC2 =>
				ap_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- restore PC
				pc_store <= '1';
				cra_op1_select <= T_CRA_OP1_ZERO;
				cra_op2_select <= T_CRA_OP2_PARAMETER;
				cra_sub <= '0';

				state_i <= RESTORE_EL1;

			when RESTORE_EL1 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read EL
				mem_read <= '1';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- increase FP
				fp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= RESTORE_EL2;
				else
					state_i <= RESTORE_EL1;
				end if;

			when RESTORE_EL2 =>
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				lp_store <= '0';
				ap_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- restore el
				el_store <= '1';
				cra_op1_select <= T_CRA_OP1_ZERO;
				cra_op2_select <= T_CRA_OP2_PARAMETER;
				cra_sub <= '0';

				state_i <= RESTORE_AP1;

			when RESTORE_AP1 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read AP
				mem_read <= '1';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- increase FP
				fp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= RESTORE_AP2;
				else
					state_i <= RESTORE_AP1;
				end if;

			when RESTORE_AP2 =>
				pc_store <= '0';
				sp_store <= '0';
				fp_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- restore ap
				ap_store <= '1';
				cra_op1_select <= T_CRA_OP1_ZERO;
				cra_op2_select <= T_CRA_OP2_PARAMETER;
				cra_sub <= '0';

				state_i <= RESTORE_LP1;

			when RESTORE_LP1 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read LP
				mem_read <= '1';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- increase FP
				fp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= RESTORE_LP2;
				else
					state_i <= RESTORE_LP1;
				end if;

			when RESTORE_LP2 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				fp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- restore LP
				lp_store <= '1';
				cra_op1_select <= T_CRA_OP1_ZERO;
				cra_op2_select <= T_CRA_OP2_PARAMETER;
				cra_sub <= '0';

				state_i <= RESTORE_FP1;
				
			when RESTORE_FP1 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				-- read FP
				mem_read <= '1';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- increase FP
				fp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= RESTORE_FP2;
				else
					state_i <= RESTORE_FP1;
				end if;

			when RESTORE_FP2 =>
				pc_store <= '0';
				sp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '1';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '0';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_FP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- restore FP
				fp_store <= '1';
				cra_op1_select <= T_CRA_OP1_ZERO;
				cra_op2_select <= T_CRA_OP2_PARAMETER;
				cra_sub <= '0';

				state_i <= PUSH_RESULT;
			when SAVERA =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '1';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_POINTER_SIZE;

				-- update sp
				sp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= SAVEEL;
				else
					state_i <= SAVERA;
				end if;
			when SAVEEL =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '1';
				mem_addr_select <= T_SP;
				mem_data_select <= T_EL;
				mem_data_size_select <= T_POINTER_SIZE;

				-- update sp
				sp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= SAVEAP;
				else
					state_i <= SAVEEL;
				end if;
			when SAVEAP =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '1';
				mem_addr_select <= T_SP;
				mem_data_select <= T_AP;
				mem_data_size_select <= T_POINTER_SIZE;

				-- update sp
				sp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= SAVELP;
				else
					state_i <= SAVEAP;
				end if;
			when SAVELP =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '1';
				mem_addr_select <= T_SP;
				mem_data_select <= T_LP;
				mem_data_size_select <= T_POINTER_SIZE;

				-- update sp
				sp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= SAVEFP;
				else
					state_i <= SAVELP;
				end if;
			when SAVEFP =>
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '1';
				mem_addr_select <= T_SP;
				mem_data_select <= T_FP;
				mem_data_size_select <= T_POINTER_SIZE;

				-- update sp
				sp_store <= memory_ready;
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_POINTER_SIZE;
				cra_sub <= '0';

				if (memory_ready = '1') then
					state_i <= NEWFRAME1;
				else
					state_i <= SAVEFP;
				end if;
			when NEWFRAME1 =>
				-- fp => ap
				pc_store <= '0';
				fp_store <= '0';
				ap_store <= '1';
				lp_store <= '0';
				sp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				-- echo FP
				cra_op1_select <= T_CRA_OP1_FP;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';

				state_i <= NEWFRAME2;
			when NEWFRAME2 =>
				-- sp => lp/fp
				pc_store <= '0';
				fp_store <= '1';
				ap_store <= '0';
				lp_store <= '1';
				sp_store <= '0';
				el_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				-- echo SP
				cra_op1_select <= T_CRA_OP1_SP;
				cra_op2_select <= T_CRA_OP2_ZERO;
				cra_sub <= '0';

				state_i <= INSTRUCTION_FETCH;
			when TRAPPED =>
				sp_store <= '0';
				fp_store <= '0';
				ap_store <= '0';
				lp_store <= '0';
				el_store <= '0';
				pc_store <= '0';
				result_store <= '0';
				instruction_store <= '0';
				parameter_store <= '0';
				operand1_store <= '0';
				operand2_store <= '0';
				validate_buffer <= '0';
				invalidate_buffer <= '1';

				cra_op1_select <= T_CRA_OP1_PC;
				cra_op2_select <= T_CRA_OP2_INSTRUCTION_SIZE;
				cra_sub <= '1';

				mem_read <= '0';
				mem_write <= '0';
				mem_addr_select <= T_SP;
				mem_data_select <= T_RESULT;
				mem_data_size_select <= T_LNG;

				if (interrupt = '1') then
					state_i <= INTERRUPT_1;
				else
					state_i <= TRAPPED;
				end if;
		end case;
	end process;

	process(clk, reset, state_i) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state_o <= RESET_STATE;
			else
				state_o <= state_i;
			end if;
		end if;
	end process;
end architecture;
