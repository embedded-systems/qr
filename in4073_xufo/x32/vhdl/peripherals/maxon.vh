#include "peripherals/1to1.vh"

#bigfunc ADD_MAXON_DEVICE(NAME, INDEX_32, INDEX_A, INDEX_B, INT_ERROR, INT_A, INT_B, PIN_A, PIN_B)
	ADD_1TO1_INPUT_DEVICE(NAME_A, INDEX_A, INT_A, 1, PIN_A)
	ADD_1TO1_INPUT_DEVICE(NAME_B, INDEX_B, INT_B, 1, PIN_B)

	CREATE_MUX_ENTRY(INDEX_32, NAME_dig, 32, NAME_DECODED, '1')

	REGISTER_IRQ(INT_ERROR, NAME_int, NAME_ERROR)

	#ifdef __IN_DECL
		signal NAME_dig : std_logic_vector(31 downto 0);
		signal NAME_int : std_logic;
	#endif
	#ifdef __IN_VHDL
		l_NAME: entity work.maxon(behaviour)
			port map ( 
				clk => clk, 
				reset => reset,
				a => NAME_A(0),
				b => NAME_B(0),
				err => NAME_int,
				value => NAME_dig
			);
	#endif 
#endbigfunc