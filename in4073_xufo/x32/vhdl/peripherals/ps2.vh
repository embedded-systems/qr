#bigfunc ADD_PS2_DEVICE(NAME, INDEX_PORT, INDEX_CONTROL, INT_INDX, BUFFER_SIZE, PIN_CLOCK, PIN_DATA)
	REGISTER_INPUT_PIN(NAME_clk, 1, PIN_CLOCK)
	REGISTER_INPUT_PIN(NAME_data, 1, PIN_DATA)
	CREATE_MUX_ENTRY(INDEX_CONTROL, NAME_data_available, 1, NAME_CONTROL, '1')
	CREATE_MUX_ENTRY(INDEX_PORT, NAME_buffer_out, 8, NAME, '1')
	CREATE_READ_SIGNAL(INDEX_PORT, NAME_read)
	REGISTER_IRQ(INT_INDX, NAME_new_data, NAME)

	#ifdef __IN_DECL
		signal NAME_new_data : std_logic;
		signal NAME_data_in : std_logic_vector(7 downto 0);
		signal NAME_buffer_out : std_logic_vector(7 downto 0);
		signal NAME_buffer_full, NAME_data_available : std_logic_vector(0 downto 0);
	#endif
	#ifdef __IN_VHDL
		l_NAME: entity work.ps2reader(behaviour)
			port map ( 
				clk => clk, 
				reset => reset,
				ps2_data => NAME_data(0),
				ps2_clk => NAME_clk(0),
				strobe => NAME_new_data,
				byte => NAME_data_in
			);
		l_NAME_fifo: entity work.fifo_buffer(behaviour)
			generic map (
				item_size => 8,
				item_count => BUFFER_SIZE
			)
			port map (
				clk => clk,
				reset => reset,
				data_in => NAME_data_in,
				data_out => NAME_buffer_out, 
				push => NAME_new_data,
				pop => NAME_read,
				buffer_full => NAME_buffer_full(0),
				data_ready => NAME_data_available(0)
			);
	#endif 
#endbigfunc
