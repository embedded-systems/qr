-------------------------------------------------------------
--   rs232out.vhd
--			Writes bytes to a rs232 tx line. Grabs the data from
--			the data when 'send' becomes high. Starts tranmitting
--			the byte and raises the 'ready' signal when done. 
--			Ready will be high for only one clockcycle. Any
--			pulses on the send input will be ignored when busy
--			is high. When send is held high, the data byte will be
--			transmitted continuously
--			
--			The sender is configured to work with the following
--			rs232 settings:
--				* baudrate: 	115200
--				* databits:		8
--				* stopbits: 	1
--				* parity: 		none
--				* rts control:	no
--
--			Note: the baudrate can be changed with the RS232SPEED
--				constant, however make sure you also change the value
--				on the RS232 reader
--
--			Author: Sijmen Woutersen
--
-------------------------------------------------------------

library ieee;

use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

entity rs232out is
	generic (
		clocks_per_bit		: in  positive
	);
	port ( 
		-- system clock
		clk     :  in     std_logic;
		-- system reset
		reset    :  in     std_logic;
		-- input data
		data     :  in     std_logic_vector(7 downto 0);
		-- send input data (AH)
		send     :  in     std_logic;
		-- RS232 tx line
		tx       :  out    std_logic;
		-- transmit complete signal (AH)
		ready    :  out    std_logic;
		-- busy transmitting (AH)
		busy     :  out    std_logic
	);
end entity;

architecture behaviour of rs232out is
	component timer is
		generic (
			size		: in  positive
		);
		port ( 
			clk			: in  std_logic; 
			reset		: in  std_logic;
			period	: in  std_logic_vector(size-1 downto 0);
			pulse		: out std_logic
		);
	end component;

	-- input & output of data register
	signal datareg_i, datareg : std_logic_vector(7 downto 0);
	-- baudrate pulse 
	signal pulse : std_logic;
	-- fsm states
	type STATE_TYPE is (RESET_STATE, IDLE, COPY, STARTBIT, BIT7, BIT6, BIT5, BIT4, BIT3, BIT2, BIT1, BIT0, STOPBIT, READY_STATE);
	-- fsm register
	signal state, next_state : STATE_TYPE;
	-- clock speed / desired baudrate: 50MHz/115200baud = 434 = 110110010
	signal RS232SPEED : std_logic_vector(15 downto 0);
begin
	RS232SPEED <= conv_std_logic_vector(clocks_per_bit, 16);

	-- baudrate generator
	bautgen: timer generic map(16) port map(clk, reset, RS232SPEED, pulse);

	-- copy data fsm: copies data from the data input to the data
	--		register in state COPY
	process(state, data, datareg) begin
		case state is
			when COPY =>
				-- copy from input
				datareg_i <= data;
			when others =>
				-- hold
				datareg_i <= datareg;
		end case;
	end process;

	-- statemachine
	process (state, send, pulse, datareg) begin
		case state is
			when RESET_STATE =>
				-- reset state
				tx <= '1';
				next_state <= IDLE;
				ready <= '0';
				busy <= '1';
			when IDLE =>
				-- idle line (tx = high)
				tx <= '1';
				ready <= '0';
				busy <= '0';
				if (send = '1') then
					next_state <= COPY;
				else
					next_state <= IDLE;
				end if;
			when COPY =>
				-- copy databit, sync statemachine with baudrate generator
				tx <= '1';
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= COPY;
				else
					next_state <= STARTBIT;
				end if;
			when STARTBIT =>
				-- tx = startbit
				tx <= '0';
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= STARTBIT;
				else
					next_state <= BIT0;
				end if;
			when BIT0 =>
				-- tx = bit0
				tx <= datareg(0);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT0;
				else
					next_state <= BIT1;
				end if;
			when BIT1 =>
				-- tx = bit1
				tx <= datareg(1);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT1;
				else
					next_state <= BIT2;
				end if;
			when BIT2 =>
				-- tx = bit2
				tx <= datareg(2);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT2;
				else
					next_state <= BIT3;
				end if;
			when BIT3 =>
				-- tx = bit3
				tx <= datareg(3);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT3;
				else
					next_state <= BIT4;
				end if;
			when BIT4 =>
				-- tx = bit4
				tx <= datareg(4);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT4;
				else
					next_state <= BIT5;
				end if;
			when BIT5 =>
				-- tx = bit 5
				tx <= datareg(5);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT5;
				else
					next_state <= BIT6;
				end if;
			when BIT6 =>
				-- tx = bit6
				tx <= datareg(6);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT6;
				else
					next_state <= BIT7;
				end if;
			when BIT7 =>
				-- tx = bit7
				tx <= datareg(7);
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= BIT7;
				else
					next_state <= STOPBIT;
				end if;
			when STOPBIT =>
				--tx = stopbit = high
				tx <= '1';
				ready <= '0';
				busy <= '1';
				if (pulse = '0') then
					next_state <= STOPBIT;
				else
					next_state <= READY_STATE;
				end if;
			when READY_STATE =>
				--tx = idle = high
				tx <= '1';
				ready <= '1';
				busy <= '1';
				next_state <= IDLE;
		end case;

	end process;

	-- state/register updater
	process (clk, reset) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state <= RESET_STATE;
				datareg <= (others => '0');
			else
				state <= next_state;
				datareg <= datareg_i;
			end if;
		end if;
	end process;

end architecture;
