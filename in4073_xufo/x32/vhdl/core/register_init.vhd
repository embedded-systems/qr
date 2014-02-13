-------------------------------------------------------------------------------
-- Filename:             register_init.vhd
-- Entity:               reg_init
-- Architectures:        behaviour
-- Author:               Sijmen Woutersen
-- Last Modified:        2007/01/06
-- Version:              1.0
-- Description:          Simple n-bit register, with two store inputs (one
--                       being the register reset). The register thus resets
--                       to a predifined value rather than zero.
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

entity reg_init is
	generic (
		-- size of the register (in bits)
		size  	: in  positive
	);
	port (
		-- system clock
		clk		: in  std_logic;
		-- system reset (register resets to zero)
		reset		: in	std_logic;
		-- store signal
		store		: in  std_logic;
		-- data input
		data_in	: in  std_logic_vector(size-1 downto 0);
		-- data output
		data_out : out std_logic_vector(size-1 downto 0);
		-- initial (reset) value
		init_val	: in  std_logic_vector(size-1 downto 0)
	);
end entity;

architecture behaviour of reg_init is
begin
	-- the register
	process(clk, reset, init_val) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				-- reset register to init_val
				data_out <= init_val;
			elsif (store = '1') then
				-- store register input
				data_out <= data_in;
			end if;
		end if;
	end process;
end architecture;

