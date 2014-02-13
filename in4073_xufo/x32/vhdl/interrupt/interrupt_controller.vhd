-------------------------------------------------------------------------------
-- Filename:             interrupt_controller.vhd
-- Entity:               interrupt_controller
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Interrupt controller for the X32
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

-------------------------------------------------------------------------------
-- Simple interrupt controller for the X32, it supports multiple interrupt
-- lines, and a basic priority system.
--
-- Devices may be connected to the interrupt controller through the 'irqs' 
-- signal, each device gets its own 1 bit signal, the device may cause an
-- interrupt by driving the line high. The highest index in the irqs vector
-- gets the highest priority, irqs(0) gets the lowest priority, two devices
-- may share a priority, but the software will no longer be able to detect
-- which of the devices caused the interrupt (which is done through the
-- interrupt priority).
--
-- The interrupt controller checks each device sequentially, this causes an
-- average delay of n/2 clockcycles + the processors response time to an
-- interrupt (n is the number of devices passed to the generic map), when
-- only one device is interrupting. In a worst case scenario (all devices
-- are interrupting), the highest priority interrupt may be up to n times
-- the processors response time, which may be up to about 30 clockcycles.
-- Note that this is still that not much compared to the time it'll take to
-- execute a "usefull" software interrupt handler.
-------------------------------------------------------------------------------

entity interrupt_controller is
	generic (
		-- number of interrupt lines to support
		devices						: in  positive;
		-- size (in bits) of priority & address
		size							: in  positive;
		-- size (in bits) of ram address
		address_bits			: in  positive;
		-- one interrupt may be critical: 
		--  it will not be stopped by the global interrupt
		critical_interrupt: in  natural
	);
	port (
		-- system clock
		clk								: in  std_logic;
		-- system reset
		reset							: in  std_logic;
		
		-- interrupt request lines
		irqs							: in  std_logic_vector(devices-1 downto 0);
		ie								: in  std_logic_vector(devices downto 0);
		
		
		-- raise interrupt signal
		interrupt					: out std_logic;
		-- interrupt acknowledge signal
		interrupt_ack			: in  std_logic;
		
		-- current processor priority (execution level)
		current_priority	: in  std_logic_vector(size-1 downto 0);
		-- new processor priority (execution level)
		priority					: out std_logic_vector(size-1 downto 0);
		-- interrupt handler
		handler_address		: out std_logic_vector(size-1 downto 0);

		-- interrupt controller ram
		ram_in						: in  std_logic_vector(size-1 downto 0);
		ram_out						: out std_logic_vector(size-1 downto 0);
		ram_address				: in  std_logic_vector(address_bits-1 downto 0);
		ram_write					: in  std_logic;
		ram_read					: in  std_logic;
		ram_ready					: out std_logic
	);
end entity;

architecture behaviour of interrupt_controller is
	-- all irq's are checked one by one, this holds the counter value
	signal current_device : integer range 0 to devices-1;
	
	-- the current irq should interrupt the processor
	signal raise_interrupt	: std_logic;
	-- increase the counter
	signal count : std_logic;
	-- the irq input registers
	signal irq_reg : std_logic_vector(devices-1 downto 0);
	-- fsm states
	type STATETYPE is (RESET_STATE, CHECKING, INTERRUPTING, WRITE, READY);
	signal state_i, state_o : STATETYPE;

	-- RAM containing all interrupt vectors and priorities
	type RAM is array(devices-1 downto 0) of std_logic_vector(size-1 downto 0);
	signal vector_memory : RAM;
	signal priority_memory : RAM;

	-- vector and priority for the irq currently being checked
	signal interrupt_priority : std_logic_vector(size-1 downto 0);
	signal interrupt_vector : std_logic_vector(size-1 downto 0);

	-- RAM write signal
	signal sig_ram_write : std_logic;
begin
	-- the memory output of the interrupt controller is either the vector or priority
	--  of the current device (which is forced to ram address upon reading) 
	--	depending on the lower bit of the address
	ram_out <= interrupt_vector when ram_address(0) = '0' else interrupt_priority;

	-- priority & vector outputs
	priority <= interrupt_priority;
	handler_address <= interrupt_vector;
	
	-- check if the current irq should raise an interrupt, this happens if and only if:
	-- 	* the irq register contains logic 1 (the device requests an interrupt)
	--	* the priority is higher than the current execution level
	--	* the global interrupt is enabled -- OR -- the index is below the 
	--		critical_interrupt input
	raise_interrupt <= irq_reg(current_device) when interrupt_priority > current_priority
		and (ie(devices) = '1' or current_device < critical_interrupt) else '0';
	
	-- state register
	process (clk, reset) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then	
				state_o <= RESET_STATE;
			else
				state_o <= state_i;
			end if;
		end if;
	end process;	
	
	-- irq registers
	process(clk, reset, irqs, interrupt_ack, current_device, ie) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				irq_reg <= (others => '0');
			else
				for i in 0 to devices-1 loop
					if (irqs(i) = '1' and ie(i) = '1') then
						-- set on irq
						irq_reg(i) <= '1';
					elsif ((interrupt_ack = '1' or ie(i) = '0') and current_device = i) then
						-- reset on ack or disabled
						irq_reg(i) <= '0';
					end if;
				end loop;
			end if;
		end if;
	end process;
	
	-- current device counter
	process(clk, reset, count, current_device) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				current_device <= 0;
			elsif (ram_read = '1' or ram_write = '1') then
				current_device <= conv_integer(ram_address(address_bits-1 downto 1));
			elsif (count = '1') then
				if (current_device = devices-1) then
					current_device <= 0;
				else
					current_device <= current_device + 1;
				end if;
			end if;
		end if;
	end process;
	
	-- ram
	interrupt_vector <= vector_memory(current_device);
	interrupt_priority <= priority_memory(current_device);
	process(clk, reset) begin
		if (clk'event and clk = '1') then
			if (sig_ram_write = '1') then
				if (ram_address(0) = '0') then
					vector_memory(current_device) <= ram_in;
				else
					priority_memory(current_device) <= ram_in;
				end if;
			end if;
		end if;
	end process;

	-- fsm
	process(state_o, raise_interrupt, interrupt_ack, ram_read, ram_write) begin
		case state_o is
			when RESET_STATE =>
				interrupt <= '0';
				count <= '0';
				ram_ready <= '0';
				sig_ram_write <= '0';
				state_i <= CHECKING;
			when CHECKING =>
				-- check the current device (counter is running)
				interrupt <= '0';
				ram_ready <= '0';
				sig_ram_write <= '0';

				-- check for read/writes on the memory bus, or if the current
				-- device should interrupt the processor
				if (ram_read = '1') then
					-- reads are done automatically, jump to ready state immediatly
					count <= '-';
					state_i <= READY;
				elsif (ram_write = '1') then
					count <= '-';
					state_i <= WRITE;
				elsif (raise_interrupt = '1') then
					count <= '0';
					state_i <= INTERRUPTING;
				else
					count <= '1';
					state_i <= CHECKING;
				end if;
			when INTERRUPTING =>
				-- interrupt the processor, when a memory action occures in this state,
				-- it gets priority such that the processor won't enter a deadlock. When
				-- reading memory, the processor doesn't check the interrupt line, so
				-- the interrupt can be cancelled safely
				interrupt <= '1';
				count <= '0';
				ram_ready <= '0';
				sig_ram_write <= '0';
				
				-- note, when receiving an ram_read/ram_write signal after
				-- raising the interrupt signal, the core hasn't 'detected' the
				-- interrupt request yet, so it's safe to cancel (as the processor
				-- will now wait for a ram_ready signal).
				if (interrupt_ack = '1') then
					state_i <= CHECKING;
				elsif (ram_read = '1') then
					state_i <= READY;
				elsif (ram_write = '1') then
					state_i <= WRITE;
				else
					state_i <= INTERRUPTING;
				end if;
			when WRITE =>
				-- memory write
				interrupt <= '0';
				count <= '0';
				ram_ready <= '0';
				sig_ram_write <= '1';
				state_i <= READY;
			when READY =>
				-- tell the memory controller the memory action is completed
				interrupt <= '0';
				count <= '0';
				sig_ram_write <= '0';
				ram_ready <= '1';
				state_i <= CHECKING;
		end case;
	end process;
	
end architecture;
