-------------------------------------------------------------
--   rs232in.vhd
--       Reads bytes from a rs232 rx line. Stores the 
--			received bytes in a register and fires a ready
--			pulse when a new byte has been received. The register
--			should be read immediatly, a new byte my override
--			the old after about 4000 clock cycles.
--			
--			The reader is configured to work with the following
--			rs232 settings:
--				* baudrate: 	115200
--				* databits:		8
--				* stopbits: 	1
--				* parity: 		none
--				* rts control:	no
--
--			Note: the baudrate can be changed with the RS232SPEED
--				constant, however make sure you also change the value
--				on the RS232 writer
--
--			Author: Sijmen Woutersen
--
-------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;


entity rs232in is
	generic (
		clocks_per_bit		: in  positive
	);
	port ( 
		-- IN:  system clock signal
		clk 		:	in		std_logic;
		-- IN:  system reset (AH)
		reset    :  in    std_logic;
		-- IN:  MAX232 RX line
		rx       :  in    std_logic;
		-- OUT: byte available in register (AH)
		ready    :  out   std_logic;
		-- OUT: data register, contains last received byte
		data     :  out   std_logic_vector(7 downto 0)
	);
end entity;

-- The RS232 receiving component is programmed as one 11 bit shift register, with
--		a variable shifting speed depending on the state of the controller.
--		When the line is idle, the shiftregister works on the system clock speed,
--		reading the line at 50MHz. Whenever a '0' on the line is detected (a startbit),
--		the shifting speed is lowered to twice the baudrate, the following shift takes
--		therefor place in the exact middle of the startbit. From here the shiftrate is
--		further reduced to the baudrate, meaning the shiftregister will from now read
--		the line in the middle of every incomming bit allowing a maximum baudrate deviation
-- 		of (thearetically) 50%/10 = 5%.
--
--		The shift register is 11 bits long, twice the startbit, 8 databits and a stopbit
--		the stopbit is completely ignored, but must be read to keep the shifting speed
--		low during the last bit (otherwise a '0' as last data bit may be interpreted as 
--		a new startbit). 

architecture behaviour of rs232in is
	signal RS232SPEED : std_logic_vector(15 downto 0);
	
	-- shift register speed controller states:
	type STATETYPE is (RESETSTATE, LINEIDLE, WAITHALF, READING, READYSTATE1, READYSTATE2);
   
   -- pulse generater, used for baudrate generator
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


	-- state machine
	signal state, next_state : STATETYPE;
	-- speed (shift register input period)
	signal speed : std_logic_vector(15 downto 0);
	-- shift register
	signal shift_reg : std_logic_vector(10 downto 0);	
	-- input and output of data register (data output)
	signal data_i, data_o : std_logic_vector(7 downto 0);
	-- reset, ready and shift signals
	signal sreset, iready, shift : std_logic;
begin
	RS232SPEED <= conv_std_logic_vector(clocks_per_bit, 16);

	-- connect data to data register
	data <= data_o;
	-- ready to internal ready signal
	ready <= iready;

	-- baudrate generator
	baudrate: timer generic map(16) port map(clk, reset, speed, shift);

	-- finite state machine
	process(state, shift_reg, data_o) begin
		case state is
			when RESETSTATE =>
				-- RESETSTATE
				--		reset all registers
				speed <= conv_std_logic_vector(2, 16);
				data_i <= data_o;
				iready <= '0';
				next_state <= LINEIDLE;
				sreset <= '1';
			when LINEIDLE =>
				-- LINEIDLE
				--		wait for startbit
				speed <= conv_std_logic_vector(2, 16);
				data_i <= data_o;
				iready <= '0';
				-- startbit enters shiftregister at position 10
				if (shift_reg(10) = '0') then
					next_state <= WAITHALF;
				else
					next_state <= LINEIDLE;
				end if;
				sreset <= '0';
			when WAITHALF =>
				-- WAITHALF
				--		wait for middle of startbit
				speed(15) <= '0';
				speed(14 downto 0) <= RS232SPEED(15 downto 1);
				data_i <= data_o;
				iready <= '0';
				-- startbit reaches position 9
				if (shift_reg(9) = '0') then
					next_state <= READING;
				else
					next_state <= WAITHALF;
				end if;
				sreset <= '0';
			when READING =>
				-- READING
				--		wait for stopbit (startbit arrives at end of shift register)
				speed <= RS232SPEED;
				data_i <= data_o;
				iready <= '0';
				-- startbit reaches end of shift register
				if (shift_reg(0) = '0') then
					next_state <= READYSTATE1;
				else
					next_state <= READING;
				end if;
				sreset <= '0';
			when READYSTATE1 =>
				-- READYSTATE1
				--		fill data register	
				speed <= RS232SPEED;
				data_i <= shift_reg(9 downto 2);
				iready <= '0';
				next_state <= READYSTATE2;
				sreset <= '0';
			when READYSTATE2 =>
				-- READYSTATE2
				--		fire ready signal		
				speed <= RS232SPEED;
				data_i <= data_o;
				iready <= '1';
				next_state <= RESETSTATE;
				sreset <= '0';
		end case;
	end process;

	-- the shift register:
	process(clk, sreset) begin
		if (clk'event and clk = '1') then
			if (sreset = '1') then
				shift_reg <= "11111111111";
			elsif (shift = '1') then
				shift_reg(10) <= rx;
				shift_reg(9 downto 0) <= shift_reg(10 downto 1);
			end if;
		end if;
	end process;
				
	-- the state and data register
	process(clk, reset) begin
		if (clk'event and clk = '1') then
			if (reset = '1') then
				state <= RESETSTATE;
				data_o <= "00000000";
			else
				state <= next_state;
				data_o <= data_i;
			end if;
		end if;
	end process;
end architecture;
