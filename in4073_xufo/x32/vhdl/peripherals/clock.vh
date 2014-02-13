#bigfunc ADD_CLOCK_DEVICE(NAME, INDEX, PERIOD)
	CREATE_MUX_ENTRY(INDEX, NAME_buff, PERIPHERAL_DATA_BITS, NAME, '1')

	#ifdef __IN_DECL
		signal NAME_buff_in : std_logic_vector(unhex(PERIPHERAL_DATA_BITS)-1 downto 0);
		signal NAME_buff : std_logic_vector(unhex(PERIPHERAL_DATA_BITS)-1 downto 0);
		signal NAME_store : std_logic;
	#endif
	#ifdef __IN_VHDL 
		l_NAME_timer: entity work.timer(behaviour)
			generic map (
				size => log2_ceil(unhex(PERIOD))
			)
			port map (
				clk => clk,
				reset => reset,
				period => conv_std_logic_vector(unhex(PERIOD), log2_ceil(unhex(PERIOD))),
				pulse => NAME_store
			);
		NAME_buff_in <= NAME_buff + 1;
		l_NAME_buff: entity work.reg(behaviour)
			generic map (
				size => unhex(PERIPHERAL_DATA_BITS)
			)
			port map (
				clk => clk,
				reset => reset,
				store => NAME_store,
				data_in => NAME_buff_in,
				data_out => NAME_buff
			);		
	#endif 
#endbigfunc