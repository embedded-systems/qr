-------------------------------------------------------------------------------
-- Filename:             timer.vhd
-- Entity:               timer
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Simple timer component. The timer counts clockcycles 
--                       from 0 to the value defined by period (exclusive) and 
--                       generates a 1 cycle pulse each time the timer reaches
--                       period. The timer contains a greater than comparator,
--                       so it will trigger immediatly when the period is
--                       lowered to a value the timer counter already passed.
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

entity timer is
	generic (
		-- counter size, or maximum period (in bits)
		size		: in  positive
	);
	port ( 
		-- system clock
		clk			: in  std_logic; 
		-- system reset
		reset		: in  std_logic;
		-- unsigned integer, a pulse is generated
		--   every 'period' clockcycles
		period	: in  std_logic_vector(size-1 downto 0);
		-- the generated pulse (AH)
		pulse		: out std_logic
	);
end entity;

architecture behaviour of timer is
   -- counter register
   signal counter : std_logic_vector(size-1 downto 0);
	-- counter reset signal	(on period reached)
   signal counter_reset : std_logic;
   -- zero signal for comparison
   signal zero					: std_logic_vector(size-1 downto 0);
begin
	zero <= (others => '0');
	-- reset the counter when the timer expires, also check whether a period is
	--  provided (other than 0), and use greater than comparator, to make sure the
	--  counter doesn't keep counting when the period is lowered to a value the
	--  counter already passed.
	counter_reset <= '1' when counter > period and not (period = zero) else '0';
	-- generate an output pulse when the counter resets
	pulse <= counter_reset;

	-- counter
	process(clk, reset, counter_reset)	begin
		if (clk'event and clk = '1') then
			if (counter_reset = '1' or reset = '1') then
				-- start at two, since it resets on period+1 (not period-1), this makes the
				-- comperator somewhat smaller (> instead of >=)
				counter(size-1 downto 2) <= (others => '0');
				counter(1) <= '1';
				counter(0) <= '0';
			else 
				counter <= counter + 1;
			end if;
		end if;
	end process;

end architecture;
