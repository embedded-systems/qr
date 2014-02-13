#bigfunc ADD_DPC_DEVICE(NAME, INDEX_PERIOD, INDEX_WIDTH, PINS)

	REGISTER_OUTPUT_PIN(NAME, 1, PINS)

	CREATE_REGISTER(INDEX_PERIOD, NAME_period, PERIPHERAL_DATA_BITS, NAME_period)
	CREATE_REGISTER(INDEX_WIDTH, NAME_width, PERIPHERAL_DATA_BITS, NAME_width)

	#ifdef __IN_VHDL 
		l_NAME: entity work.dpc(behaviour)
			generic map (
				size => unhex(PERIPHERAL_DATA_BITS)
			)
			port map ( 
				clk => clk,
				reset => reset,
				period => NAME_period,
				width => NAME_width,
				pulse => NAME(0)
			);
	#endif 
#endbigfunc
