#include "debug_decoder.h"
#include <string.h>
#include <stdio.h>

int action_text(int *state, int *substate, int action, unsigned char* data, char* message) {
	int instruction;
	message[0] = 0;

	if (*substate == 0) {
		// substate = 0 => this should be an INSTRUCTION FETCH
		if (action == ACTION_READ) {
			instruction = (data[2]<<8) + (data[3] & 0xF0);
			if (opcode_id(instruction)) {
				*state = data[2];
				*substate = 1;
				sprintf(message, "INSTRUCTION FETCH [%s]", opcode_name(instruction));

				return 1;
			} else {
				*state = 0;
				*substate = 0;
				return 0;
			}
		} else {
			*state = 0;
			*substate = 0;
			strcat(message, "BOOTING?");
			return 0;
		}
	} else {
		switch(*state) {
			case 0x01: // COMPUTE ADDRESS (ADDRG, ADDRF, ADDRL, CNST)
				// PARAM -> PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "FETCH PARAMETER [ADDRESS]");
						return 1;
					case 2: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [ADDRESS]");
						return 1;
					default: break;
				}
				break;
			case 0x02: // SAVE REGISTER (SAVEAP, SAVEFP, SAVELP, SAVESP)
				// PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [REGISTER]");
						return 1;
					default: break;
				}
				break;
			case 0x03: // ARITHMETIC (ADD, SUB, MUL, ...)
				// POP -> POP -> PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [OPERAND1]");
						return 1;
					case 2: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [OPERAND2]");
						return 1;
					case 3: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [RESULT]");
						return 1;
					default: break;
				}
				break;
			case 0x04: // ONE OPERAND ARITHMETIC (COMP, NEG)
				// POP -> PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [OPERAND]");
						return 1;
					case 2: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [RESULT]");
						return 1;
					default: break;
				}
				break;
			case 0x05: // FUNCTION CALL (CALL, SYSCALL)
				// POP -> PUSH -> PUSH -> PUSH -> PUSH -> PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [ADDRESS]");
						return 1;
					case 2: 
						if (action != ACTION_WRITE) break;
						*substate = *substate + 1;
						strcat(message, "PUSH [RETURN ADDRESS]");
						return 1;
					case 3: 
						if (action != ACTION_WRITE) break;
						*substate = *substate + 1;
						strcat(message, "PUSH [EXECUTION LEVEL]");
						return 1;
					case 4: 
						if (action != ACTION_WRITE) break;
						*substate = *substate + 1;
						strcat(message, "PUSH [ARGUMENT POINTER]");
						return 1;
					case 5: 
						if (action != ACTION_WRITE) break;
						*substate = *substate + 1;
						strcat(message, "PUSH [LOCAL POINTER]");
						return 1;
					case 6: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [FRAME POINTER]");
						return 1;
					default: break;
				}
				break;
			case 0x06: // JUMP
				// POP
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = 0;
						strcat(message, "POP [ADDRESS]");
						return 1;
					default: break;
				}
				break;
			case 0x07: // FUNCTION RETURN (RET)
				// READ
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [RETURN VALUE]");
						return 1;
					case 2: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [RETURN ADDRESS]");
						return 1;
					case 3: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [EXECUTION LEVEL]");
						return 1;
					case 4: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [ARGUMENT POINTER]");
						return 1;
					case 5: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [LOCAL POINTER]");
						return 1;
					case 6: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [FRAME POINTER]");
						return 1;
					case 7: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [RETURN VALUE]");
						return 1;
					default: break;
				}
				break;
			case 0x08: // FETCH (INDIR)
				// POP -> READ -> PUSH
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [ADDRESS]");
						return 1;
					case 2: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "READ [DATA]");
						return 1;
					case 3: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [DATA]");
						return 1;
					default: break;
				}
				break;
			case 0x09: // STACKFRAME (ARGSTACK)
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = 0;
						strcat(message, "FETCH PARAMETER [STACKSPACE]");
						return 1;
					default: break;
				}
				break;
			case 0x0A: // STACKFRAME (VARSTACK)
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = 0;
						strcat(message, "FETCH PARAMETER [STACKSPACE]");
						return 1;
					default: break;
				}
				break;
			case 0x0C: // COMPARE (EQ, GE, GT, LT, LE, NE)
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "FETCH PARAMETER [ADDRESS]");
						return 1;
					case 2: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [OPERAND1]");
						return 1;
					case 3: 
						if (action != ACTION_READ) break;
						*substate = 0;
						strcat(message, "POP [OPERAND2]");
						return 1;
					default: break;
				}
				break;
			case 0x0D: // ASGN
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [DATA]");
						return 1;
					case 2: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [ADDRESS]");
						return 1;
					case 3: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "WRITE [DATA]");
						return 1;
					default: break;
				}
				break;
			case 0x0E: // LOAD REGISTER (LOADAP, LOADFP, LOADLP, LOADSP)
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = 0;
						strcat(message, "POP [REGISTER]");
						return 1;
					default: break;
				}
				break;
			case 0x0F: // CV*
				switch(*substate) {
					case 1: 
						if (action != ACTION_READ) break;
						*substate = *substate + 1;
						strcat(message, "POP [DATA]");
						return 1;
					case 2: 
						if (action != ACTION_WRITE) break;
						*substate = 0;
						strcat(message, "PUSH [DATA]");
						return 1;
					default: break;
				}
			default: break;
		}

		*state = 0;
		*substate = 0;
		return 0;
	}
}
