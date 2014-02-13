-------------------------------------------------------------------------------
-- Filename:             data32to8.vhd
-- Entity:               data32to8
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Simple memory controller which breaks up each 32-bit
--                       operation in multiple 8-bit operations, used to
--                       execute 32-bit memory operations over an RS232 (8-bit)
--                       protocol. The most significant byte is written/read
--                       first
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
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity data32to8 is
	port (
		-- debug
		state					: out positive;
		-- system clock
		clk						: in  std_logic;
		-- system reset
		reset					: in  std_logic;
		
		-- 32-bit input
		word_in				: in  std_logic_vector(31 downto 0);
		-- 32-bit output
		word_out			: out std_logic_vector(31 downto 0);
		-- start a 32-bit read
		word_read			: in  std_logic;
		-- start a 32-bit write
		word_write		: in  std_logic;
		-- 32-bit operation completed (high for 1 cycle)
		word_ready		: out std_logic;

		-- mode, if mode8 = 1, the component runs in 8-bit mode,
		-- meaning it will only transfer the least significant byte
		mode8					: in  std_logic;

		-- 8-bit input
		byte_in				: in  std_logic_vector(7 downto 0);
		-- 8-bit output
		byte_out  		: out std_logic_vector(7 downto 0);
		-- start a 8-bit read
		byte_read 		: out std_logic;
		-- start a 8-bit write
		byte_write		: out std_logic;
		-- 8-bit operation completed (high for 1 cycle)
		byte_ready		: in  std_logic
	);
end entity;
	
architecture behaviour of data32to8 is
	-- register write enables
	signal save_byte1, save_byte2, save_byte3, save_byte4 : std_logic; 
	-- fsm
	type STATETYPE is (RESET_STATE, IDLE, READ1, READ2, READ3, READ4, 
				WRITE1, WRITE2, WRITE3, WRITE4, READY_STATE);
	signal state_i, state_o: STATETYPE;
begin
	-- fsm
	process(state_o, word_read, mode8, word_write, byte_ready, word_in) begin
		case state_o is
			when RESET_STATE =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '0';
				byte_out <= (others => '-');
				state_i <= IDLE;
state <= 1;
			when IDLE =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '0';
				byte_out <= (others => '-');
				if (word_read = '1') then
					if (mode8 = '1') then
						state_i <= READ4;
					else
						state_i <= READ1;
					end if;
				elsif (word_write = '1') then
					if (mode8 = '1') then
						state_i <= WRITE4;
					else
						state_i <= WRITE1;
					end if;
				else
					state_i <= IDLE;
				end if;
state <= 2;
			when READ1 =>
				word_ready <= '0';
				byte_read <= '1';
				byte_write <= '0';
				byte_out <= (others => '-');
				if (byte_ready = '1') then
					state_i <= READ2;
				else
					state_i <= READ1;
				end if;
state <= 3;
			when READ2 =>
				word_ready <= '0';
				byte_read <= '1';
				byte_write <= '0';
				byte_out <= (others => '-');
				if (byte_ready = '1') then
					state_i <= READ3;
				else
					state_i <= READ2;
				end if;
state <= 4;
			when READ3 =>
				word_ready <= '0';
				byte_read <= '1';
				byte_write <= '0';
				byte_out <= (others => '-');
				if (byte_ready = '1') then
					state_i <= READ4;
				else
					state_i <= READ3;
				end if;
state <= 5;
			when READ4 =>
				word_ready <= '0';
				byte_read <= '1';
				byte_write <= '0';
				byte_out <= (others => '-');
				if (byte_ready = '1') then
					state_i <= READY_STATE;
				else
					state_i <= READ4;
				end if;
state <= 6;
			when WRITE1 =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '1';
				byte_out <= word_in(31 downto 24);
				if (byte_ready = '1') then
					state_i <= WRITE2;
				else
					state_i <= WRITE1;
				end if;
state <= 7;
			when WRITE2 =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '1';
				byte_out <= word_in(23 downto 16);
				if (byte_ready = '1') then
					state_i <= WRITE3;
				else
					state_i <= WRITE2;
				end if;
state <= 8;
			when WRITE3 =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '1';
				byte_out <= word_in(15 downto 8);
				if (byte_ready = '1') then
					state_i <= WRITE4;
				else
					state_i <= WRITE3;
				end if;
state <= 9;
			when WRITE4 =>
				word_ready <= '0';
				byte_read <= '0';
				byte_write <= '1';
				byte_out <= word_in(7 downto 0);
				if (byte_ready = '1') then
					state_i <= READY_STATE;
				else
					state_i <= WRITE4;
				end if;
state <= 10;
			when READY_STATE =>
				word_ready <= '1';
				byte_read <= '0';
				byte_write <= '0';
				byte_out <= (others => '-');
				state_i <= IDLE;
state <= 11;
		end case;	
	end process;

	-- fsm register
	process(clk, reset, state_i) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state_o <= RESET_STATE;
			else
				state_o <= state_i;
			end if;
		end if;
	end process;

	-- 8 bit output registers
	save_byte1 <= '1' when state_o = READ1 else '0';
	save_byte2 <= '1' when state_o = READ2 else '0';
	save_byte3 <= '1' when state_o = READ3 else '0';
	save_byte4 <= '1' when state_o = READ4 else '0';

	process(clk, reset, save_byte1, save_byte2, save_byte3, save_byte4) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				word_out <= (others => '0');
			else
				if (save_byte1 = '1') then
					word_out(31 downto 24) <= byte_in;
				end if;
				if (save_byte2 = '1') then
					word_out(23 downto 16) <= byte_in;
				end if;
				if (save_byte3 = '1') then
					word_out(15 downto 8) <= byte_in;
				end if;
				if (save_byte4 = '1') then
					word_out(7 downto 0) <= byte_in;
				end if;
			end if;
		end if;
	end process;
end architecture;
