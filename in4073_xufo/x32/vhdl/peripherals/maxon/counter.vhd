---------------------------------------------------------------------------
-- counter with synchronous up/dn/reset 
-- counts on each positive up/dn EDGE
--
-- Author: Arjan van Gemund
---------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Sijmen edit: changed name from counter to udcounter
entity udcounter is
	port(
		clk: in std_logic;
		reset: in std_logic;
		edge_up: in std_logic;
		edge_dn: in std_logic;
		count: out std_logic_vector(31 downto 0)
	);
end udcounter;

architecture behavior of udcounter is
type statetype is (standby, up, dn);
signal s: statetype := standby;
signal c: std_logic_vector(31 downto 0) := (others => '0');

begin
	process
		begin
			wait until rising_edge(clk);
			case s is

			when standby =>
				if (reset = '1') then
					c <= (others => '0');
					s <= standby;
				end if;
				if (edge_up = '1') then
					-- Sijmen edit: protect from overflow
					if (c /= x"7FFFFFFF") then
						c <= c + '1';
					end if;
					s <= up;
				end if;
				if (edge_dn = '1') then
					-- Sijmen edit: protect from overflow
					if (c /= x"80000000") then
						c <= c - '1';
					end if;
					s <= dn;
				end if;

			when up =>
				if (reset = '1') then
					c <= (others => '0');
					s <= standby;
				end if;
				if (edge_up = '0') then
					s <= standby;
				end if;

			when dn =>
				if (reset = '1') then
					c <= (others => '0');
					s <= standby;
				end if;
				if (edge_dn = '0') then
					s <= standby;
				end if;
			end case;

			count <= c;

		end process;
end behavior;

