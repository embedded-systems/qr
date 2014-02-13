#bigfunc ADD_SOFTINT_DEVICE(NAME, INDEX, INT_INDX)
	CREATE_WRITE_SIGNAL(INDEX, NAME_write, NAME)
	REGISTER_IRQ(INT_INDX, NAME_write, NAME)

	/*
	 * The #define is automatically created using the CREATE_MUX_ENTRY macro,
	 *	this peripheral does not use this macro, so the #define must be added
	 *	manually (Note, the INTERRUPT_ macro is created using REGISTER_IRQ)
	 */
	#ifdef __IN_HEADER
		printf("#define PERIPHERAL_toupper(NAME) INDEX")
	#endif
#endbigfunc