library ieee;
use ieee.std_logic_1164.all;

entity uart is
	generic (
		out_buffer_size		: positive;
		in_buffer_size		: positive;
		clocks_per_bit		: positive
	);
	port (
		clk						: in  std_logic;
		reset					: in  std_logic;
		
		data_in				: in  std_logic_vector(7 downto 0);
		data_out			: out std_logic_vector(7 downto 0);

		write					: in  std_logic;
		read					: in  std_logic;

		allow_write		: out std_logic;
		allow_read		: out std_logic;

		rx_int				: out std_logic;
		tx_int				: out std_logic;
		
		rx						: in  std_logic;
		tx						: out std_logic
	);
end entity;

architecture behaviour of uart is
	signal in_buff_push, out_buff_push : std_logic;
	signal in_buff_pop, out_buff_pop : std_logic;
	signal in_buff_full, out_buff_full : std_logic;
	signal in_buff_available, out_buff_available : std_logic;
	signal rs232_data_in, rs232_data_out : std_logic_vector(7 downto 0);
	
	signal data_out_reg_i : std_logic_vector(7 downto 0);
	
	--signal rs232_sendbusy, data_out_reg_store : std_logic;
begin
	rs232_receiver: entity work.rs232in(behaviour) 
		generic map (
			clocks_per_bit => clocks_per_bit
		)
		port map (
			clk => clk,
			reset => reset,
			rx => rx,
			ready => in_buff_push,
			data => rs232_data_in
		);

	input_fifo: entity work.fifo_buffer(behaviour) 
		generic map (
			item_size => 8, 
			item_count => in_buffer_size
		)
		port map (
			clk => clk, 
			reset => reset, 
			data_in => rs232_data_in, 
			data_out => data_out_reg_i, 
			push => in_buff_push, 
			pop => in_buff_pop, 
--			buffer_full => in_buff_full, 
			data_ready => in_buff_available
		);

	output_fifo: entity work.fifo_buffer(behaviour)
		generic map (
			item_size => 8, 
			item_count => out_buffer_size
		)
		port map (
			clk => clk, 
			reset => reset, 
			data_in => data_in, 
			data_out => rs232_data_out, 
			push => out_buff_push, 
			pop => out_buff_pop, 
			buffer_full => out_buff_full, 
			data_ready => out_buff_available
		);

	rs232_transmitter: entity work.rs232out(behaviour) 
		generic map (
			clocks_per_bit => clocks_per_bit
		)
		port map (
			clk => clk, 
			reset => reset, 
			data => rs232_data_out,
			send => out_buff_available, 
			tx => tx, 
			ready => out_buff_pop
--			busy => rs232_sendbusy
		);

	data_out <= data_out_reg_i;

	allow_write <= not out_buff_full;
	allow_read <= in_buff_available;

	in_buff_pop <= read;
	out_buff_push <= write;

	rx_int <= in_buff_push;
	tx_int <= out_buff_pop;
end architecture;
	
