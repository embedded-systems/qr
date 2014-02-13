#bigfunc ADD_TIMER_DEVICE(NAME, INDEX, INT_INDX)
	CREATE_REGISTER(INDEX, NAME_period, PERIPHERAL_DATA_BITS, NAME_period)
	REGISTER_IRQ(INT_INDX, NAME_int, NAME)

	#ifdef __IN_DECL
		signal NAME_int : std_logic;
		signal NAME_reset : std_logic;
	#endif
	#ifdef __IN_VHDL
		NAME_reset <= reset or NAME_period_store;

		l_NAME_timer: entity work.timer(behaviour)
			generic map (
				size => unhex(PERIPHERAL_DATA_BITS)
			)
			port map (
				clk => clk,
				reset => NAME_reset,
				period => NAME_period,
				pulse => NAME_int
			);
	#endif 
#endbigfunc