library ieee;
    
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

entity pdc is
	generic (
		size			: positive
	);
	port ( 
		clk       : in std_logic;
		reset     : in std_logic;
		input     : in std_logic;

		value     : out std_logic_vector(size-1 downto 0);
		newvalue	: out std_logic
	);
end entity;

architecture behaviour of pdc is
	-- count & copy signals, counter counts when count is high, resets
	-- 	otherwise, when copy is high, the counter register is copied
	--		to the output register
	signal count, copy    : std_logic;
	-- counter signal
	signal counter  			: std_logic_vector(size-1 downto 0);
	-- input buffer signal
	signal input_buff			: std_logic;
	-- fsm states
	type STATETYPE is (RESETSTATE, INIT, COUNTING, COPYSTATE, WAITING, INTERRUPT);
	-- fsm state register
	signal state, next_state: STATETYPE;
begin
	newvalue <= '1' when state = INTERRUPT else '0';

	process(clk, reset, input) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				input_buff <= '0';
			else
				input_buff <= input;
			end if;
		end if;
	end process;

	process(state, input_buff) begin
		case state is
			when RESETSTATE =>
				-- reset everyting
				count <= '0';
				copy <= '0';
				next_state <= INIT;
			when INIT =>
				-- wait till first pulse ends (first pulse is ignored, if booting
				--		within a pulse, no value is generated)
				if (input_buff = '0') then
					next_state <= WAITING;
				else
					next_state <= INIT;
				end if;
				count <= '0';
				copy <= '0';
			when WAITING =>
				-- wait for pulse
				count <= '0';
				copy <= '0';
				if (input_buff = '1') then
					next_state <= COUNTING;
				else
					next_state <= WAITING;
				end if;
			when COUNTING =>
				-- count cycles pulse is high
				count <= '1';
				copy <= '0';
				if (input_buff = '0') then
					next_state <= COPYSTATE;
				else
					next_state <= COUNTING;
				end if;
			when COPYSTATE =>
				-- copy counter to output register
				count <= '1';
				copy <= '1';
				next_state <= INTERRUPT;
			when INTERRUPT =>
				-- raise interrupt
				count <= '1';
				copy <= '0';
				next_state <= WAITING;
		end case;
	end process;
   
	process(clk, reset) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state <= RESETSTATE;
			else
				state <= next_state;
			end if;
		end if;
	end process;
   
	-- counter (reset when reset is high or no longer counting
	process(clk, count, reset, counter) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				counter <= (others => '0');
			elsif (count = '1') then
				counter <= counter + 1;
			else
				counter <= (others => '0');
			end if; 
		end if; 
	end process; 

	-- output register           
	process(clk, reset, copy, counter) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				value <= (others => '0');
			elsif (copy = '1') then
				value <= counter;
			end if;
		end if;
	end process;

end architecture;
