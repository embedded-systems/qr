-------------------------------------------------------------
--   bin2hex.vhd
--       Converts a binary data byte to its hexadecimal
--			represenation on a 7 segment display
--
--			Author: Sijmen Woutersen
--
-------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity bin2hex is
    port (
	 	-- binary data
	 	bindata	: in std_logic_vector(7 downto 0);
		-- most significant hexadecimal character
		hexmsw	: out std_logic_vector(6 downto 0);
		-- leas significant hexadecimal character
		hexlsw 	: out std_logic_vector(6 downto 0));
end entity;

architecture behaviour of bin2hex is
begin
	-- ROM
	hexmsw <= "0000001" when bindata(7 downto 4) = "0000"
		else	 "1001111" when bindata(7 downto 4) = "0001"
		else	 "0010010" when bindata(7 downto 4) = "0010"
		else	 "0000110" when bindata(7 downto 4) = "0011"
		else	 "1001100" when bindata(7 downto 4) = "0100"
		else	 "0100100" when bindata(7 downto 4) = "0101"
		else	 "0100000" when bindata(7 downto 4) = "0110"
		else	 "0001111" when bindata(7 downto 4) = "0111"
		else	 "0000000" when bindata(7 downto 4) = "1000"
		else	 "0000100" when bindata(7 downto 4) = "1001"
		else	 "0001000" when bindata(7 downto 4) = "1010"
		else	 "1100000" when bindata(7 downto 4) = "1011"
		else	 "0110001" when bindata(7 downto 4) = "1100"
		else	 "1000010" when bindata(7 downto 4) = "1101"
		else	 "0110000" when bindata(7 downto 4) = "1110"
		else	 "0111000" when bindata(7 downto 4) = "1111";
	-- ROM
	hexlsw <= "0000001" when bindata(3 downto 0) = "0000"
		else	 "1001111" when bindata(3 downto 0) = "0001"
		else	 "0010010" when bindata(3 downto 0) = "0010"
		else	 "0000110" when bindata(3 downto 0) = "0011"
		else	 "1001100" when bindata(3 downto 0) = "0100"
		else	 "0100100" when bindata(3 downto 0) = "0101"
		else	 "0100000" when bindata(3 downto 0) = "0110"
		else	 "0001111" when bindata(3 downto 0) = "0111"
		else	 "0000000" when bindata(3 downto 0) = "1000"
		else	 "0000100" when bindata(3 downto 0) = "1001"
		else	 "0001000" when bindata(3 downto 0) = "1010"
		else	 "1100000" when bindata(3 downto 0) = "1011"
		else	 "0110001" when bindata(3 downto 0) = "1100"
		else	 "1000010" when bindata(3 downto 0) = "1101"
		else	 "0110000" when bindata(3 downto 0) = "1110"
		else	 "0111000" when bindata(3 downto 0) = "1111";
end architecture;
