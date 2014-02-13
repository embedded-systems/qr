-------------------------------------------------------------------------------
-- Filename:             cache_parameters.vhd
-- Package:              cache_parameters
-- Author:               Matias Escudero
-- Last Modified:        2010/04/16
-- Version:              1.1
-- Description:          This file has the constants that describe the
--                       cache architecture in terms of block size, set
--                       size, and memory size. Changing them is possible
--                       to change the organization of the cache.
--                       
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
library IEEE;
use IEEE.STD_LOGIC_1164.all;

package cache_parameters is


	-- X32 address bits ( addressing bytes ).
	constant PHY_ADDR_BITS			: INTEGER := 32;

	-- Assuming words of 32 bits.
	-- Physical memory address 26 bits addressing bytes.
	-- Physical memory address 24 bits addressing words.
	-- Physical memory address = CACHE_TAG + CACHE_SET + BLOCK 	
	constant PHY_MEM_ADDR_BITS		: INTEGER := 26;
	
	-- BITS of the DDR SDRAM memory (words of 16 bits).
	constant DDR_ADDR_BITS			: INTEGER := 25;

	-- BITS of the physical address ( Adressing  Words of 32bits ).
	constant MEM_ADDR_BITS		  	: INTEGER := DDR_ADDR_BITS - 1;
	
  
	-- BITS of the address corresponding with the block offset.
	constant CACHE_BLOCK_BITS		: INTEGER := 5;
	constant CACHE_BLOCK_SIZE		: INTEGER := 2**CACHE_BLOCK_BITS; -- 128 bytes /// 32 words.
	
	-- BITS of the address corresponding with the cache set.  
	constant CACHE_SET_BITS 		: INTEGER := 9;
	constant CACHE_ADDR_BITS		: INTEGER := CACHE_SET_BITS + CACHE_BLOCK_BITS;
	constant CACHE_SIZE				: INTEGER := 2 ** CACHE_ADDR_BITS; -- 32 Kbytes /// 8 Kwords.
	
	-- BITS corresponding to the tag of each set.
	constant CACHE_TAG_BITS			: INTEGER := MEM_ADDR_BITS - (CACHE_SET_BITS + CACHE_BLOCK_BITS); -- 11 bits.

end cache_parameters;

