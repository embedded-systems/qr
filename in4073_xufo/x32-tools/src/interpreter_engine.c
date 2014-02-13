/* see interpreter_engine.h for general information */
#include "interpreter_engine.h"
#include "interpreter_peripherals.h"

/* return the minimum of 2 integers */
int min(int a, int b) { return (a < b ? a : b); }
/* return the maximum of 2 integers */
int max(int a, int b) { return (a > b ? a : b); }
/* do a binary compare instruction (works on any size of operand) */
int compare_instruction(Interpreter*, int, int, int, unsigned char*, unsigned char*, BOOL*);

int alu(int, Value*, Value*, Value**);
int compare(int, Value*, Value*, BOOL*);
int convert(Value*, Value*);

/*
 * create a new interpreter info structure, and allocates memory
 */
Interpreter* create_interpreter(int memory_size) {
	int i;
	Interpreter* interpreter;
	/* make info structure */
	interpreter = (Interpreter*)malloc(sizeof(Interpreter));
	if (!interpreter) return 0;
	/* allocate memory */
	interpreter->memory_size = memory_size;
	interpreter->memory = (unsigned char*)malloc(memory_size);
	interpreter->allow_discard = ALLOW_DISCARD;
	
	for (i = 0; i < MAX_OPCODE_ID; i++) interpreter->analysis[i] = 0;
	
	/* check memory */
	if (!interpreter->memory) {
		free(interpreter);
		return 0;
	}
	
	/* return */
	return interpreter;
}

/*
 * reset the interpreters registers
 */
void reset(Interpreter* interpreter) {
	/* start executing at address 0 */
	interpreter->pc = interpreter->el = 0;
	/* stack starts from the top end of the memory */
	interpreter->ap = interpreter->lp = interpreter->fp = interpreter->sp = interpreter->program_size;
	interpreter->stack_offset = interpreter->sp;
	interpreter->executed = interpreter->stackpeak = 0;
	interpreter->aligned = interpreter->unaligned = 0;
	interpreter->overflow = interpreter->div0 = interpreter->trapped = 
		interpreter->out_of_memory = FALSE;
}

/*
 * load a program file into the interpreters memory
 */
BOOL load_program(FILE* file, Interpreter* interpreter, int offset) {
	void* ptr;
	
	/* check file pointer */
	if (!file) return FALSE;
	
	/* start address */
	ptr = interpreter->memory + offset;
	
	/* check the size */
	fseek(file, 0, SEEK_END);
	if ((interpreter->program_size = ftell(file) + offset) > interpreter->memory_size) {
		return FALSE;
	}

	/* copy the file */
	fseek(file, 0, SEEK_SET);
	while (!feof(file)) ptr += fread(ptr, 1, BUFF_SIZE, file);

	/* reset the interpreters state */
	reset(interpreter);

	return TRUE;
}

/*
 * destroy an interpreter info structure en deallocate memory
 */
void destroy_interpreter(Interpreter* interprerer) {
	/* deallocate interpreter memory */
	free(interprerer->memory);
	/* deallocate info structure */
	free(interprerer);
}

/*
 * setup the stack for a function call
 */
int setup_call(Interpreter* interpreter, Value* params[], int count, Value* result) {
	int i, j;
	
	/* add all parameters to the stack */
	interpreter->fp = interpreter->sp;
	for (i = 0; i < count; i++) {
		push_value(interpreter, params[i]);
	}

	/* make room for return value and return address */
	interpreter->sp -= result->size; 
	interpreter->sp -= PTR_SIZE;

	if (interpreter->sp < interpreter->program_size) return ERR_OUT_OF_STACK;

	/* set return address to 0 */
	for (i = 0; i < PTR_SIZE; i++) interpreter->memory[interpreter->sp+j] = 0;
	
	return ERR_NOERROR;
}

/*
 * jump to a specified address
 */
BOOL jump(Interpreter* interpreter, int address) {
	interpreter->pc = address;
}

/*
 * log a memory action (for statistics)
 */
void log_memory_action(Interpreter* interpreter, int address, int size) {
	switch(size) {
		case 4:
			if ((address % 4) == 0) {
				interpreter->aligned++;
			} else {
				interpreter->unaligned++;
			}
			break;
		case 2:
			if ((address % 4) == 3) {
				interpreter->unaligned++;
			} else {
				interpreter->aligned++;
			}
			break;
		default:
			interpreter->aligned++;
			break;
	}
}

/*
 * return the top of the stack
 */
BOOL pop_value(Interpreter* interpreter, Value* value) {
	if (interpreter->sp > OOM_LOWER_BOUND && interpreter->sp < OOM_UPPER_BOUND) {
		interpreter->out_of_memory = TRUE;
	}


	if (interpreter->memory_size >= interpreter->sp && interpreter->sp - value->size >= 0) {
		log_memory_action(interpreter, interpreter->sp, value->size);
		interpreter->sp -= value->size;
		memcpy(value->data, &interpreter->memory[interpreter->sp], value->size);
		return TRUE;
	} else {
		return FALSE;
	} 
}

/*
 * add value to stack
 */
BOOL push_value(Interpreter* interpreter, Value* value) {
	int i;
	
	if (interpreter->sp > OOM_LOWER_BOUND && interpreter->sp < OOM_UPPER_BOUND) {
		interpreter->out_of_memory = TRUE;
	}


	if (interpreter->memory_size >= interpreter->sp + value->size && interpreter->sp >= 0) {
		log_memory_action(interpreter, interpreter->sp, value->size);
		memcpy(&interpreter->memory[interpreter->sp], value->data, value->size);
		interpreter->sp += value->size;
		return TRUE;
	} else {
		return FALSE;
	}
}

/*
 * return any value from memory
 */
BOOL get_value(Interpreter* interpreter, Value* value, int address) {
	Value* tmp;

	if (address > OOM_LOWER_BOUND && address < OOM_UPPER_BOUND) {
		interpreter->out_of_memory = TRUE;
	}

	if (address >= ADDR_PERIPHERALS) {
		tmp = get_peripheral_value(interpreter, (address-ADDR_PERIPHERALS) >> 2);
		if (tmp == 0) tmp = create_unsigned(0, value->size);
		memcpy(value->data, tmp->data, value->size);
		destroy_value(tmp);
		return TRUE;
	} else if (interpreter->memory_size >= address + value->size && address >= 0) {
		log_memory_action(interpreter, address, value->size);
		memcpy(value->data, &interpreter->memory[address], value->size);
		return TRUE;
	} else {
		return FALSE;
	} 
}

/*
 * set any value in the memory
 */
BOOL set_value(Interpreter* interpreter, Value* value, int address) {
	if (address > OOM_LOWER_BOUND && address < OOM_UPPER_BOUND) {
		interpreter->out_of_memory = TRUE;
	}

	if (address >= ADDR_PERIPHERALS) {
		set_peripheral_value(interpreter, (address - ADDR_PERIPHERALS) >> 2, value);
		return TRUE;
	} else if (interpreter->memory_size >= address + value->size && address >= 0) {
		log_memory_action(interpreter, address, value->size);
		memcpy(&interpreter->memory[address], value->data, value->size);
		return TRUE;
	} else {
		return FALSE;
	} 
}

/*
 * return any value from memory
 */
BOOL get_parameter(Interpreter* interpreter, Value* value) {
	if (interpreter->pc + OPCSIZE > OOM_LOWER_BOUND && interpreter->pc + OPCSIZE < OOM_UPPER_BOUND) {
		interpreter->out_of_memory = TRUE;
	}

	if (interpreter->memory_size >= interpreter->pc + OPCSIZE + value->size && interpreter->pc >= 0) {
		log_memory_action(interpreter, interpreter->pc+OPCSIZE, value->size);
		memcpy(value->data, &interpreter->memory[interpreter->pc+OPCSIZE], value->size);
		return TRUE;
	} else {
		return FALSE;
	} 
}

/*
 * interrupt the interpreter (make a call to address)
 */
int interrupt(Interpreter* interpreter, int address, int priority) {
	Value *param;		/* values for two operands, a parameter and a result */

	/* untrap interpreter: */
	interpreter->trapped = FALSE;

	/* reset flags */
	interpreter->overflow = interpreter->div0 = interpreter->out_of_memory = FALSE;

	/* save pc */
	param = create_ptr(interpreter->pc);
	if (!push_value(interpreter, param)) {
		destroy_value(param);
		return ERR_MEMWRITE_ERROR;
	}
	
	/* should be, since interrupt support wasn't there before 0.2.0 */
	if (interpreter->vid >= 0x0020) {
		/* save el */
		param = create_ptr(interpreter->el);
		if (!push_value(interpreter, param)) {
			destroy_value(param);
			return ERR_MEMWRITE_ERROR;
		}

		/* save ap */
		param = create_ptr(interpreter->ap);
		if (!push_value(interpreter, param)) {
			destroy_value(param);
			return ERR_MEMWRITE_ERROR;
		}
		
		/* save lp */
		param = create_ptr(interpreter->lp);
		if (!push_value(interpreter, param)) {
			destroy_value(param);
			return ERR_MEMWRITE_ERROR;
		}
	
		/* save fp */
		param = create_ptr(interpreter->fp);
		if (!push_value(interpreter, param)) {
			destroy_value(param);
			return ERR_MEMWRITE_ERROR;
		}
		
		interpreter->ap = interpreter->fp;
		interpreter->lp = interpreter->sp;
		interpreter->fp = interpreter->sp;
	}
	
	/* jump */
	interpreter->pc = address;
	interpreter->el = priority;	
	
	interpreter->executed++;

	return ERR_NOERROR;
}

/*
 * execute the next instruction
 */
int execute(Interpreter* interpreter) {
	/* temporaries */
	unsigned long long u;
	signed long long i;
	
	int opcode;			/* instruction opcode */
	int size;				/* operand size */
	int type;				/* operand type */

	int error;			/* error code */

	BOOL jump;			/* compare instruction result */

	/* exit the execution loop */
	BOOL exit;
	exit = FALSE;
	
	Value *a,*b,*param,*result;		/* values for two operands, a parameter and a result */
	
	/* if a trap is not handled, the interpreter generates an exit code */
	if (interpreter->trapped) return ERR_TRAPPED;

	/* decode */
	#ifdef ALLOW_EXEC_OUTSIDE_CODEMEM
		if (interpreter->memory_size < interpreter->pc+2 || interpreter->pc < 0) return ERR_PC_ERROR;
	#else
		if (interpreter->program_size < interpreter->pc+2 || interpreter->pc < 0) return ERR_PC_ERROR;
	#endif
	log_memory_action(interpreter, interpreter->pc, 2);
	decode(&interpreter->memory[interpreter->pc], &opcode, &type, &size);

	/* reset flags */
	interpreter->overflow = interpreter->div0 = interpreter->out_of_memory = FALSE;

	a = b = param = result = 0;
	
	if (opcode_id(opcode) < MAX_OPCODE_ID && opcode_id(opcode) > 0)
		interpreter->analysis[opcode_id(opcode)]++;
	
	//printf("0x%04X: %s\n", interpreter->pc, opcode_name(opcode)); fflush(stdout);

	/* execute instruction */
	switch(opcode) {
		case ADDRF: case ADDRL: case ADDRA: case ADDR_VR: case ADDR_EL: case ADDR_SP:
			/* ADDRFP4 */
			/* ADDRLP4 */
			/* ADDRAP4 */

			/* SAVEAP */
			/* SAVEFP */
			/* SAVELP */
			/* SAVESP */
			/* SAVEVR */
			/* SAVEEL */
			if (type == POINTER && size == PTR_SIZE) {
				/* load parameter */
				param = create_ptr(0);
				if (!get_parameter(interpreter, param)) {
					destroy_value(param);
					return ERR_MEMREAD_ERROR;
				}
				/* load pointer */
				switch(opcode) {
					case ADDRA:
						result = create_ptr(interpreter->fp + parse_ulong(param));
						break;
					case ADDRF:
						result = create_ptr(interpreter->ap + parse_ulong(param));
						break;
					case ADDRL:
						result = create_ptr(interpreter->lp + parse_ulong(param));
						break;
					case ADDR_VR:
						result = create_ptr(interpreter->vid + parse_ulong(param));
						break;
					case ADDR_EL:
						result = create_ptr(interpreter->el + parse_ulong(param));
						break;
					case ADDR_SP:
						result = create_ptr(interpreter->sp + parse_ulong(param));
						break;
				}
				/* push */
				if (!push_value(interpreter, result)) {
					destroy_value(param);
					destroy_value(result);
					return ERR_MEMWRITE_ERROR;
				}
				/* update PC */
				interpreter->pc += OPCSIZE + INT_SIZE;
			} else {
				return ERR_INVALID_TYPE;
			}
			break;
		case ADDRG:
			/* ADDRGP4 */
			if (type == POINTER && size == PTR_SIZE) {
				/* load parameter */
				param = create_ptr(0);
				if (!get_parameter(interpreter, param)) {
					destroy_value(param);
					return ERR_MEMREAD_ERROR;
				}
				/* push */
				if (!push_value(interpreter, param)) {
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}
				/* update PC */
				interpreter->pc += OPCSIZE + PTR_SIZE;
			} else {
				return ERR_INVALID_TYPE;
			}
			break;
		case CNST:
			/* CNSTF4 CNSTI1 CNSTI2 CNSTI4 CNSTP4 CNSTU1 CNSTU2 CNSTU4 */
			switch(type) {
				case INTEGER: 
					param = create_integer(0, size);
					break;
				case UNSIGNED: case POINTER:
					param = create_unsigned(0, size);
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			/* get parameter */
			if (!get_parameter(interpreter, param)) {
				destroy_value(param);
				return ERR_MEMREAD_ERROR;
			}
			/* push parameter */
				if (!push_value(interpreter, param)) {
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}
			/* update PC */
			interpreter->pc += OPCSIZE + size;
			break;
		case BCOM: case NEG:
			/* BCOMI4 BCOMU4 */
			switch(type) {
				case UNSIGNED:
					a = create_unsigned(0, size);
					break;
				case INTEGER:
					a = create_integer(0, size);
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			/* pop value */
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				destroy_value(result);
				return ERR_MEMREAD_ERROR;
			}
			/* ALU instruction */
			if ((error = alu(opcode, a, 0, &result)) != ERR_NOERROR) {
				destroy_value(a);
				if (result) destroy_value(result);
				return error;
			}
			/* push to stack */
			if (!push_value(interpreter, result)) {
				destroy_value(a);
				destroy_value(result);
				return ERR_MEMWRITE_ERROR;
			}
			/* update PC */
			interpreter->pc += OPCSIZE;
			break;
		case INDIR:
			/* INDIRB INDIRF4 INDIRI1 INDIRI2 INDIRI4 INDIRP4 INDIRU1 INDIRU2 INDIRU4 */
			/* read pointer from stack */
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}

			/*
			param = create_int(0);
			if (!get_parameter(interpreter, param)) {
				destroy_value(param);
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			*/
			
			switch(type) {
				case INTEGER:
					b = create_integer(0, size);
					break;
				case UNSIGNED:
					b = create_unsigned(0, size);
					break;
				case POINTER:
					b = create_ptr(0);
					break;
				/*					
				case STRUCT:
					/* TODO: BOUNDARY CHECK! *//*
					memcpy(&interpreter->memory[interpreter->sp], &interpreter->memory[parse_ulong(a)], parse_ulong(param));
					interpreter->sp += parse_ulong(param);
					break;
				*/
				default:
					destroy_value(a);
					destroy_value(b);
					destroy_value(param);
					return ERR_INVALID_TYPE;
			}
			
			if (type != STRUCT) {
				/* read value */
				if (!get_value(interpreter, b, parse_ulong(a))) {
					destroy_value(a);
					destroy_value(b);
					return ERR_MEMREAD_ERROR;
				}
				
				/* push value */
				if (!push_value(interpreter, b)) {
					destroy_value(a);
					destroy_value(b);
					return ERR_MEMWRITE_ERROR;
				}
			}
			
			/* update PC */
			/*interpreter->pc += OPCSIZE+INT_SIZE;*/
			interpreter->pc += OPCSIZE;
			break;
		case ADD: case BAND: case BOR: case BXOR: case DIV: case LSH: 
		case MOD: case MUL: case RSH: case SUB:
			/* ADDF4 ADDI4 ADDU4 ADDP4 */
			/* BANDI4 BANDU4 */
			/* BORI4 BORU4 */
			/* BXORI4 BXORU4 */
			/* DIVF4 DIVI4 DIVU4 DIVP4 */
			/* LSHI4 LSHU4 */
			/* MODF4 MODI4 MODU4 */
			/* MULF4 MULI4 MULU4 */
			/* RSHI4 RSHU4
			/* SUBF4 SUBI4 SUBU4 */

			/* get operands from stack */
			switch(type) {
				case INTEGER:
					a = create_integer(0, size);
					b = create_integer(0, size);
					result = create_integer(0, size);
					break;
				case UNSIGNED:
					a = create_unsigned(0, size);
					b = create_unsigned(0, size);
					result = create_unsigned(0, size);
					break;
				case POINTER:
					a = create_ptr(0);
					b = create_ptr(0);
					result = create_ptr(0);
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			if (!pop_value(interpreter, a) || !pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				destroy_value(result);
				return ERR_MEMREAD_ERROR;
			}
			
			/* do the math */
			error = alu(opcode, b, a, &result);
			switch(error) {
				case ERR_NOERROR: 
					break;
				case ERR_OVERFLOW:
					interpreter->overflow = TRUE;
					break;
				case ERR_DIV0:
					interpreter->div0 = TRUE;
					break;
				default:
					destroy_value(a);
					destroy_value(b);
					destroy_value(result);
					return error;
			}
			
			/* push value back to stack */
			if (!push_value(interpreter, result)) {
				destroy_value(a);
				destroy_value(b);
				destroy_value(result);
				return ERR_MEMWRITE_ERROR;
			}

			/* update PC */
			interpreter->pc += OPCSIZE;
			break;
		case ASGN:
			/* ASGNB ASGNF4 ASGNI1 ASGNI2 ASGNI4 ASGNP4 ASGNU1 ASGNU2 ASGNU4 */
			
			/*
			param = create_int(0);
			if (!get_parameter(interpreter, param)) {
				destroy_value(param);
				return ERR_MEMREAD_ERROR;
			}
			*/

			switch(type) {
				case INTEGER:
					b = create_integer(0, size);
					break;
				case UNSIGNED:
					b = create_unsigned(0, size);
					break;
				case POINTER:
					b = create_ptr(0);
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			
			if (type != STRUCT) {
				a = create_ptr(0);
	
				/* get address and value */
				if (!pop_value(interpreter, b) || !pop_value(interpreter, a)) {
					destroy_value(a);
					destroy_value(b);
					return ERR_MEMREAD_ERROR;
				}
				
				/* write value */
				if (!set_value(interpreter, b, parse_ulong(a))) {
					destroy_value(a);
					destroy_value(b);
					return ERR_MEMWRITE_ERROR;
				}
			}
			
			/* update PC */
			/*interpreter->pc += OPCSIZE+INT_SIZE;*/
			interpreter->pc += OPCSIZE;
			break;
		case EQ: case GT: case GE: case LT: case LE: case NE:
			switch (type) {				
				case INTEGER:
					a = create_integer(0, size);
					b = create_integer(0, size);
					break;
				case UNSIGNED:
					a = create_unsigned(0, size);
					b = create_unsigned(0, size);
					break;
				default:
					return ERR_INVALID_TYPE;
			}

			if (!pop_value(interpreter, a) || !pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}

			if ((error = compare(opcode, b, a, &jump)) != ERR_NOERROR) {
				destroy_value(a);
				destroy_value(b);
				return error;
			}

			param = create_ptr(0);
			if (!get_parameter(interpreter, param)) {
				destroy_value(a);
				destroy_value(b);
				destroy_value(param);
				return ERR_MEMREAD_ERROR;
			}

			if (jump) {
				interpreter->pc = parse_ulong(param);
			} else {
				interpreter->pc += OPCSIZE + PTR_SIZE; 
			}

			break; 
		case ARG:
			/* ARGB ARGF4 ARGI4 ARGP4 ARGU4 */
			if (interpreter->vid < 0x0021) {
				param = create_int(0);
				if (!get_parameter(interpreter, param)) {
					destroy_value(param);
					return ERR_MEMREAD_ERROR;
				}
				
				switch(type) {
					case INTEGER:
						a = create_integer(0, size);
						break;
					case UNSIGNED: case POINTER:
						a = create_unsigned(0, size);
						break;
					default:
						destroy_value(param);
						return ERR_INVALID_TYPE;
				}
				
				if (!pop_value(interpreter, a)) {
					destroy_value(a);
					destroy_value(param);
					return ERR_MEMREAD_ERROR;
				}
				
				if(!set_value(interpreter, a, parse_int(param)+interpreter->fp)) {
					destroy_value(param);
					destroy_value(a);
					return ERR_MEMWRITE_ERROR;
				}
				
				interpreter->pc += OPCSIZE + INT_SIZE; 
			} else {
				// ARG no longer supported
				return ERR_INVALID_OPCODE;
			}
			break;
		case CALL:
			/* CALLF4 CALLI4 CALLP4 CALLU4 CALLV */
			
	
			b = create_ptr(0);
			if (!pop_value(interpreter, b)) {
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
/*
			printf("CALL to %08X:\n", parse_ulong(b));
			printf("\tEL: %08X\n", interpreter->el);
			printf("\tAP: %08X\n", interpreter->ap); 
			printf("\tLP: %08X\n", interpreter->lp);
			printf("\tFP: %08X\n", interpreter->fp);
			printf("\n");
			printf("\tSP: %08X\n", interpreter->sp);
*/						
			/* save pc */
			param = create_ptr(interpreter->pc+OPCSIZE);
			if (!push_value(interpreter, param)) {
				destroy_value(b);
				destroy_value(param);
				return ERR_MEMWRITE_ERROR;
			}
			
			if (interpreter->vid >= 0x0020) {
				/* save el */
				param = create_ptr(interpreter->el);
				if (!push_value(interpreter, param)) {
					destroy_value(b);
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}

				/* save ap */
				param = create_ptr(interpreter->ap);
				if (!push_value(interpreter, param)) {
					destroy_value(b);
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}
				
				/* save lp */
				param = create_ptr(interpreter->lp);
				if (!push_value(interpreter, param)) {
					destroy_value(b);
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}
		
				/* save fp */
				param = create_ptr(interpreter->fp);
				if (!push_value(interpreter, param)) {
					destroy_value(b);
					destroy_value(param);
					return ERR_MEMWRITE_ERROR;
				}
				
				interpreter->ap = interpreter->fp;
				interpreter->lp = interpreter->sp;
				interpreter->fp = interpreter->sp;
			}
	
			
			/* jump */
			interpreter->pc = parse_ulong(b);

			break;
		case RET:
			/* RETF4 RETI4 RETP4 RETU4 RETV */
			
			switch(type) {
				case INTEGER:
					a = create_integer(0, size);
					break;
				case POINTER: case UNSIGNED:
					a = create_unsigned(0, size);
					break;
				case VOID:
					a = create_void();
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			
			/* pop return value */
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			
			/* restore SP */
			interpreter->sp = interpreter->fp;
			
			/* restore FP */
			b = create_ptr(0);
			if (!pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->fp = parse_ulong(b);
			/* restore LP */
			if (!pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->lp = parse_ulong(b);
			/* restore AP */
			if (!pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->ap = parse_ulong(b);
			/* restore EL */
			if (!pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->el = parse_ulong(b);
			/* restore PC */
			if (!pop_value(interpreter, b)) {
				destroy_value(a);
				destroy_value(b);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->pc = parse_ulong(b);
			
			/* push return value */
			push_value(interpreter, a);
			break; 
		case JUMP:
			/* JUMPV */
			if (type == VOID || type == POINTER){
				/* read new address */
				a = create_ptr(0);
				if (!pop_value(interpreter, a)) {
					destroy_value(a);
					return ERR_MEMREAD_ERROR;
				}
				/* update pc */
				interpreter->pc = parse_ulong(a);
			} else {
				return ERR_INVALID_TYPE;
			}
			break;
/*			
		case NEWSTACK:
			a = create_ptr(interpreter->ap);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->ap  = interpreter->lp = interpreter->fp = interpreter->sp = parse_ulong(a);
			interpreter->pc += OPCSIZE;
			break;
		case SAVESTATE:
			a = create_ptr(interpreter->ap);
			push_value(interpreter, a);
			destroy_value(a);
			a = create_ptr(interpreter->lp);
			push_value(interpreter, a);
			destroy_value(a);
			a = create_ptr(interpreter->fp);
			push_value(interpreter, a);
			destroy_value(a);
			a = 0;
			
			interpreter->pc += OPCSIZE;
			break;
*/
/*
		case SAVEAP:
			a = create_ptr(interpreter->ap);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
		case SAVEFP:
			a = create_ptr(interpreter->fp);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
		case SAVELP:
			a = create_ptr(interpreter->lp);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
		case SAVESP:
			a = create_ptr(interpreter->sp);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
		case SAVEVR:
			a = create_ptr(interpreter->vid);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
		case SAVEEL:
			a = create_ptr(interpreter->el);
			push_value(interpreter, a);

			interpreter->pc += OPCSIZE;
			break;
*/
/*
		case LOADAP:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->ap = parse_ulong(a);

			interpreter->pc += OPCSIZE;
			break;
		case LOADFP:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->fp = parse_ulong(a);

			interpreter->pc += OPCSIZE;
			break;
		case LOADLP:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->lp = parse_ulong(a);

			interpreter->pc += OPCSIZE;
			break;
		case LOADSP:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->sp = parse_ulong(a);

			interpreter->pc += OPCSIZE;
			break;
		case LOADVR:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}

			interpreter->pc += OPCSIZE;
			break;
		case LOADEL:
			a = create_ptr(0);
			if (!pop_value(interpreter, a)) {
				destroy_value(a);
				return ERR_MEMREAD_ERROR;
			}
			interpreter->el = parse_ulong(a);

			interpreter->pc += OPCSIZE;
			break;
*/
		case MOVESTACK:
			/* read argument stack size */
			param = create_int(0);
			get_parameter(interpreter, param);
			/* set FP */
			if (interpreter->vid < 0x0020) {
				interpreter->ap = interpreter->fp;
				interpreter->fp = interpreter->sp;
			}
			/* increase stack */
			interpreter->sp = parse_int(param);
			if (interpreter->vid >= 0x0020) {
				interpreter->lp = interpreter->sp;
			}
			/* update PC */
			interpreter->pc += OPCSIZE + INT_SIZE;
			break;
		case ARGSTACK:
			/* read argument stack size */
			param = create_int(0);
			get_parameter(interpreter, param);
			/* set FP */
			if (interpreter->vid < 0x0020) {
				interpreter->ap = interpreter->fp;
				interpreter->fp = interpreter->sp;
			}
			/* increase stack */
			interpreter->sp += parse_int(param);
			if (interpreter->vid >= 0x0020) {
				interpreter->lp = interpreter->sp;
			}
			/* update PC */
			interpreter->pc += OPCSIZE + INT_SIZE;
			break;
		case VARSTACK:
			/* read argument stack size */
			param = create_int(0);
			get_parameter(interpreter, param);
			if (interpreter->vid < 0x0020) {
				/* set LP */
				interpreter->lp = interpreter->sp;
			}
			/* increase stack */
			interpreter->sp += parse_int(param);
			/* update PC */
			interpreter->pc += OPCSIZE + INT_SIZE;
			break;
/*	SYSCALL IS NO LONGER SUPPORTED, use get_value & set_value to control peripherals
		case SYSCALL:
			a = create_int(0);  
			pop_value(interpreter, a);
			switch(parse_int(a)) {
				case SC_READ:
					b = create_int(read_char());
					push_value(interpreter, b);
					break;
				case SC_READX:
					b = create_int(read_char_non_blocking());
					push_value(interpreter, b);
					break;
				case SC_WRITE:
					b = create_int(0);
					pop_value(interpreter, b);
					write_char(parse_int(b));
					break;
				case SC_RAND:
					b = create_int(rand());
					push_value(interpreter, b);
					break;
				case SC_CLOCK:
					b = create_int(interpreter->executed);
					push_value(interpreter, b);
					break;
				default:
					b = create_int(-1);
					push_value(interpreter, b);
					break;
			}
			interpreter->pc += OPCSIZE;
			break;
*/
		case DISCARD:
			if (interpreter->allow_discard) {
				interpreter->sp -= size;
			} else {
				interpreter->executed--;   
			}
			interpreter->pc += OPCSIZE;
			break;
//		case HALT:
			/* exit program */
//			exit = TRUE;
//			break;
		default:
			if ((opcode & 0xFF00) == CV) {
				/* from */
				switch(type) {
					case INTEGER:
						a = create_integer(0, size);
						break;
					case UNSIGNED:
						a = create_unsigned(0, size);
						break;
					case POINTER:
						a = create_ptr(0);
						break;
					default:
						return ERR_INVALID_TYPE;
				}
				
				/* to */
				if (opcode & 0x10) {
					/* signed */
					switch((opcode & 0xE0) >> 4) {
						case TYPE_CHAR:
							b = create_integer(0, 1);
							break;
						case TYPE_SHORT:
							b = create_integer(0, SHRT_SIZE);
							break;
						case TYPE_INT:
							b = create_integer(0, INT_SIZE);
							break;
						case TYPE_LONG:
							b = create_integer(0, LNG_SIZE);
							break;
						case TYPE_LONG2:
							/* long long is not supported */
							b = create_integer(0, LNG_SIZE);
							break;
						default:
							destroy_value(a);
							return ERR_INVALID_TYPE;
					}
					
				} else {
					/* unsigned */
					
					switch((opcode & 0xE0) >> 4) {
						case TYPE_CHAR:
							b = create_unsigned(0, 1);
							break;
						case TYPE_SHORT:
							b = create_unsigned(0, SHRT_SIZE);
							break;
						case TYPE_INT:
							b = create_unsigned(0, INT_SIZE);
							break;
						case TYPE_LONG:
							b = create_unsigned(0, LNG_SIZE);
							break;
						case TYPE_LONG2:
							/* long long is not supported */
							b = create_unsigned(0, LNG_SIZE);
							break;
						case TYPE_POINTER:
							b = create_ptr(0);
							break;
						default:
							destroy_value(a);
							return ERR_INVALID_TYPE;
					}

				}
				
				if (!pop_value(interpreter, a)) {
					destroy_value(param);
					destroy_value(a);
					return ERR_MEMREAD_ERROR;
				}

				/* convert */
				error = convert(a,b);
				switch(error) {
					case ERR_OVERFLOW:
						interpreter->overflow = TRUE;
					case ERR_NOERROR:
						if (!push_value(interpreter, b)) {
							destroy_value(a);
							destroy_value(b);
							destroy_value(param);
							return ERR_MEMWRITE_ERROR;
						}
						break;
					default:
						destroy_value(a);
						destroy_value(b);
						destroy_value(param);
						return error;
				}

				/* update PC */
				interpreter->pc += OPCSIZE;				
	
			} else if (opcode & TRAP) {
				interpreter->trapped = TRUE;
			} else {
				/* unknown opcode */
				return ERR_INVALID_OPCODE;
			}
	}
	
	if (a) destroy_value(a);
	if (b) destroy_value(b);
	if (result) destroy_value(result);
	if (param) destroy_value(param);

	if (interpreter->sp >= interpreter->memory_size) return ERR_OUT_OF_STACK;
	
	interpreter->executed++;
	if (interpreter->sp - interpreter->stack_offset > interpreter->stackpeak) {
		interpreter->stackpeak = interpreter->sp - interpreter->stack_offset;
	}

	if (exit) { 
		return ERR_EXIT_PROGRAM;
	} else {
		return ERR_NOERROR;
	}
}

/*
 * execute compare instruction (works on any parameter size)
 */
int compare(int opcode, Value* operand1, Value* operand2, BOOL* result) {
	int i;
	
	if (operand1->type != operand2->type || operand1->size != operand2->size) return ERR_INVALID_TYPE;
	
	if (operand1->type == INTEGER && SIGN(operand1->data) != SIGN(operand2->data)) {
		if (SIGN(operand1->data)) {
			/* operand1 is positive, operand2 is negative */
			switch(opcode) {
				case EQ: case LE: case LT: 
					*result = FALSE;
					return ERR_NOERROR;
				case NE: case GE: case GT: 
					*result =  TRUE;
					return ERR_NOERROR;
			}
		} else {
			/* operand1 is negative, operand2 is positive */
			switch(opcode) {
				case EQ: case GE: case GT: 
					*result =  FALSE;
					return ERR_NOERROR;
				case NE: case LE: case LT: 
					*result =  TRUE;
					return ERR_NOERROR;
			}
		}
	} else if (operand1->type == INTEGER || operand1->type == UNSIGNED) {
		/* unsigned or both parameters > 0 */
		for (i = 0; i < operand1->size; i++) {
			if (operand1->data[i] > operand2->data[i]) {
				/* greater */
				switch(opcode) {
					case EQ: 
						*result =  FALSE;
						return ERR_NOERROR;
					case LE: case LT: 
						*result =  FALSE;
						return ERR_NOERROR;
					case NE: 
						*result = TRUE;
						return ERR_NOERROR;
					case GE: case GT: 
						*result =  TRUE;
						return ERR_NOERROR;
				}
			} else if (operand1->data[i] < operand2->data[i]) {
				/* lesser */
				switch(opcode) {
					case EQ: 
						*result = FALSE;
						return ERR_NOERROR;
					case GE: case GT: 
						*result = FALSE;
						return ERR_NOERROR;
					case NE: 
						*result =  TRUE;
						return ERR_NOERROR;
					case LE: case LT: 
						*result =  TRUE;
						return ERR_NOERROR;
				}
			}
		}
		/* equal */
		switch(opcode) {
			case EQ: 
				*result =  TRUE;
				return ERR_NOERROR;
			case GE: case LE: 
				*result =  TRUE;
				return ERR_NOERROR;
			case NE: 
				*result =  FALSE;
				return ERR_NOERROR;
			case GT: case LT: 
				*result =  FALSE;
				return ERR_NOERROR;
		}
	} else {
		return ERR_INVALID_TYPE;
	}
	return ERR_NOERROR;
}

int sign(int i) {
	return i>0?0:1;
}

/*
 * execute ALU instruction
 */
int alu(int opcode, Value* operand1, Value* operand2, Value** result) {
	unsigned long long u1, u2, ur;
	signed long long i1, i2, ir;

	if ((opcode == DIV || opcode == MOD) && parse_long(operand2) == 0) {
		if (*result) destroy_value(*result);
		*result = create_integer(0, operand1->size);
		return ERR_DIV0;
	}

	if (opcode == NEG || opcode == BCOM) {
	} else {
		if (!(operand1->type == operand2->type)) {
			return ERR_INVALID_TYPE;
		}
		if (!(operand1->size == operand2->size)) {
			return ERR_INVALID_TYPE;
		}
	}
	
	/* remake result variable */
	if (*result) destroy_value(*result);
	*result = 0;

	switch(operand1->type) {
		case INTEGER:
			switch(opcode) {
				case NEG: case BCOM:
					i1 = parse_long(operand1);
					break;
				case ADD: case BAND: case BOR: case BXOR: case DIV: case LSH: 
				case MOD: case MUL: case RSH: case SUB:
					i1 = parse_long(operand1);
					i2 = parse_long(operand2);
					break;
			}
		
			switch(opcode) {
				case NEG: 		ir = -i1;				break;
				case BCOM:		ir = ~i1;				break;
				case ADD: 		ir = i1 + i2;		break;
				case BAND: 		ir = i1 & i2;		break;
				case BOR: 		ir = i1 | i2;		break;
				case BXOR: 		ir = i1 ^ i2;		break;
				case DIV: 		ir = i1 / i2;		break;
				case LSH: 		ir = i1 << i2;	break;
				case MOD: 		ir = i1 % i2;		break;
				case MUL: 		ir = i1 * i2;		break;
				case RSH: 		ir = i1 >> i2;	break;
				case SUB:			ir = i1 - i2;		break;
				default:			return ERR_INVALID_OPCODE;
			}

			*result = create_integer(ir, operand1->size);

			switch(opcode) {
				case ADD:
					if (sign((int)i1) == sign((int)i2)) {
						if (sign((int)i1) != sign((int)ir)) {
							return ERR_OVERFLOW;
						}
					}
					break;
				case SUB:
					if (sign((int)i1) != sign((int)i2)) {
						if (sign((int)i1) != sign((int)ir)) {
							return ERR_OVERFLOW;
						}
					}
					break;
				case MUL:
					if (i1 != 0 && i2 != 0) {
						/* sign check */
						if (sign((int)i1) && sign((int)i2)) {
							/* neg * neg should be pos */
							if (sign(ir)) {
								return ERR_OVERFLOW;
							}
						} else if (sign((int)i1) && !sign((int)i2)) {
							/* neg * pos should be neg */
							if (!sign(ir)) {
								return ERR_OVERFLOW;
							}
						} else if (!sign((int)i1) && sign((int)i2)) {
							/* pos * neg should be neg */
							if (!sign(ir)) {
								return ERR_OVERFLOW;
							}
						} else if (!sign((int)i1) && !sign((int)i2)) {
							/* pos * pos should be pos */
							if (sign(ir)) {
								return ERR_OVERFLOW;
							}
						}
	
						/* division check */
						if ((int)ir / (int)i1 != (int)i2) {
							return ERR_OVERFLOW;
						} else if ((int)ir / (int)i2 != (int)i1) {
							return ERR_OVERFLOW;
						}
					}
					break;
			}

			break;
		case UNSIGNED: case POINTER:
			switch(opcode) {
				case NEG: 
					return ERR_INVALID_TYPE;
				case BCOM:
					u1 = parse_ulong(operand1);
					break;
				case ADD: case BAND: case BOR: case BXOR: case DIV: case LSH: 
				case MOD: case MUL: case RSH: case SUB:
					u1 = parse_ulong(operand1);
					u2 = parse_ulong(operand2);
					break;
			}

			switch(opcode) {
				case NEG: 		ur = -u1;				break;
				case BCOM:		ur = ~u1;				break;
				case ADD: 		ur = u1 + u2;		break;
				case BAND: 		ur = u1 & u2;		break;
				case BOR: 		ur = u1 | u2;		break;
				case BXOR: 		ur = u1 ^ u2;		break;
				case DIV: 		ur = u1 / u2;		break;
				case LSH: 		ur = u1 << u2;	break;
				case MOD: 		ur = u1 % u2;		break;
				case MUL: 		ur = u1 * u2;		break;
				case RSH: 		ur = u1 >> u2;	break;
				case SUB:			ur = u1 - u2;		break;
				default:			return ERR_INVALID_OPCODE;
			}
			
			*result = create_unsigned(ur, operand1->size);

			switch(opcode) {
				case ADD:
					if ((unsigned)ur < (unsigned)u1 || (unsigned)ur < (unsigned)u2) {
						return ERR_OVERFLOW;
					}
					break;
				case SUB:
					if ((unsigned)u2 > (unsigned)u1) {
						return ERR_OVERFLOW;
					}
					break;
				case MUL:
					if ((unsigned)u1 > 0 && (unsigned)u2 > 0) {
						/* division check */
						if (((unsigned int)ur) / ((unsigned int)u1) != ((unsigned int)u2)) {
							return ERR_OVERFLOW;
						} else if (((unsigned int)ur) / ((unsigned int)u2) != ((unsigned int)u1)) {
							return ERR_OVERFLOW;
						}
					}
					break;
			}

			break;
		default:
			return ERR_INVALID_TYPE;
	}

	return ERR_NOERROR;
}

/*
 * execute convert instruction
 */
int convert(Value* in, Value* out) {
	int i;
	
	switch(in->type) {
		case INTEGER:
			switch(out->type) {
				case INTEGER: case UNSIGNED:
					for (i = 0; i < out->size; i++) {
						if (i < in->size) {
							out->data[out->size-i-1] = in->data[in->size-i-1];
						} else {
							out->data[out->size-i-1] = SIGN(in->data) ? 0x00 : 0xFF;
						}
					}
					break;
/*					
				case UNSIGNED:
					if (in->size == out->size) {
						memcpy(out->data, in->data, out->size);
					} else {
						return ERR_INVALID_TYPE;
					}
					break;
*/					
				default:
					return ERR_INVALID_TYPE;
			}
			break;
		case UNSIGNED:
			switch(out->type) {
/*				
				case INTEGER: 
					if (in->size == out->size) {
						memcpy(out->data, in->data, out->size);
					} else {
						return ERR_INVALID_TYPE;
					}
					break;
*/					
				case UNSIGNED: case INTEGER:
					for (i = 0; i < out->size; i++) {
						if (i < in->size) {
							out->data[out->size-i-1] = in->data[in->size-i-1];
						} else {
							out->data[out->size-i-1] = 0x00;
						}
					}
					break;
				case POINTER:
					if (in->size != out->size) {
						return ERR_INVALID_TYPE;
					} else {
						memcpy(out->data, in->data, out->size);
					}
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			break;
		case POINTER:
			switch(out->type) {
				case UNSIGNED:
					if (in->size != out->size) {
						return ERR_INVALID_TYPE;
					} else {
						memcpy(out->data, in->data, out->size);
					}
					break;
				default:
					return ERR_INVALID_TYPE;
			}
			break;
		default:
			return ERR_INVALID_TYPE;
	}

	if (in->size > out->size) {
		/* downscale, check for overflow */
		if (in->type == INTEGER) {
			if (out->type == INTEGER) {
				/* int -> int */
				if (SIGN(in->data) != SIGN(out->data)) return ERR_OVERFLOW;
				for (i = 0; i < in->size - out->size; i++) {
					if (in->data[i] != (SIGN(in->data)?0x00:0xFF)) return ERR_OVERFLOW;
				}
			} else {
				/* int -> unsigned */
				for (i = 0; i < in->size - out->size; i++) {
					if (in->data[i] != 0x00) return ERR_OVERFLOW;
				}
			} 
		} else {
			if (out->type == INTEGER) {
				/* unsigned -> int */
				for (i = 0; i < in->size - out->size; i++) {
					if (in->data[i] != 0x00) return ERR_OVERFLOW;
				}
			} else {
				/* unsigned -> unsigned */
				for (i = 0; i < in->size - out->size; i++) {
					if (in->data[i] != 0x00) return ERR_OVERFLOW;
				}
			}
		}

	}
	return ERR_NOERROR;
}


/*
 * create a signed integer value with specified value and size
 */
Value* create_integer(long long i, int size) {
	int j;
	Value* val;
	val = (Value*)malloc(sizeof(Value));
	if (!val) return 0;
	val->size = size;
	val->type = INTEGER;
	val->data = (unsigned char*)malloc(val->size);

	for (j = val->size-1; j>= 0; j--) {
		val->data[j] = i & 0xFF;
		i = i >> 8;
	}
	
	return val;
}

/*
 * create an unsigned integer value with specified value and size
 */
Value* create_unsigned(unsigned long long u, int size) {
	int i;
	Value* val;
	val = (Value*)malloc(sizeof(Value));
	if (!val) return 0;
	val->size = size; 
	val->type = UNSIGNED;
	val->data = (unsigned char*)malloc(val->size);

	for (i = val->size-1; i>= 0; i--) {
		val->data[i] = u & 0xFF;
		u = u >> 8;
	}
	
	return val;
}

long long parse_signed(Value* value) {
	unsigned long long ret, mask;
	int i;
	unsigned char* ptr;
	ret = mask = 0;
	ptr = value->data;
	
	/* sign extend */
	for (i = (value->size<<3); i < sizeof(unsigned long)<<3; i++) {
		mask += (*(value->data)) & 0x80;
		mask = mask << 1;
	}
	
	/* read */
	for (i = 0; i < value->size; i++) {
		ret = ret << 8;
		ret += *(ptr++);
	}
	return ret | mask;
}

long long unsigned parse_unsigned(Value* value) {
	unsigned long long ret;
	int i;
	unsigned char* ptr;
	ret = 0;
	ptr = value->data;
	
	/* read */
	for (i = 0; i < value->size; i++) {
		ret = ret << 8;
		ret += *(ptr++);
	}
	return ret;
}

Value* create_void() {
	Value* val;
	val = (Value*)malloc(sizeof(Value));
	val->size = 0;
	val->type = VOID;
	val->data = 0;
	return val;
}

void destroy_value(Value* value) {
	if(value->data) free(value->data);
	free(value);
}

Value* create_char(char c) {	return create_integer(c, 1); }
Value* create_short(long i) {	return create_integer(i, SHRT_SIZE); }
Value* create_int(long i) {	return create_integer(i, INT_SIZE); }
Value* create_long(long i) {	return create_integer(i, LNG_SIZE); }
Value* create_uchar(unsigned char c) {	return create_unsigned(c, 1); }
Value* create_ushort(unsigned long i) {	return create_unsigned(i, SHRT_SIZE); }
Value* create_uint(unsigned long i) {	return create_unsigned(i, INT_SIZE); }
Value* create_ulong(unsigned long i) {	return create_unsigned(i, LNG_SIZE); }
Value* create_ptr(unsigned long i) {	return create_unsigned(i, PTR_SIZE); }

int parse_int(Value* value) { return (int)parse_signed(value); }
long parse_long(Value* value) { return (long)parse_signed(value); }
char parse_char(Value* value) { return (char)parse_signed(value); }
short parse_short(Value* value) { return (short)parse_signed(value); }
unsigned int parse_uint(Value* value) { return (unsigned int)parse_unsigned(value); }
unsigned long parse_ulong(Value* value) { return (unsigned long)parse_unsigned(value); }
unsigned char parse_uchar(Value* value) { return (unsigned char)parse_unsigned(value); }
unsigned short parse_ushort(Value* value) { return (unsigned short)parse_unsigned(value); }
unsigned long parse_ptr(Value* value) {	return (unsigned long)parse_unsigned(value); }


/*
 * dump interpreter state to stream
 */
int dumpstate(Interpreter* interpreter, FILE* file) {
	int opcode, type, size;

#ifdef ALLOW_EXEC_OUTSIDE_CODEMEM
	if (interpreter->memory_size < interpreter->pc+2 || interpreter->pc < 0) {
#else
	if (interpreter->program_size < interpreter->pc+2 || interpreter->pc < 0) {
#endif
		fprintf(file, "Invalid program counter: %04X\n", interpreter->pc);
		return ERR_PC_ERROR;
	} else {
		decode(&interpreter->memory[interpreter->pc], &opcode, &type, &size);
	}
	
	fprintf(file, "0x%04X %02X%02X: opcode: %04X (%s) type: %01X (%s) size: %-3d\n", interpreter->pc, interpreter->memory[interpreter->pc], interpreter->memory[interpreter->pc+1], opcode, opcode_name(opcode), type, type_name(type), size);
#ifdef ALLOW_EXEC_OUTSIDE_CODEMEM
	if (interpreter->memory_size < interpreter->pc+6 || interpreter->pc < 0) {
#else
	if (interpreter->program_size < interpreter->pc+6 || interpreter->pc < 0) {
#endif
		fprintf(file, "\t(Parameter: %02X %02X %02X %02X)\n", interpreter->memory[interpreter->pc+2], interpreter->memory[interpreter->pc+3], interpreter->memory[interpreter->pc+4], interpreter->memory[interpreter->pc+5]);
	}
	
	return ERR_NOERROR;
}

/* 
 * dump stack to stream 
 */
void dumpstack(Interpreter* interpreter, FILE* file, int max_rollback) {
	int i,j;

	if (interpreter->sp < 0 || interpreter->sp > interpreter->memory_size) {
		printf("Invalid stackpointer, can't dump stack (0x%X)\r\n", interpreter->sp);
		return;
	}

	/* print registers */
	fprintf(file, "Stack: (FP: 0x%04X, LP: 0x%04X, SP: 0x%04X, AP: 0x%04X)\n", interpreter->fp, interpreter->lp, interpreter->sp, interpreter->ap);
	/* print stack */ 

	for (i = (max(interpreter->stack_offset, interpreter->sp-max_rollback)+3)&(-4); i < interpreter->sp; i+=4) {
		fprintf(file, "0x%04X:", i);
		for (j = 0; j < 4; j++) {
			fprintf(file, i+j<=interpreter->sp ? " %02X" : " **",  interpreter->memory[i+j]);
		}
		if (interpreter->fp > i && interpreter->fp <= i+4) {
			fprintf(file, " (fp)");
		}
		if (interpreter->sp > i && interpreter->sp <= i+4) {
			fprintf(file, " (sp)");
		}
		if (interpreter->lp > i && interpreter->lp <= i+4) {
			fprintf(file, " (lp)");
		}
		if (interpreter->ap > i && interpreter->ap <= i+4) {
			fprintf(file, " (ap)");
		}
		fprintf(file, "\n");
	}

	
	fprintf(file, "\n");	
}


