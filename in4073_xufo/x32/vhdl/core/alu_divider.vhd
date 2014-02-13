-------------------------------------------------------------------------------
-- Filename:             alu_divider.vhd
-- Entity:               divider
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          ALU component which takes care of division. The
--                       component also checks division by zero
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

-- radix 2 non-restoring divider based on chapter 13 of 
-- "Computer Arithmetic: Algorithms and hardware designs" by Behrooz Parhami
-- (Oxford University Press, 2000)

entity divider is
	generic (
		-- size (in bits) of the operands
		size		: in  positive
	);
	port (
		-- clk signal
		clk				: in  std_logic;
		-- reset signal
		reset			: in  std_logic;
		-- start performing a division, a pulse on this signal starts
		--  the division, the divider responds with a pulse on the ready
		--  signal when the division is done
		start			: in  std_logic;
		-- defines whether the division operates on signed (1) or 
		--  unsigned(0) operands
		signed		: in  std_logic;
		-- operand 1
		op1				: in  std_logic_vector(size-1 downto 0);
		-- operand 2
		op2				: in  std_logic_vector(size-1 downto 0);
		-- quotient
		result		: out std_logic_vector(size-1 downto 0);
		-- remainder
		remainder	: out std_logic_vector(size-1 downto 0);
		-- division by zero exception (the divider generates a pulse
		--	on this signal whenever a division by zero is initiated)
		div0			: out std_logic;
		-- ready signal, the divider generates a pulse on this signal
		--  whenever a division operation finishes
		ready			: out std_logic
	);
end entity;

architecture behaviour of divider is
	signal s_i, s_o, shifted_s_o, shifted_op2 : std_logic_vector(2*size downto 0);
	signal p_i, set_result : std_logic;
	
	type STATE_TYPE is (IDLE, CALCULATING, CORRECT, READY_STATE);
	signal state_i, state_o : STATE_TYPE;
	signal counter : std_logic_vector(size-1 downto 0);
	signal reset_counter : std_logic;
	
	signal p : std_logic_vector(size-1 downto 0);
	signal q : std_logic_vector(size downto 0);
	
	signal result_i : std_logic_vector(size downto 0);
	signal result_o : std_logic_vector(size-1 downto 0);
	
	signal sign_d, sign_z, sign_s, s_zero : std_logic;
	
	-- 2x size+1 bit adder
	signal adder_in1, adder_in2, adder_result : std_logic_vector(2*size downto 0);
	signal sub : std_logic;

	signal zeroes : std_logic_vector(size*2 downto 0);
begin	
	-- check for div0
	zeroes <= (others => '0');
	div0 <= '1' when op2 = zeroes(size-1 downto 0) else '0';

	-- shifters
	shifted_s_o(2*size downto 1) <= s_o(2*size-1 downto 0);
	shifted_s_o(0) <= '0';
	
	shifted_op2(2*size) <= signed and op2(size-1);
	shifted_op2(2*size-1 downto size) <= op2;
	shifted_op2(size-1 downto 0) <= (others => '0');
	
	-- signs of the inputs
	sign_z <= signed and op1(size-1);
	sign_d <= signed and op2(size-1);
	
	-- sign of the intermediate result
	s_zero <= '1' when s_o(size*2 downto 0) = zeroes else '0';
	sign_s <= s_o(2*size) when s_zero = '0' else sign_z;

	process(state_o, op1, shifted_op2, shifted_s_o, start, op2, s_o, counter, q,
		sign_z, sign_d, sign_s, adder_result) begin
		case state_o is
			when IDLE =>
				-- IDLE: wait for start signal
				ready <= '0';
				reset_counter <= '1';
				set_result <= '0';
				s_i(2*size downto size) <= (others => sign_z);
				s_i(size-1 downto 0) <= op1;
				p_i <= '0';
				result_i <= (others => '-');

				adder_in1 <= (others => '-');
				adder_in2 <= (others => '-');
				sub <= '-';

				if (start = '1') then 
					state_i <= CALCULATING;
				else
					state_i <= IDLE;
				end if;
			when CALCULATING =>
				-- start calculation, this state is n times executed (n being 
				--  the operand size)

				ready <= '0';
				reset_counter <= '0';
				set_result <= '0';
				-- compute temporary result depending on sign (non-restoring)
				if (sign_d = sign_s) then
					p_i <= '1';
					--s_i <= shifted_s_o - shifted_op2;
					adder_in1 <= shifted_s_o;
					adder_in2 <= shifted_op2;
					sub <= '1';
					s_i <= adder_result;
				else
					p_i <= '0';
					--s_i <= shifted_s_o + shifted_op2;
					adder_in1 <= shifted_s_o;
					adder_in2 <= shifted_op2;
					sub <= '0';
					s_i <= adder_result;
				end if;
				result_i <= (others => '-');
				-- after n cycles, move to correction state
				if (counter(size-1) = '1') then
					state_i <= CORRECT;
				else
					state_i <= CALCULATING;
				end if;
			when CORRECT =>
				-- correction state (required for non-restoring division)
				ready <= '0';
				reset_counter <= '0';
				set_result <= '1';

				if (sign_s = sign_z) then
					-- no correction required
					s_i <= s_o;
					result_i <= q;

					adder_in1 <= (others => '-');
					adder_in2 <= (others => '-');
					sub <= '-';
				else
					-- + or - correction required depending on remainder sign
					if (sign_d = sign_s) then
--						s_i <= s_o - shifted_op2;
						adder_in1 <= s_o;
						adder_in2 <= shifted_op2;
						sub <= '1';
						s_i <= adder_result;

						result_i <= q+1;
					else
						--s_i <= s_o + shifted_op2;
						adder_in1 <= s_o;
						adder_in2 <= shifted_op2;
						sub <= '0';
						s_i <= adder_result;

						result_i <= q-1;
					end if;
				end if;
				p_i <= '0';
				state_i <= READY_STATE;
			when READY_STATE =>
				-- division is ready, trigger ready signal
				ready <= '1';
				reset_counter <= '0';
				result_i <= (others => '-');
				set_result <= '0';

				adder_in1 <= (others => '-');
				adder_in2 <= (others => '-');
				sub <= '-';

				s_i <= s_o;
				p_i <= '0';
				state_i <= IDLE;
		end case;
	end process;
	
	-- fsm register
	process(clk, reset, state_i, s_i) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state_o <= IDLE;
				s_o <= (others => '0');
			else
				state_o <= state_i;
				s_o <= s_i;
			end if;
		end if;
	end process;
	
	-- adder
	process(adder_in1, adder_in2, sub) begin
		if (sub = '0') then
			adder_result <= adder_in1 + adder_in2;
		else
			adder_result <= adder_in1 - adder_in2;
		end if;
	end process;
	
	-- p register
	process(clk, reset, p, p_i) begin
		if (clk = '1' and clk'event) then
			p(size-1 downto 1) <= p(size-2 downto 0);
			p(0) <= p_i;
		end if;
	end process;
	
	-- counter (counts iterations executed by the divider)
	process(clk, reset_counter, start, counter) begin
		if (clk'event and clk = '1') then
			if (reset_counter = '1') then
				counter(size-1 downto 1) <= (others => '0');
				counter(0) <= '1';
			else
				counter(size-1 downto 1) <= counter(size-2 downto 0);
				counter(0) <= '0';
			end if;
		end if;
	end process;

	-- result register
	process(clk, reset, result_i, set_result, result_o) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				result_o <= (others => '0');
			elsif (set_result = '1') then
				result_o <= result_i(size-1 downto 0);
			else
				result_o <= result_o;
			end if;
		end if;
	end process;
	
	-- q signal
	q(size) <= not p(size-1);
	q(size-1 downto 1) <= p(size-2 downto 0);
	q(0) <= '1';

	-- remainder & result outputs
	remainder <= s_o(size*2-1 downto size);
	result <= result_o;
end architecture;
