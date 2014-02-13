
#bigfunc REGISTER_INPUT_PIN(NAME, WIDTH, PINS)
	#ifdef __IN_SIGLIST
		pin_NAME : in std_logic_vector(unhex(WIDTH)-1 downto 0);
	#endif 
	#ifdef __IN_SIGMAPLIST
		pin_NAME => pin_NAME,
	#endif 
	#ifdef __IN_TL_DECL
		attribute LOC of pin_NAME : signal is PINS;
	#endif
	#ifdef __IN_DECL
		signal NAME : std_logic_vector(unhex(WIDTH)-1 downto 0);
	#endif
	#ifdef __IN_VHDL
		process(clk) begin
			if (clk'event and clk = '1') then
				NAME <= pin_NAME;
			end if;
		end process;
	#endif
#endbigfunc

#bigfunc REGISTER_OUTPUT_PIN(NAME, WIDTH, PINS)
	#ifdef __IN_SIGLIST
		pin_NAME : out std_logic_vector(unhex(WIDTH)-1 downto 0);
	#endif 
	#ifdef __IN_SIGMAPLIST
		pin_NAME => pin_NAME,
	#endif 
	#ifdef __IN_TL_DECL
		attribute LOC of pin_NAME : signal is PINS;
	#endif
	#ifdef __IN_DECL
		signal NAME : std_logic_vector(unhex(WIDTH)-1 downto 0);
	#endif
	#ifdef __IN_VHDL
		pin_NAME <= NAME;
	#endif
#endbigfunc

#bigfunc REGISTER_IRQ(INDEX, SIGNAME, INTNAME)
	#ifdef INTERRUPTS_ENABLE
		#ifdef __IN_VHDL
			irqs(unhex(INDEX)) <= SIGNAME;
		#endif
		#ifdef __IN_HEADER
			printf("#define INTERRUPT_toupper(INTNAME) INDEX")
		#endif
	#endif
#endbigfunc

#bigfunc CREATE_MUX_ENTRY(INDEX, SIGNAME, WIDTH, DEVNAME, SIGREADY)
	#ifdef __IN_MUX
		when pp_to_std_logic_vector(INDEX,PERIPHERAL_ADDRESS_BITS) => -- INDEX (DEVNAME)
			ready <= SIGREADY;
			assert WIDTH > PERIPHERAL_DATA_BITS
				report "Device DEVNAME incompatible with data bus"
				severity error;

			data_out(PERIPHERAL_DATA_BITS-1 downto WIDTH) <= (others => '0');
			data_out(WIDTH-1 downto 0) <= SIGNAME(WIDTH-1 downto 0);
	#endif 
	#ifdef __IN_HEADER
		printf("#define PERIPHERAL_toupper(DEVNAME) INDEX")
	#endif
#endbigfunc

#bigfunc CREATE_WRITE_SIGNAL(INDEX, NAME)
	#ifdef __IN_DECL
		signal NAME : std_logic;
	#endif
	#ifdef __IN_VHDL
		NAME <= '1' when \
			address(unhex(PERIPHERAL_ADDRESS_BITS)+1 downto 2) = \
			pp_to_std_logic_vector(INDEX, PERIPHERAL_ADDRESS_BITS) \
			and write = '1' else '0';
	#endif
#endbigfunc

#bigfunc CREATE_READ_SIGNAL(INDEX, NAME)
	#ifdef __IN_DECL
		signal NAME : std_logic;
	#endif
	#ifdef __IN_VHDL
		NAME <= '1' when \
			address(unhex(PERIPHERAL_ADDRESS_BITS)+1 downto 2) = \
			pp_to_std_logic_vector(INDEX, PERIPHERAL_ADDRESS_BITS) \
			and read = '1' else '0';
	#endif
#endbigfunc

#bigfunc CREATE_REGISTER(INDEX, SIGNAME, WIDTH, DEVNAME)
	CREATE_WRITE_SIGNAL(INDEX, SIGNAME_store)
	CREATE_MUX_ENTRY(INDEX, SIGNAME, WIDTH, DEVNAME, '1')

	#ifdef __IN_DECL
		signal SIGNAME : std_logic_vector(unhex(WIDTH)-1 downto 0);
	#endif
	#ifdef __IN_VHDL
		l_SIGNAME: entity work.reg(behaviour)
			generic map (
				size => unhex(WIDTH)
			)
			port map (
				clk => clk,
				reset => reset,
				store => SIGNAME_store,
				data_in => data_in(unhex(WIDTH)-1 downto 0),
				data_out => SIGNAME
			);
	#endif
#endbigfunc

#bigfunc CREATE_INTERRUPT_ON_CHANGE(INT_INDX, NAME, WIDTH)
	#ifdef INTERRUPTS_ENABLE
		REGISTER_IRQ(INT_INDX, '0' when NAME_prev = NAME else '1', NAME)
	
		#ifdef __IN_DECL
			signal NAME_prev : std_logic_vector(WIDTH-1 downto 0);
		#endif
		#ifdef __IN_VHDL 
			process(clk, reset) begin
				if (clk'event and clk = '1') then
					if (reset = '1') then	
						NAME_prev <= (others => '0');
					else
						NAME_prev <= NAME;
					end if;
				end if;
			end process;
		#endif 
	#endif
#endbigfunc

#bigfunc CREATE_INTERRUPT_ON_RISING_EDGE(INT_INDX, NAME)
	#ifdef INTERRUPTS_ENABLE
		REGISTER_IRQ(INT_INDX, NAME and not NAME_prev, NAME)
	
		#ifdef __IN_DECL
			signal NAME_prev : std_logic;
		#endif
		#ifdef __IN_VHDL 
			process(clk, reset) begin
				if (clk'event and clk = '1') then
					if (reset = '1') then	
						NAME_prev <= '0';
					else
						NAME_prev <= NAME;
					end if;
				end if;
			end process;
		#endif 
	#endif
#endbigfunc

#bigfunc CREATE_INTERRUPT_ON_FALLING_EDGE(INT_INDX, NAME)
	#ifdef INTERRUPTS_ENABLE
		REGISTER_IRQ(INT_INDX, NAME_prev and not NAME, NAME)
	
		#ifdef __IN_DECL
			signal NAME_prev : std_logic;
		#endif
		#ifdef __IN_VHDL 
			process(clk, reset) begin
				if (clk'event and clk = '1') then
					if (reset = '1') then	
						NAME_prev <= '0';
					else
						NAME_prev <= NAME;
					end if;
				end if;
			end process;
		#endif 
	#endif
#endbigfunc
