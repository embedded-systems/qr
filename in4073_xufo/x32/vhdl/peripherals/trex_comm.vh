
#bigfunc ADD_TREXCOMM_DEVICE(NAME, INDEX_COUNT, INDEX_TIMESTAMP, INDEX_S0, INDEX_S1, INDEX_S2, INDEX_S3, INDEX_S4, INDEX_A, IRQ_INDEX, PIN_RX, PIN_TX)

	REGISTER_INPUT_PIN(NAME_rx, 1, PIN_RX)
	REGISTER_OUTPUT_PIN(NAME_tx, 1, PIN_TX)
	REGISTER_IRQ(IRQ_INDEX, NAME_int, NAME)

	CREATE_MUX_ENTRY(INDEX_COUNT, NAME_count, 31, NAME_count, '1')
	CREATE_MUX_ENTRY(INDEX_TIMESTAMP, NAME_timestamp, 31, NAME_timestamp, '1')
	CREATE_MUX_ENTRY(INDEX_S0, NAME_s0, 11, NAME_s0, '1')
	CREATE_MUX_ENTRY(INDEX_S1, NAME_s1, 11, NAME_s1, '1')
	CREATE_MUX_ENTRY(INDEX_S2, NAME_s2, 11, NAME_s2, '1')
	CREATE_MUX_ENTRY(INDEX_S3, NAME_s3, 11, NAME_s3, '1')
	CREATE_MUX_ENTRY(INDEX_S4, NAME_s4, 11, NAME_s4, '1')

	CREATE_REGISTER(INDEX_A, NAME_A, 8, NAME_A)

	#ifdef __IN_DECL
		signal NAME_count : std_logic_vector(30 downto 0);
		signal NAME_timestamp : std_logic_vector(30 downto 0);
		signal NAME_s0 : std_logic_vector(10 downto 0);
		signal NAME_s1 : std_logic_vector(10 downto 0);
		signal NAME_s2 : std_logic_vector(10 downto 0);
		signal NAME_s3 : std_logic_vector(10 downto 0);
		signal NAME_s4 : std_logic_vector(10 downto 0);
		signal NAME_int : std_logic;
	#endif

	#ifdef __IN_VHDL
		l_NAME: entity work.trex_comm(Behavioral)
			port map (
				CLK => clk,
				reset => reset,
				RC_TREX => NAME_rx(0),
				TX_TREX => NAME_tx(0),
				count => NAME_count,
				timestamp => NAME_timestamp,
				s0 => NAME_s0,
				s1 => NAME_s1,
				s2 => NAME_s2,
				s3 => NAME_s3,
				s4 => NAME_s4,
				int => NAME_int,
				a => data_in(7 downto 0),
				send => NAME_A_store
			);
	#endif

#endbigfunc
