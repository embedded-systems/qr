---------------------------------------------------------------------------
-- maxon 
-- decode Maxon encoder signals A, B
-- output 32 bit signal value + error pulse on signal error
--
---------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity maxon is
	port (
		clk			: in  std_logic;
		reset		: in  std_logic;
		a				: in  std_logic;
		b				: in  std_logic;
		err			: out std_logic;
		value 	: out std_logic_vector(31 downto 0)
	);
end entity;

architecture behaviour of maxon is

	component udcounter
		port(
			clk: in std_logic;
			reset: in std_logic;
			edge_up: in std_logic;
			edge_dn: in std_logic;
			count: out std_logic_vector(31 downto 0)
		);
	end component;
	component maxdecoder
		port(
			clk: in std_logic;
			reset: in std_logic;
			a: in std_logic;
			b: in std_logic;
			up: out std_logic;
			dn: out std_logic;
			err: out std_logic
		);
	end component;

	signal up, down : std_logic;
begin
	decoder: maxdecoder port map (clk, reset, a, b, up, down, err);
	counter: udcounter port map(clk, reset, up, down, value);
end architecture;