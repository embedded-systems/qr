/*
 * Instructions: Contains the opcodes used by the assembler & linker
 *
 *	Author: Sijmen Woutersen
 *
 */

#include <assert.h>
#include "bool.h"


/* opcode list and binary representation */
#define ADDRF				0x0120
#define ADDRG				0x01C0
#define ADDRL				0x0140
#define ADDRA				0x0180
#define CNST				0x0100

/* these are not standard in LCC Bytecode, but supported by the 
		processor, they are the same as ADDRF ADDRG etc, but these 
		contain all registers (including VERSION and EXECUTION LEVEL
 */
#define ADDR_AP			0x0120
#define ADDR_LP			0x0140
#define ADDR_FP			0x0180
#define ADDR_SP			0x0110
#define ADDR_VR			0x0160
#define ADDR_EL			0x01A0

/* pseudo: these instructions are assembles using ADDR_LP opcodes
		above, these are NOT supported by the processor or simulator */
#define SAVEAP			0x0220
#define SAVEFP			0x0280
#define SAVELP			0x0240
#define SAVESP			0x0210
#define SAVEVR			0x0260
#define SAVEEL			0x02A0

#define BCOM				0x04C0

#define CV					0x0F00
/* note: the following opcodes are no longer used, instead, the from and to 
		variable types are coded within the lower 8 bits of the opcode */
#define CVF					(CV+1)
#define CVI					(CV+2)
#define CVP					(CV+3)
#define CVU					(CV+4)

#define INDIR				0x08D0
#define NEG					0x04B0
#define ADD					0x0310
#define BAND				0x0320
#define BOR					0x0330
#define BXOR				0x0340
#define DIV					0x0350
#define LSH					0x0360
#define MOD					0x0370
#define MUL					0x0380
#define RSH					0x0390
#define SUB					0x03A0
#define ASGN				0x0DE0
#define EQ					0x0C10
#define GE					0x0C30
#define GT					0x0C20
#define LE					0x0C50
#define LT					0x0C40
#define NE					0x0C80
#define ARG					0x0B80
#define CALL				0x0500
#define RET					0x07D0
#define JUMP				0x06D0
/* label is not an instruction
/*#define LABEL				0x21*/

/* new instructions created for functions */
#define VARSTACK		0x0A10
#define ARGSTACK		0x0910
/* pseudo instruction, same as VARSTACK, but parameter is a label instead of
		a constant, to move the stack beyond the bss space. The size of the bss
		space is stored in label $bss_size, MOVESTACK $bss_size will thus move
		the stack beyond the bss size */
#define MOVESTACK		0x0A00
/* SAVESTATE is no longer supported, use SAVEAP => SAVELP => SAVEFP instead */
/*#define SAVESTATE 	0x28*/
/* NEWSTACK is no longer supported, use LOADSP instead */
/*#define NEWSTACK		0x29*/

/* pseudo: (it's actually an ALU instruction which pops 2 operands and pushes
		only the first operand back (ALU opcode OP1) */
#define DISCARD			0x03D0

/* no longer supported:
	#define LOADAP			0x0E20
	#define LOADFP			0x0E80
	#define LOADLP			0x0E40
	#define LOADSP			0x0E10
	#define LOADVR			0x0E60		// FORBIDDEN!!!
	#define LOADEL			0x0EA0
*/

/* new instructions created for special functions */
/* note: syscall is NOT supported by hardware, will act like a normal call */
#define SYSCALL			0x0510

/* MARK is no longer supported */
/*#define MARK				0x78*/		/* instruction which does nothing, 
														but shows 0F0F0F0F in executable 
														file (debug, no longer supported) */
#define HALT				0x8001	/* end program */
#define TRAP				0x8000	/* trap (breakpoint) */

#define MAX_OPCODE_ID	0x50	/* highest opcode id */

/* segments */
#define UNKNOWN 0x00
#define BSS 		0x01
#define CODE		0x02
#define LIT			0x04
#define DATA		0x08
/* base segment is always relative to the program's base,
		eg, a label at location 4 in segment BASE will always be at
		location program base + 4, used to create "constant" labels */
#define BASE		0x10	

/* variable types */
#define POINTER		0x01
#define INTEGER 	0x02
#define FLOAT			0x03
#define UNSIGNED	0x04
#define VOID			0x05
#define STRUCT		0x06

#define SC_READ		0x01	/* read char from console */
#define SC_READX	0x03	/* non blocking read */
#define SC_WRITE	0x02	/* write char to console */
#define SC_RAND		0x04	/* generate random integer */
#define SC_CLOCK	0x05	/* get clock ticks executed */

/* variable sizes SHOULD BE THE SAME AS LCC'S! */
#define SHRT_SIZE 	2
#define INT_SIZE 		4
#define PTR_SIZE 		4
#define LNG_SIZE 		4


/* variable type/size encodings */
#define TYPE_VOID			0x00
#define TYPE_CHAR			0x02
#define TYPE_SHORT		0x04
#define TYPE_INT			0x06
#define TYPE_LONG			0x08
#define TYPE_LONG2		0x0A
#define TYPE_POINTER	0x0C

/* opcode size */
#define OPCSIZE		2

/* some helper functions for building instructions */
int build_opcode(int, int, int, unsigned char*);
int build_parameter(unsigned long long, unsigned char*, int);
/* converting binary instructions to readable text */
char* opcode_name(int);
char* type_name(int);
char* type_shortname(int);
char* segment_name(int);
/* instruction decoder (bin->opcode/type/size) */
void decode(unsigned char*, int*, int*, int*);
/* instruction parser (text->opcode/type/size) */
BOOL parse_instruction(char*, int*, int*, int*);

/* convert opcodes to unique ints and visa versa */
int opcode_id(int);
int opcode_from_id(int);

/* parameter size information */
int param_size(int, int);
