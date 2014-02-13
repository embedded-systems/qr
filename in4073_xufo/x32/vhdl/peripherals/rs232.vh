#bigfunc ADD_RS232_DEVICE(NAME, INDEX_PORT, INDEX_CONTROL, IN_BUFF_SIZE, OUT_BUFF_SIZE, BAUDRATE, PIN_TX, PIN_RX, IN_INT_INDEX, OUT_INT_INDEX)

	REGISTER_INPUT_PIN(NAME_rx, 1, PIN_RX)
	REGISTER_OUTPUT_PIN(NAME_tx, 1, PIN_TX)

	CREATE_WRITE_SIGNAL(INDEX_PORT, NAME_write)
	CREATE_READ_SIGNAL(INDEX_PORT, NAME_read)

	CREATE_MUX_ENTRY(INDEX_PORT, NAME_out, 8, NAME_data, '1')
	CREATE_MUX_ENTRY(INDEX_CONTROL, NAME_control, 2, NAME_status, '1')

	REGISTER_IRQ(IN_INT_INDEX, NAME_rx_int, NAME_rx)
	REGISTER_IRQ(OUT_INT_INDEX, NAME_tx_int, NAME_tx)

	#ifdef __IN_DECL
		signal NAME_out : std_logic_vector(7 downto 0);
		signal NAME_allow_write : std_logic;
		signal NAME_allow_read : std_logic;
		signal NAME_control : std_logic_vector(1 downto 0);
		signal NAME_rx_int : std_logic;
		signal NAME_tx_int : std_logic;
	#endif
	#ifdef __IN_VHDL 
		NAME_control(1) <= NAME_allow_read;
		NAME_control(0) <= NAME_allow_write;

		l_NAME: entity work.uart(behaviour)
			generic map (
				out_buffer_size => OUT_BUFF_SIZE,
				in_buffer_size => IN_BUFF_SIZE,
				clocks_per_bit => CLOCKSPEED / BAUDRATE
			)
			port map (
				clk => clk,
				reset => reset,
				data_in => data_in(7 downto 0),
				data_out => NAME_out,
				write => NAME_write,
				read => NAME_read,
				allow_write => NAME_allow_write,
				allow_read => NAME_allow_read,
				rx_int => NAME_rx_int,
				tx_int => NAME_tx_int,
				rx => NAME_rx(0),
				tx => NAME_tx(0)
			);
		
	#endif 
#endbigfunc