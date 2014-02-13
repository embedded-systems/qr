
#bigfunc ADD_4x7SEGDISP_DEVICE(NAME, INDEX, DATA_PINS, CONTROL_PINS)
	REGISTER_OUTPUT_PIN(NAME_control, 4, CONTROL_PINS)
	REGISTER_OUTPUT_PIN(NAME_data, 8, DATA_PINS)

	CREATE_WRITE_SIGNAL(INDEX, NAME_reg_store)
	CREATE_MUX_ENTRY(INDEX, NAME_reg, 16, NAME, '1')

	#ifdef __IN_DECL
		signal NAME_reg : std_logic_vector(15 downto 0);
	#endif
	#ifdef __IN_VHDL
		l_NAME_reg: entity work.reg_init(behaviour)
			generic map (
				size => 16
			)
			port map (
				clk => clk,
				reset => reset,
				store => NAME_reg_store,
				data_in => data_in(15 downto 0),
				data_out => NAME_reg,
				init_val => conv_std_logic_vector(unhex(PERIPHERAL_ID), 16)
			);

		l_NAME: entity work.disp7seg(behaviour) 
			port map(
				clk => clk, 
				reset => reset,
				byte1 => NAME_reg(15 downto 8), 
				byte2 => NAME_reg(7 downto 0), 
				dots => "0000",
				control => NAME_control, 
				data => NAME_data
			);
	#endif 
#endbigfunc