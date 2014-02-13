-- Thanks to Bert Regelink for the initial version of this file

library ieee;
use ieee.std_logic_1164.all;

entity ps2reader is
	port (
		clk					: in  std_logic;
		reset				: in  std_logic;
		ps2_data		: in  std_logic;
		ps2_clk			: in  std_logic;
		strobe			: out std_logic;
		linebusy		: out std_logic;
		byte				: out std_logic_vector(7 downto 0)
	);
end entity;

architecture behaviour of ps2reader is
	signal ps2_clk_buff : std_logic;
	signal ps2_data_buff : std_logic;
	signal ps2_clk_buff_prev : std_logic;
	signal ps2_clk_falling_edge : std_logic;
	signal register_reset : std_logic;
	signal shift_register : std_logic_vector(10 downto 0);
	signal output_register : std_logic_vector(7 downto 0);
	signal save : std_logic;
begin
	process (clk, reset, ps2_clk, ps2_clk_buff) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				ps2_clk_buff <= '1';
				ps2_clk_buff_prev <= '1';
				ps2_data_buff <= '1';
			else
				ps2_clk_buff <= ps2_clk;
				ps2_clk_buff_prev <= ps2_clk_buff;
				ps2_data_buff <= ps2_data;
			end if;
		end if;
	end process;
	
	ps2_clk_falling_edge <= ps2_clk_buff_prev and not ps2_clk_buff;
	register_reset <= save or reset;
	
	process (clk, register_reset, ps2_clk_falling_edge) begin
		if (clk'event and clk = '1') then
			if (register_reset = '1') then
				shift_register <= (others => '1');
			elsif (ps2_clk_falling_edge = '1') then
				shift_register(10) <= ps2_data_buff;
				shift_register(9 downto 0) <= shift_register(10 downto 1);
			end if;
		end if;
	end process;
	
	save <= '1' when shift_register(0) = '0' else '0';
	
	process (clk, reset, save) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				output_register <= (others => '0');
				strobe <= '0';
			elsif (save = '1') then
				output_register <= shift_register(8 downto 1);
				strobe <= '1';
			else
				strobe <= '0';
			end if;
		end if;
	end process;

	process (ps2_clk_falling_edge, register_reset) begin
		if (ps2_clk_falling_edge = '1') then
			linebusy <= '1';
		elsif (register_reset = '1') then
			linebusy <= '0';
		end if;
	end process;
	
	byte <= output_register;
end architecture;
