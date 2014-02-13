#include "instructions.h"
#include <string.h>

	/*	16 bit:
	 * oooooootttssss**
	 *	o: opcode
	 *	t: type
	 *	s: size (log2)
	 *	*: unused 
	 */
/* build the instruction (opcode part) from the opcode, size and type */
int build_opcode(int opcode, int size, int type, unsigned char* data) {
	int type_size;
	assert(OPCSIZE == 2);

	/*
	data[0] = (opcode & 0x7F) << 1;
	data[0] += (type & 0x07) >> 2;
	data[1] = (type & 0x07) << 6; 
	data[1] += (log2(size) & 0x0F) << 2;
	*/

	switch(type) {
		case POINTER:
			if (size == PTR_SIZE) {
				type_size = TYPE_POINTER;
			} else {
				/* error */
				type_size = -1;
			}
			break;
		case INTEGER:
			if (size == 1) {
				type_size = TYPE_CHAR+1;
			} else if (size == SHRT_SIZE) {
				type_size = TYPE_SHORT+1;
			} else if (size == INT_SIZE) {
				type_size = TYPE_INT+1;
			} else if (size == LNG_SIZE) {
				type_size = TYPE_LONG+1;
			} else {
				/* error */
				type_size = -1;
			}
			break;
		case UNSIGNED:
			if (size == 1) {
				type_size = TYPE_CHAR;
			} else if (size == SHRT_SIZE) {
				type_size = TYPE_SHORT;
			} else if (size == INT_SIZE) {
				type_size = TYPE_INT;
			} else if (size == LNG_SIZE) {
				type_size = TYPE_LONG;
			} else {
				/* error */
				type_size = -1;
			}
			break;
		case VOID:
			type_size = TYPE_VOID;
			break;
		default:
			type_size = -1;
			break;
	}	
	
	if (type_size == -1) {
		return 0;
	} else {
		data[0] = (opcode & 0xFF00) >> 8;
		data[1] = (opcode & 0xF0) + type_size;
		return OPCSIZE;
	}
}

/* build the parameter (bigendian) */
int build_parameter(unsigned long long param, unsigned char* data, int paramsize) {
	int i;
	
	for (i = paramsize-1; i >= 0; i--) {
		data[i] = param & 0xFF;
		param = param >> 8;
	}

	return paramsize;
}

/* decode an instruction */
void decode(unsigned char* instruction, int* opcode, int* type, int* size) {
	
	*opcode = (instruction[0] << 8) + (instruction[1] & 0xF0);
	
	switch(instruction[1] & 0x0E) {
		case TYPE_VOID:
			*type = VOID;
			*size = 0;
			break;
		case TYPE_CHAR:
			if (instruction[1] & 0x01) {
				*type = INTEGER;
			} else {
				*type = UNSIGNED;
			}
			*size = 1;
			break;
		case TYPE_SHORT:
			if (instruction[1] & 0x01) {
				*type = INTEGER;
			} else {
				*type = UNSIGNED;
			}
			*size = SHRT_SIZE;
			break;
		case TYPE_INT:
			if (instruction[1] & 0x01) {
				*type = INTEGER;
			} else {
				*type = UNSIGNED;
			}
			*size = INT_SIZE;
			break;
		case TYPE_LONG:
			if (instruction[1] & 0x01) {
				*type = INTEGER;
			} else {
				*type = UNSIGNED;
			}
			*size = LNG_SIZE;
			break;
		case TYPE_LONG2:
			if (instruction[1] & 0x01) {
				*type = INTEGER;
			} else {
				*type = UNSIGNED;
			}
			*size = LNG_SIZE;
			break;
		case TYPE_POINTER:
			*type = POINTER;
			*size = PTR_SIZE;
			break;
	}
	
	/*
	*opcode = *instruction>>1;
	*type = ((*instruction & 0x01) << 2) + (*(instruction+1)>>6); 
	*size = 1<<((*(instruction+1)>>2) & 0x0F);
	*/
}

/* get the opcode in string format (debug) */
char* opcode_name(int opcode) {
	switch (opcode) {
		case ADDRF: 		return "ADDRF";
		case ADDRG: 		return "ADDRG";
		case ADDRL: 		return "ADDRL";
		case ADDRA: 		return "ADDRA";
		case CNST: 			return "CNST"; 
		case BCOM: 			return "BCOM";
		case CVF: 			return "CVF";
		case CVI: 			return "CVI";
		case CVP: 			return "CVP";
		case CVU: 			return "CVU";
		case INDIR: 		return "INDIR";
		case NEG: 			return "NEG";
		case ADD: 			return "ADD";
		case BAND: 			return "BAND";
		case BOR: 			return "BOR";
		case BXOR: 			return "BXOR";
		case DIV: 			return "DIV:";
		case LSH: 			return "LSH";
		case MOD: 			return "MOD";
		case MUL: 			return "MUL";
		case RSH: 			return "RSH";
		case SUB: 			return "SUB";
		case ASGN: 			return "ASGN";
		case EQ: 				return "EQ";
		case GE: 				return "GE";
		case GT: 				return "GT";
		case LE: 				return "LE";
		case LT: 				return "LT";
		case NE: 				return "NE";
		case ARG: 			return "ARG";
		case CALL: 			return "CALL";
		case RET: 			return "RET";
		case JUMP: 			return "JUMP";
/*
		case LABEL: 		return "LABEL";
*/		
		case VARSTACK: 	return "VARSTACK";
		case ARGSTACK: 	return "ARGSTACK";
/*			
		case SAVESTATE:	return "SAVESTATE";
		case NEWSTACK: 	return "NEWSTACK";
*/		
		case DISCARD:		return "DISCARD";

		case SYSCALL: 	return "SYSCALL";
		case HALT:			return "HALT";

/*			
		case LOADAP:		return "LOADAP";
		case LOADFP:		return "LOADFP";
		case LOADSP:		return "LOADSP";
		case LOADLP:		return "LOADLP";
		case LOADVR:		return "LOADVR";
		case LOADEL:		return "LOADEL";
		case SAVEAP:		return "SAVEAP";
		case SAVEFP:		return "SAVEFP";
		case SAVESP:		return "SAVESP";
		case SAVELP:		return "SAVELP";
		case SAVEVR:		return "SAVEVR";
		case SAVEEL:		return "SAVEEL";
*/

		case MOVESTACK:	return "MOVESTACK";

		case TRAP:			return "TRAP";
			
		default: 				
			if ((opcode & 0xFF00) == CV) {
				return "CV*";
			} else {
				return "UNKNOWN";
			}
	}
}

/* get the opcode in id format (debug) */
int opcode_id(int opcode) {
	switch (opcode) {
		case ADDRF: 		return 0x01;
		case ADDRG: 		return 0x02;
		case ADDRL: 		return 0x03;
		case CNST: 			return 0x04;
		case BCOM: 			return 0x05;
		case CVF: 			return 0x06;
		case CVI: 			return 0x07;
		case CVP: 			return 0x08;
		case CVU: 			return 0x09;
		case INDIR: 		return 0x0A;
		case NEG: 			return 0x0B;
		case ADD: 			return 0x0C;
		case BAND: 			return 0x0D;
		case BOR: 			return 0x0E;
		case BXOR: 			return 0x0F;
		case DIV: 			return 0x10;
		case LSH: 			return 0x11;
		case MOD: 			return 0x12;
		case MUL: 			return 0x13;
		case RSH: 			return 0x14;
		case SUB: 			return 0x15;
		case ASGN: 			return 0x16;
		case EQ: 				return 0x17;
		case GE: 				return 0x18;
		case GT: 				return 0x19;
		case LE: 				return 0x1A;
		case LT: 				return 0x1B;
		case NE: 				return 0x1C;
		case ARG: 			return 0x1D;
		case CALL: 			return 0x1E;
		case RET: 			return 0x1F;
		case JUMP: 			return 0x20;
/*
		case LABEL: 		return 0x21;
*/		
		case VARSTACK: 	return 0x22;
		case ARGSTACK: 	return 0x23;
/*			
		case SAVESTATE:	return 0x24;
		case NEWSTACK: 	return 0x25;
*/		
		case DISCARD:		return 0x26;

		case SYSCALL: 	return 0x27;
		case HALT:			return 0x28;
			
/*
		case LOADAP:		return 0x29;
		case LOADFP:		return 0x2A;
		case LOADSP:		return 0x2B;
		case LOADLP:		return 0x2C;
		case SAVEAP:		return 0x2D;
		case SAVEFP:		return 0x2E;
		case SAVESP:		return 0x2F;
		case SAVELP:		return 0x30;
*/
			
/*
		case SAVEVR:		return 0x32;
		case SAVEEL:		return 0x33;
*/
/*
		case LOADVR:		return 0x34;
		case LOADEL:		return 0x35;
*/

		case TRAP:			return 0x40;
		case ADDRA: 		return 0x41;
			
		default: 				
			if ((opcode & 0xFF00) == CV) {
				return 0x31;
			} else {
				return 0x00;
			}
	}
}

/* get the opcode in id format (debug) */
int opcode_from_id(int opcode) {
	switch (opcode) {
		case 0x01:				return ADDRF; 		
		case 0x02:				return ADDRG; 		
		case 0x03:				return ADDRL; 		
		case 0x04:				return CNST; 			
		case 0x05:				return BCOM; 			
		case 0x06:				return CVF;
		case 0x07:				return CVI;
		case 0x08:				return CVP;
		case 0x09:				return CVU;
		case 0x0A:				return INDIR;
		case 0x0B:				return NEG;
		case 0x0C:				return ADD;
		case 0x0D:				return BAND;
		case 0x0E:				return BOR;
		case 0x0F:				return BXOR;
		case 0x10:				return DIV;
		case 0x11:				return LSH;
		case 0x12:				return MOD;
		case 0x13:				return MUL;
		case 0x14:				return RSH;
		case 0x15:				return SUB;
		case 0x16:				return ASGN;
		case 0x17:				return EQ;
		case 0x18:				return GE;
		case 0x19:				return GT;
		case 0x1A:				return LE;
		case 0x1B:				return LT;
		case 0x1C:				return NE;
		case 0x1D:				return ARG;
		case 0x1E:				return CALL;
		case 0x1F:				return RET;
		case 0x20:				return JUMP;
/*
		case 0x21:				return LABEL;
*/		
		case 0x22:				return VARSTACK;
		case 0x23:				return ARGSTACK;
/*			
		case 0x24:				return SAVESTATE;
		case 0x25:				return NEWSTACK;
*/		
		case 0x26:				return DISCARD;

		case 0x27:				return SYSCALL;
		case 0x28:				return HALT;

/*
		case 0x29:				return LOADAP;
		case 0x2A:				return LOADFP;
		case 0x2B:				return LOADSP;
		case 0x2C:				return LOADLP;
		case 0x2D:				return SAVEAP;
		case 0x2E:				return SAVEFP;
		case 0x2F:				return SAVESP;
		case 0x30:				return SAVELP;
*/
		case 0x31:				return CV;

/*
		case 0x32:				return SAVEVR;
		case 0x33:				return SAVEEL;
		case 0x34:				return LOADVR;
		case 0x35:				return LOADEL;
*/

		case 0x40:				return TRAP;
		case 0x41:				return ADDRA;


		default: 					return 0x00;
	}
}


/* get the type name in string format (debug) */
char* type_name(int type) {
	switch (type) {
		case POINTER: 	return "POINTER";
		case INTEGER: 	return "INTEGER";
		case FLOAT: 		return "FLOAT";
		case UNSIGNED: 	return "UNSIGNED";
		case VOID: 			return "VOID";
		case STRUCT: 		return "STRUCT";
			
		default: 				return "UNKNOWN";
	}
}

/* get the type name in char format (debug) */
char* type_shortname(int type) {
	switch (type) {
		case POINTER: 	return "P";
		case INTEGER: 	return "I";
		case FLOAT: 		return "F";
		case UNSIGNED: 	return "U";
		case VOID: 			return "V";
		case STRUCT: 		return "B";
		default: 				return "?";
	}
}

/* get the segment name in string format (debug) */
char* segment_name(int segment) {
	switch(segment) {
		case UNKNOWN: return "UNKNOWN";
		case CODE: return "CODE";
		case DATA: return "DATA";
		case LIT: return "LIT";
		case BSS: return "BSS";
		default: return "?";
	}
}

/* parse a text instruction */
BOOL parse_instruction(char* instruction, int* opcode, int* size, int* type) {
	int i;
	
	/* read instruction size from right to left */
	for (i = strlen(instruction)-1; i >= 0; i--) {
		if (instruction[i] < '0' || instruction[i] > '9') break;
	}
	
	/* parse size */
	*size = atoi(&instruction[i+1]);
	
	/* parse type */
	switch(instruction[i]) {
		case 'P':
			*type = POINTER;
			break;
		case 'I':
			*type = INTEGER;
			break;
		case 'F':
			*type = FLOAT;
			break;
		case 'U':
			*type = UNSIGNED;
			break;
		case 'V':
			*type = VOID;
			break;
		case 'B':
			*type = STRUCT;
			break;
		default:
			return FALSE;
	}
	
	/* check opcode */
	if (i == 2) {
		if (strncmp(instruction, "EQ", i) == 0) *opcode = EQ;
		else if (strncmp(instruction, "GE", i) == 0) *opcode = GE;
		else if (strncmp(instruction, "GT", i) == 0) *opcode = GT;
		else if (strncmp(instruction, "LE", i) == 0) *opcode = LE;
		else if (strncmp(instruction, "LT", i) == 0) *opcode = LT;
		else if (strncmp(instruction, "NE", i) == 0) *opcode = NE;
		else return FALSE;
	} else if (i == 3) {
		if (strncmp(instruction, "CVF", i) == 0) *opcode = CVF;
		else if (strncmp(instruction, "CVI", i) == 0) *opcode = CVI;
		else if (strncmp(instruction, "CVP", i) == 0) *opcode = CVP;
		else if (strncmp(instruction, "CVU", i) == 0) *opcode = CVU;
		else if (strncmp(instruction, "NEG", i) == 0) *opcode = NEG;
		else if (strncmp(instruction, "ADD", i) == 0) *opcode = ADD;
		else if (strncmp(instruction, "BOR", i) == 0) *opcode = BOR;
		else if (strncmp(instruction, "DIV", i) == 0) *opcode = DIV;
		else if (strncmp(instruction, "LSH", i) == 0) *opcode = LSH;
		else if (strncmp(instruction, "MOD", i) == 0) *opcode = MOD;
		else if (strncmp(instruction, "MUL", i) == 0) *opcode = MUL;
		else if (strncmp(instruction, "RSH", i) == 0) *opcode = RSH; 
		else if (strncmp(instruction, "SUB", i) == 0) *opcode = SUB;
		else if (strncmp(instruction, "ARG", i) == 0) *opcode = ARG;
		else if (strncmp(instruction, "RET", i) == 0) *opcode = RET;
		else return FALSE;
	} else if (i == 4) {
		if (strncmp(instruction, "CNST", i) == 0) *opcode = CNST;
		else if (strncmp(instruction, "BCOM", i) == 0) *opcode = BCOM;
		else if (strncmp(instruction, "BAND", i) == 0) *opcode = BAND;
		else if (strncmp(instruction, "BXOR", i) == 0) *opcode = BXOR;
		else if (strncmp(instruction, "ASGN", i) == 0) *opcode = ASGN;
		else if (strncmp(instruction, "CALL", i) == 0) *opcode = CALL;
		else if (strncmp(instruction, "JUMP", i) == 0) *opcode = JUMP;
		else if (strncmp(instruction, "HALT", i) == 0) *opcode = HALT; 
		else if (strncmp(instruction, "TRAP", i) == 0) *opcode = TRAP; 
		else return FALSE;
	} else if (i == 5) {
		if (strncmp(instruction, "ADDRF", i) == 0) *opcode = ADDRF;
		else if (strncmp(instruction, "ADDRG", i) == 0) *opcode = ADDRG;
		else if (strncmp(instruction, "ADDRL", i) == 0) *opcode = ADDRL;
		else if (strncmp(instruction, "ADDRA", i) == 0) *opcode = ADDRA;
		else if (strncmp(instruction, "INDIR", i) == 0) *opcode = INDIR;
/*		else if (strncmp(instruction, "LABEL", i) == 0) *opcode = LABEL; */
		else return FALSE;
	} else if (i == 6) {
		if (strncmp(instruction, "SAVEAP", i) == 0) *opcode = SAVEAP;
		else if (strncmp(instruction, "SAVEFP", i) == 0) *opcode = SAVEFP;
		else if (strncmp(instruction, "SAVELP", i) == 0) *opcode = SAVELP;
		else if (strncmp(instruction, "SAVESP", i) == 0) *opcode = SAVESP;
		else if (strncmp(instruction, "SAVEVR", i) == 0) *opcode = SAVEVR;
		else if (strncmp(instruction, "SAVEEL", i) == 0) *opcode = SAVEEL;
/*
		else if (strncmp(instruction, "LOADAP", i) == 0) *opcode = LOADAP;
		else if (strncmp(instruction, "LOADFP", i) == 0) *opcode = LOADFP;
		else if (strncmp(instruction, "LOADLP", i) == 0) *opcode = LOADLP;
		else if (strncmp(instruction, "LOADSP", i) == 0) *opcode = LOADSP;
		else if (strncmp(instruction, "LOADVR", i) == 0) *opcode = LOADVR;
		else if (strncmp(instruction, "LOADEL", i) == 0) *opcode = LOADEL;
*/
		else return FALSE;
	} else if (i == 7) {
		if (strncmp(instruction, "SYSCALL", i) == 0) *opcode = SYSCALL;
		else if (strncmp(instruction, "DISCARD", i) == 0) *opcode = DISCARD;
		else return FALSE;
	} else if (i == 8) {
		if (strncmp(instruction, "VARSTACK", i) == 0) *opcode = VARSTACK;
		else if (strncmp(instruction, "ARGSTACK", i) == 0) *opcode = ARGSTACK;
/*
		else if (strncmp(instruction, "NEWSTACK", i) == 0) *opcode = LOADSP;
*/
		else return FALSE;
	} else if (i == 9) {
		if (strncmp(instruction, "MOVESTACK", i) == 0) *opcode = MOVESTACK;
		else return FALSE;
/*		
	} else if (i == 9) {
		if (strncmp(instruction, "SAVESTATE", i) == 0) *opcode = SAVESTATE;
		else return FALSE;
*/
	} else { 
		return FALSE;
	}
	
	/* finally here without errors */
	return TRUE;
}

/* get size of the parameter of an instruction */
int param_size(int opcode, int size) {
	switch(opcode) {
		case ARGSTACK: case VARSTACK: case ARG:
			return INT_SIZE;
		case ADDRG: case EQ: case GE: case GT: case LE: case LT: case NE: 
		case ADDRF: case ADDRL: case ADDRA: 
			return PTR_SIZE;
		case CNST:
			return size;
		case BCOM: case CALL: case RET: case JUMP: case HALT: case SYSCALL:
		case NEG: case ADD: case BAND: case BOR: case BXOR:
		case DIV: case LSH: case MOD: case MUL: case RSH: case SUB:
		case SAVELP: case SAVEFP: case SAVEAP: case SAVESP: case SAVEVR: case SAVEEL:
/*
		case LOADLP: case LOADFP: case LOADAP: case LOADSP: case LOADVR: case LOADEL:
*/
		case DISCARD: case ASGN: case INDIR: case CVI: case CVP: case CVU: case CVF: 
		case TRAP:
		default:
			return 0;
	}
}
