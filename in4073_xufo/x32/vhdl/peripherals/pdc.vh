#bigfunc ADD_PDC_DEVICE(NAME, INDEX, INT_INDX, PINS)
	REGISTER_INPUT_PIN(NAME, 1, PINS)
	CREATE_MUX_ENTRY(INDEX, NAME_dig, PERIPHERAL_DATA_BITS, NAME, '1')
	REGISTER_IRQ(INT_INDX, NAME_int, NAME)

	#ifdef __IN_DECL
		signal NAME_dig : std_logic_vector(unhex(PERIPHERAL_DATA_BITS)-1 downto 0);
		signal NAME_int : std_logic;
	#endif
	#ifdef __IN_VHDL
		l_NAME: entity work.pdc(behaviour)
			generic map (
				size => unhex(PERIPHERAL_DATA_BITS)
			)
			port map ( 
				clk => clk, 
				reset => reset,
				input => NAME(0),
				value => NAME_dig,
				newvalue => NAME_int
			);
	#endif 
#endbigfunc
