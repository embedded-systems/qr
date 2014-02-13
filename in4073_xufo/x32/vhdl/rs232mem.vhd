-------------------------------------------------------------------------------
-- Filename:             rs232mem.vhd
-- Entity:               rs232mem
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          RS232 memory controller. The component transfers 
--                       32-bit memory operations over the RS232 interface,
--                       such that the X32's memory can be simulated on a
--                       personal computer. See the rs232memserv utility
--                       (x32-tools) for more information
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

entity rs232mem is
	port (
		-- system clock
		clk				: in  std_logic;
		-- system reset
		reset			: in  std_logic;

-- debug
state : out positive;
substate : out positive;

		-- x32 memory signals
		mem_address			: in    std_logic_vector(31 downto 0);
		mem_data_in			: in    std_logic_vector(31 downto 0);
		mem_data_out  	: out   std_logic_vector(31 downto 0);
		mem_data_size		: in    std_logic_vector(2 downto 0);
		mem_data_signed	: in    std_logic;
		mem_read				: in    std_logic;
		mem_write				: in    std_logic;
		mem_ready				: out   std_logic;

		-- rs232 line signals
		rx							: in  std_logic;
		tx							: out std_logic
	
	);
end entity;

architecture behaviour of rs232mem is
	signal word_in			: std_logic_vector(31 downto 0);
	signal word_out			: std_logic_vector(31 downto 0);
	signal word_read		: std_logic;
	signal word_write		: std_logic;
	signal word_ready		: std_logic;
	signal byte_in			: std_logic_vector(7 downto 0);
	signal byte_out  		: std_logic_vector(7 downto 0);
	signal byte_read 		: std_logic;
	signal byte_write		: std_logic;
	signal byte_ready		: std_logic;
	signal mode8				: std_logic;

	signal rs232_in_ready			: std_logic;
	signal rs232_out_ready		: std_logic;

	type STATETYPE is (RESET_STATE, SYNCHRONIZE_SEND_COMMAND, 
			SYNCHRONIZE_SEND_DATA, SYNCHRONIZE_RECV, IDLE, SEND_COMMAND, SEND_ADDR, 
			READ_DATA, WRITE_DATA, READ_WRITE_ACK, READY_STATE);
	signal state_i, state_o : STATETYPE;
begin
	lbl1: entity work.data32to8(behaviour) port map (
		clk => clk,
		reset => reset,
state => substate,
		word_in => word_in,
		word_out => word_out,
		word_read => word_read,
		word_write => word_write,
		word_ready => word_ready,
		byte_in => byte_in,
		byte_out => byte_out,
		byte_read => byte_read,
		byte_write => byte_write,
		byte_ready => byte_ready,
		mode8 => mode8
	);
	lbl2: entity work.rs232in(behaviour) 
		generic map (
			clocks_per_bit => 434
		)
		port map (
			clk => clk,
			reset => reset,
			rx => rx,
			ready => rs232_in_ready,
			data => byte_in
		);
	lbl3: entity work.rs232out(behaviour) 
		generic map (
			clocks_per_bit => 434
		)
		port map (
			clk => clk,
			reset => reset,
			tx => tx,
			ready => rs232_out_ready,
			data => byte_out,
			send => byte_write
		);
	byte_ready <= (rs232_in_ready and byte_read) or 
		(rs232_out_ready and byte_write);

	process(state_o, word_ready, word_out, mem_read, mem_write, mem_data_size, 
		mem_address, mem_data_in, mem_data_signed) begin

		case state_o is
			when RESET_STATE =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '0';
				state_i <= SYNCHRONIZE_SEND_COMMAND;
state <= 1;
			when SYNCHRONIZE_SEND_COMMAND =>
				word_in <= x"000000FF";
				word_write <= '1';
				word_read <= '0';
				mode8 <= '1';
				mem_ready <= '0';
				if (word_ready = '1') then
					state_i <= SYNCHRONIZE_SEND_DATA;
				else
					state_i <= SYNCHRONIZE_SEND_COMMAND;
				end if;
state <= 3;
			when SYNCHRONIZE_SEND_DATA =>
				word_in <= x"0F1E2D3C";
				word_write <= '1';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '0';
				if (word_ready = '1') then
					state_i <= SYNCHRONIZE_RECV;
				else
					state_i <= SYNCHRONIZE_SEND_DATA;
				end if;
state <= 4;
			when SYNCHRONIZE_RECV =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '1';
				mode8 <= '0';
				mem_ready <= '0';
				if (word_ready = '1') then
					if (word_out = x"0F1E2D3C") then
						state_i <= IDLE;
					else
						state_i <= SYNCHRONIZE_SEND_COMMAND;
					end if;
				else
					state_i <= SYNCHRONIZE_RECV;
				end if;
state <= 5;
			when IDLE =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '0';
				if (mem_read = '1' or mem_write = '1') then
					state_i <= SEND_COMMAND;
				else
					state_i <= IDLE;
				end if;
state <= 6;
			when SEND_COMMAND =>
				word_in <= x"000000" & "01" & mem_read & mem_write & mem_data_signed & mem_data_size;
				word_write <= '1';
				word_read <= '0';
				mode8 <= '1';
				mem_ready <= '0';
				if (word_ready = '1') then
					state_i <= SEND_ADDR;
				else
					state_i <= SEND_COMMAND;
				end if;
state <= 7;
			when SEND_ADDR =>
				word_in <= mem_address;
				word_write <= '1';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '0';
				if (word_ready = '1') then
					if (mem_read = '1') then
						state_i <= READ_DATA;
					elsif (mem_write = '1') then
						state_i <= WRITE_DATA;
					else 
						state_i <= RESET_STATE;
					end if;
				else
					state_i <= SEND_ADDR;
				end if;
state <= 8;
			when READ_DATA =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '1';
				mode8 <= '0';
				mem_ready <= '0';
				if (word_ready = '1') then
					state_i <= READY_STATE;
				else
					state_i <= READ_DATA;
				end if;
state <= 9;
			when WRITE_DATA =>
				word_in <= mem_data_in;
				word_write <= '1';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '0';
				if (word_ready = '1') then
					state_i <= READ_WRITE_ACK;
				else
					state_i <= WRITE_DATA;
				end if;
state <= 10;
			when READ_WRITE_ACK =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '1';
				mode8 <= '1';
				mem_ready <= '0';
				if (word_ready = '1') then
					if (word_out(7 downto 0) = "01" & mem_read & mem_write & mem_data_signed & mem_data_size) then
						state_i <= READY_STATE;
					else
						state_i <= RESET_STATE;
					end if;
				else
					state_i <= READ_WRITE_ACK;
				end if;
state <= 11;
			when READY_STATE =>
				word_in <= (others => '-');
				word_write <= '0';
				word_read <= '0';
				mode8 <= '0';
				mem_ready <= '1';
				state_i <= IDLE;
state <= 12;
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

	process(clk, reset, state_o) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				mem_data_out <= (others => '0');
			elsif (state_o = READ_DATA) then
				mem_data_out <= word_out;
			end if;
		end if;
	end process;
end architecture;
