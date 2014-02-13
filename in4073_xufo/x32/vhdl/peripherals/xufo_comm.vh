
#bigfunc ADD_XUFOCOMM_DEVICE(NAME, INDEX_COUNT, INDEX_TIMESTAMP, INDEX_S0, INDEX_S1, INDEX_S2, INDEX_S3, INDEX_S4, INDEX_S5, INDEX_S6, IRQ_INDEX, PIN_RX, PIN_TX, INDEX_A0,INDEX_A1,INDEX_A2,INDEX_A3)


	REGISTER_INPUT_PIN(NAME_rx, 1, PIN_RX)
	REGISTER_OUTPUT_PIN(NAME_tx, 1, PIN_TX)
	REGISTER_IRQ(IRQ_INDEX, NAME_int, NAME)

	CREATE_MUX_ENTRY(INDEX_COUNT, NAME_count, 31, NAME_count, '1')
	CREATE_MUX_ENTRY(INDEX_TIMESTAMP, NAME_timestamp, 31, NAME_timestamp, '1')
	CREATE_MUX_ENTRY(INDEX_S0, NAME_s0, 18, NAME_s0, '1')
	CREATE_MUX_ENTRY(INDEX_S1, NAME_s1, 18, NAME_s1, '1')
	CREATE_MUX_ENTRY(INDEX_S2, NAME_s2, 18, NAME_s2, '1')
	CREATE_MUX_ENTRY(INDEX_S3, NAME_s3, 18, NAME_s3, '1')
	CREATE_MUX_ENTRY(INDEX_S4, NAME_s4, 18, NAME_s4, '1')
	CREATE_MUX_ENTRY(INDEX_S5, NAME_s5, 18, NAME_s5, '1')
	CREATE_MUX_ENTRY(INDEX_S6, NAME_s6, 18, NAME_s6, '1')

	CREATE_REGISTER(INDEX_A0, NAME_A0, 10, NAME_A0)
	CREATE_REGISTER(INDEX_A1, NAME_A1, 10, NAME_A1)
	CREATE_REGISTER(INDEX_A2, NAME_A2, 10, NAME_A2)
	CREATE_REGISTER(INDEX_A3, NAME_A3, 10, NAME_A3)

	#ifdef __IN_DECL
		signal NAME_count : std_logic_vector(30 downto 0);
		signal NAME_timestamp : std_logic_vector(30 downto 0);
		signal NAME_s0 : std_logic_vector(17 downto 0);
		signal NAME_s1 : std_logic_vector(17 downto 0);
		signal NAME_s2 : std_logic_vector(17 downto 0);
		signal NAME_s3 : std_logic_vector(17 downto 0);
		signal NAME_s4 : std_logic_vector(17 downto 0);
		signal NAME_s5 : std_logic_vector(17 downto 0);
		signal NAME_s6 : std_logic_vector(17 downto 0);
		signal NAME_int : std_logic;
	#endif

	#ifdef __IN_VHDL
		l_NAME: entity work.xufo_comm(Behavioral)
			port map (
				CLK => clk,
				reset => reset,
				RC_XUFO => NAME_rx(0),
				TX_XUFO => NAME_tx(0),
				count => NAME_count,
				timestamp => NAME_timestamp,
				s0 => NAME_s0,
				s1 => NAME_s1,
				s2 => NAME_s2,
				s3 => NAME_s3,
				s4 => NAME_s4,
				s5 => NAME_s5,
				s6 => NAME_s6,
				int => NAME_int,
				A0 => NAME_A0,
				A1 => NAME_A1,
				A2 => NAME_A2,
				A3 => NAME_A3,
				trigger => NAME_A3_store
			);
	#endif

#endbigfunc
