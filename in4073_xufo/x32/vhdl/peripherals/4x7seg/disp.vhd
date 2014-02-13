-------------------------------------------------------------
--   disp.vhd
--       Display controller for the FPGA 7-segment display.
--       The controller has two 8-bit inputs, the inputs are
--			converted from binary to hexadecimal (0-F) and
--			displayed as one byte per two displays.
--
--			The bin2hex component is used to convert the binary
--			input data to the hexadecimal representations
--
--			Note: the dots are not used and always off
--
--			Author: Sijmen Woutersen
--
-------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity disp7seg is
	port ( 
		-- system clock
		clk : in std_logic;
		-- system reset
		reset : in std_logic;
		-- first byte value (showed on left display)
		byte1 : in std_logic_vector(7 downto 0);
		-- second byte value (showed on right display)
		byte2 : in std_logic_vector(7 downto 0);
		-- 4 dots
		dots : in std_logic_vector(3 downto 0);
		-- display control bits (anode enable)
		control : out std_logic_vector(3 downto 0);
		-- display data bits
		data : out std_logic_vector(7 downto 0)
	);
end entity;

architecture behaviour of disp7seg is
	-- counter which counts all four states
	signal cntr : std_logic_vector(15 downto 0);
	-- four display values
	signal disp1_i, disp2_i, disp3_i, disp4_i : std_logic_vector(6 downto 0);
	signal disp1_o, disp2_o, disp3_o, disp4_o : std_logic_vector(6 downto 0);
begin
	-- convert binary data to hexadecimal displays
	hex1: entity work.bin2hex(behaviour)
		port map (
			bindata => byte1, 
			hexmsw => disp1_i, 
			hexlsw => disp2_i
		);
	hex2: entity work.bin2hex(behaviour)
		port map (
			bindata => byte2, 
			hexmsw => disp3_i, 
			hexlsw => disp4_i
		);

	-- set control bits
	control <= 
		"0111" when cntr(15 downto 14) = "00" else 
		"1011" when cntr(15 downto 14) = "01" else 
		"1101" when cntr(15 downto 14) = "10" else 
		"1110" when cntr(15 downto 14) = "11";

	-- set display data
	data(7 downto 1) <= 
		disp1_o when cntr(15 downto 14) = "00" else 
		disp2_o when cntr(15 downto 14) = "01" else 
		disp3_o when cntr(15 downto 14) = "10" else 
		disp4_o when cntr(15 downto 14) = "11";
	-- set dots
	data(0) <= 
		not dots(0) when cntr(15 downto 14) = "00" else 
		not dots(1) when cntr(15 downto 14) = "01" else 
		not dots(2) when cntr(15 downto 14) = "10" else 
		not dots(3) when cntr(15 downto 14) = "11";

	-- update counter every clock
	main: process(clk, reset, disp1_i, disp2_i, disp3_i, disp4_i)
	begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				cntr <= "0000000000000000";
				disp1_o <= "0000000";
				disp2_o <= "0000000";
				disp3_o <= "0000000";
				disp4_o <= "0000000";
			else
				cntr <= cntr + 1;
				disp1_o <= disp1_i;
				disp2_o <= disp2_i;
				disp3_o <= disp3_i;
				disp4_o <= disp4_i;
			end if;
		end if;
	end process;

end architecture;
