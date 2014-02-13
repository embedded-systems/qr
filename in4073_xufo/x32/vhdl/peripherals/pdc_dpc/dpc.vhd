library ieee;
    
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

entity dpc is
	generic (
		size			: positive
	);
	port ( 
		-- system clock
		clk    		: in std_logic; 
		-- system reset
		reset  		: in std_logic;
		-- dpc's period
		period 		: in std_logic_vector(size-1 downto 0);
		-- dpc's pulse width
		width  		: in std_logic_vector(size-1 downto 0);
		-- the generated pulse
		pulse  		: out std_logic
	);
end entity;

architecture behaviour of dpc is
	-- counter register
	signal counter : std_logic_vector(size-1 downto 0);
	-- counter reset signal	(on system reset or period reached)
	signal counter_reset : std_logic;
begin
	-- counter
	process(clk, reset, counter_reset) begin
		if (clk'event and clk = '1') then
			if (counter_reset = '1' or reset = '1') then
				counter <= (others => '0');
			else 
				counter <= counter + 1;
			end if;
		end if;
	end process;

	-- comperators           
	process (counter, width, period, reset)	begin
		-- period: reset counter when reached
		if (counter >= period) then
			counter_reset <= '1';
		else
			counter_reset <= '0';
		end if;
	
		-- pulsewidth: reset pulse when reached
		if (counter >= width or reset = '1') then
			pulse <= '0';
		else
			pulse <= '1';
		end if;
	end process;
end behaviour;