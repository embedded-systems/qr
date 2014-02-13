#include "c.h"
#define I(f) b_##f

/* sizeof(int) */
#define XBC_INT_SIZE 4

/* default:
 *		sizeof(long) = 2*sizeof(int)
 *		sizeof(long long) = sizeof(long)
 *		sizeof(void*) = sizeof(int)
 *		sizeof(short) = sizeof(int)/2 		if sizeof(int) > 1 
 *										sizeof(int) 			otherwise
 * note: sizeof(char) must always be one!
 */
/* #define XBC_LNG_SIZE (XBC_INT_SIZE << 1) */
#define XBC_LNG_SIZE XBC_INT_SIZE
#define XBC_PTR_SIZE XBC_INT_SIZE
#define XBC_LNGLNG_SIZE XBC_LNG_SIZE
#define XBC_SHORT_SIZE (XBC_INT_SIZE>1?XBC_INT_SIZE>>1:XBC_INT_SIZE)

static char rcsid[] = "$Id: xbytecode.c,v 0.2.1 2006/09/06 23:48:41 drh Exp $";

static char *type_names[] = {"type0", "float", "type2", "type3", 
	"type4", "integer", "unsigned", "pointer", "void", "struct", "union",
	"function","array","enum","typeE","const"};
static char *suffixes[] = {"?", "F", "D", "C", "S", "I", "U", "P", "V", "B", "?","?","?","?","?","?"};

char* current_func;

/* x32 version id */
int vid = 0;
/* this counter keeps track of the argument-to-child-functions position; lcc
			used to generate plain ARG instructions:

			...
			<generate argument>
			ARG
			<generate second argument>
			ARG
			...

			which is very hard to execute, this counter keeps track of the size of
			all previous arguments written such that code like the following can be
			generated:

			...
			<generate argument>
			ARG 0
			<generate second argument>
			ARG 0+sizeof(previous argument)
			...

			which is much easier to execute (the target location is now known).
*/
int arg_cntr = 0;

/* prototypes */
static void I(stabline)(Coordinate *cp);
/*
static void print_global_debug_info(Symbol);
static void print_function_debug_info(Symbol, Symbol[]);
static void print_type_debug_info(Type);
*/
static void print_local_debug_info(Symbol);
static void print_type_name(Type t);
static void print_type_info(Type t);

static void I(segment)(int n) {
	static int cseg;

	if (cseg != n)
		switch (cseg = n) {
		case CODE: print("code\n"); return;
		case DATA: print("data\n"); return;
		case BSS:  print("bss\n");  return;
		case LIT:  print("lit\n");  return;
		default: assert(0);
		}
}

static void I(address)(Symbol q, Symbol p, long n) {
	q->x.name = stringf("%s%s%D", p->x.name, n > 0 ? "+" : "", n);
}

static void I(defaddress)(Symbol p) {
	print("address %s\n", p->x.name);
}  

static void I(defconst)(int suffix, int size, Value v) {
	switch (suffix) {
	case I:
		if (size > sizeof (int))
			print("byte %d %D\n", size, v.i);
		else
			print("byte %d %d\n", size, v.i);
		return;
	case U:
		if (size > sizeof (unsigned))
			print("byte %d %U\n", size, v.u);
		else
			print("byte %d %u\n", size, v.u);
		return;
	case P: print("byte %d %U\n", size, (unsigned long)v.p); return;
	case F:
		if (size == 4) {
			float f = v.d;
			print("byte 4 %d\n", *(unsigned *)&f);
		} else {
			double d = v.d;
			unsigned *p = (unsigned *)&d;
			print("byte 4 %d\n", p[swap]);
			print("byte 4 %d\n", p[1 - swap]);
		}
		return;
	}
	assert(0);
}

static void I(defstring)(int len, char *str) {
	char *s;

	for (s = str; s < str + len; s++)
		print("byte 1 %d\n", (*s)&0xFF);
}

static void I(defsymbol)(Symbol p) {
	if (p->scope == CONSTANTS)
		switch (optype(ttob(p->type))) {
		case I: p->x.name = stringf("%D", p->u.c.v.i); break;
		case U: p->x.name = stringf("%U", p->u.c.v.u); break;
		case P: p->x.name = stringf("%U", p->u.c.v.p); break;
		default: assert(0);
		}
	else if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf("$%d", genlabel(1));
	else if (p->scope == LABELS || p->generated)
		p->x.name = stringf("$%s", p->name);
	else
		p->x.name = p->name;
}

static void dumptree(Node p) {
	int i;
	
	switch (specific(p->op)) {
	case ASGN+B:
		assert(p->kids[0]);
		assert(p->kids[1]);
		assert(p->syms[0]);
		
		/* ASGNB must be preceeded by INDIRB */
		assert(specific(p->kids[1]->op) == INDIR+B);

		/* Some debug info */
		if (glevel>5) print("# BEGINNING OF STRUCTURE ASSIGNMENT (%d bytes)\n", p->syms[0]->u.c.v.u);
		
		/* TODO: structs are always aligned by 4, not neccessary! */
		/* TODO: structure field addresses are computed using ADD, 
							they can in some cases be computerd at compile time */
		/* lng2size bit aligned values */
		for (i = 0; i < p->syms[0]->u.c.v.u; i += XBC_LNGLNG_SIZE) {
			if (glevel>5) print("# BYTE %d->%d\n", i, i + XBC_LNGLNG_SIZE-1);
			if (glevel>5) print("# TO:\n");
			dumptree(p->kids[0]);
			if (i) {
				print("CNSTP%d %d\n", XBC_PTR_SIZE, i);
				print("ADDP%d\n", XBC_PTR_SIZE);
			}
			if (glevel>5) print("# FROM:\n");
			/* address of indir */
			dumptree(p->kids[1]->kids[0]);
			if (i) {
				print("CNSTP%d %d\n", XBC_PTR_SIZE, i);
				print("ADDP%d\n", XBC_PTR_SIZE);
			}
			/* copy as unsigned */
			if (glevel>5) print("# COPY\n");
			/* fetch */
			print("INDIRU%d\n", XBC_LNGLNG_SIZE);
			/* assign */
			print("ASGNU%d\n", XBC_LNGLNG_SIZE);
		}
		
		/* 8 bit aligned values */
		for (i = p->syms[0]->u.c.v.u - (p->syms[0]->u.c.v.u % XBC_LNGLNG_SIZE); i < p->syms[0]->u.c.v.u; i++) {
			if (glevel>5) print("# ADDR %d->%d\n", i, i);
			dumptree(p->kids[0]);
			if (i) {
				print("CNSTP%d %d\n", XBC_PTR_SIZE, i);
				print("ADDP%d\n", XBC_PTR_SIZE);
			}
			if (glevel>5) print("# FROM:\n");
			/* address of indir */
			dumptree(p->kids[1]->kids[0]);
			if (i) {
				print("CNSTP%d %d\n", XBC_PTR_SIZE, i);
				print("ADDP%d\n", XBC_PTR_SIZE);
			}
			/* copy as unsigned */
			if (glevel>5) print("# COPY\n");
			/* fetch */
			print("INDIRU%d\n", 1);
			/* assign */
			print("ASGNU%d\n", 1);
		}
		
		if (glevel>5) print("# END OF STRUCTURE ASSIGNMENT (%d bytes)\n", p->syms[0]->u.c.v.u);
		
		return;
	case RET+V:
		assert(!p->kids[0]);
		assert(!p->kids[1]);
		print("%s\n", opname(p->op));
		return;
	}
	switch (generic(p->op)) {
	case LABEL:
		/* label should be a directive (lower case) */
		assert(!p->kids[0]);
		assert(!p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		if (vid > 0) {
			print("label %s\n", p->syms[0]->x.name);
		} else {
			print("%s %s\n", opname(p->op), p->syms[0]->x.name);
		}
		return;
	case CNST: case ADDRG: case ADDRF: case ADDRL: 
		assert(!p->kids[0]);
		assert(!p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		print("%s %s\n", opname(p->op), p->syms[0]->x.name);
		return;
	case CVF: case CVI: case CVP: case CVU:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		assert(p->syms[0]);
		dumptree(p->kids[0]);
		print("%s %d\n", opname(p->op), p->syms[0]->u.c.v.i);
		return;
	case ARG:
		if (vid > 0x0020) {
			assert(p->kids[0]);
			assert(!p->kids[1]);

			// ADDRAP
			printf("ADDRAP%d %d\n", XBC_PTR_SIZE, arg_cntr);
			// VALUE
			dumptree(p->kids[0]);
			// ASSIGN
			printf("ASGN%s%d\n", suffixes[optype(p->op)], opsize(p->op));

			arg_cntr += opsize(p->op);
		} else {
			assert(p->kids[0]);
			assert(!p->kids[1]);
			dumptree(p->kids[0]);
			if (vid > 0) {
				print("%s %d\n", opname(p->op), arg_cntr);
				arg_cntr += opsize(p->op);
			} else {
				print("%s\n", opname(p->op));
			}
		}
		return;
	case BCOM: case NEG: case INDIR: case JUMP: case RET:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		dumptree(p->kids[0]);
		print("%s\n", opname(p->op));
		return;
	case CALL:
		assert(p->kids[0]);
		assert(!p->kids[1]);
		assert(optype(p->op) != B);
		dumptree(p->kids[0]);
		print("%s\n", opname(p->op));
		arg_cntr = 0;
		return;
	case ASGN: case BOR: case BAND: case BXOR: case RSH: case LSH:
	case ADD: case SUB: case DIV: case MUL: case MOD:
		assert(p->kids[0]);
		assert(p->kids[1]);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		print("%s\n", opname(p->op));
		return;
	case EQ: case NE: case GT: case GE: case LE: case LT:
		assert(p->kids[0]);
		assert(p->kids[1]);
		assert(p->syms[0]);
		assert(p->syms[0]->x.name);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		print("%s %s\n", opname(p->op), p->syms[0]->x.name);
		return;
	}
	assert(0);
}

static void I(emit)(Node p) {
	for (; p; p = p->link) {
		dumptree(p);

		if (generic(p->op) == CALL) {
			if (p->kids[0] && p->kids[0]->syms[0]) {
				if (p->kids[0]->syms[0]->sclass == 74) {
					warning("Function '%s' called without prototype!\n", p->kids[0]->syms[0]->name);
				} else if (optype(p->op) != VOID) {
					print("DISCARD%s%d\n", suffixes[optype(p->op)], opsize(p->op));
				}
			}
		}
/*
if (generic(p->op) == CALL) {
	if (p->kids[0] && p->kids[0]->syms[0]) {
		fprintf(stderr, "KID: %s\n", p->kids[0]->syms[0]->name);
		fprintf(stderr, "KID PROTO: %d\n", p->kids[0]->syms[0]->type->u.f.proto);
		fprintf(stderr, "KID SCLASS: %d\n", p->kids[0]->syms[0]->sclass);
	}
}

		if (generic(p->op) == CALL && optype(p->op) != VOID) {
			print("DISCARD%s%d\n", suffixes[optype(p->op)], opsize(p->op));
		}
*/


	}
}

static void I(export)(Symbol p) {
	print("export %s\n", p->x.name);
}

static void I(function)(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	int i;

	current_func = f->x.name;

	(*IR->segment)(CODE);
	offset = 0;
	for (i = 0; caller[i] && callee[i]; i++) {
		offset = roundup(offset, caller[i]->type->align);
		caller[i]->x.name = callee[i]->x.name = stringf("%d", offset);
		caller[i]->x.offset = callee[i]->x.offset = offset;
		offset += caller[i]->type->size;
	}
	maxargoffset = maxoffset = argoffset = offset = 0;
	gencode(caller, callee);
	
	//print_function_debug_info(f, callee);
	
	if (vid == 0) {
		print("proc %s %d %d\n", f->x.name, maxoffset, maxargoffset);
		emitcode();
		current_func = 0;
		print("endproc %s %d %d\n", f->x.name, maxoffset, maxargoffset);
	} else if (vid < 0x0020) {
		print("label %s\n", f->x.name);
		print("SAVEAPP%d\n", XBC_PTR_SIZE);
		print("SAVELPP%d\n", XBC_PTR_SIZE); 
		print("SAVEFPP%d\n", XBC_PTR_SIZE);		
		print("ARGSTACKP%d %d\n", XBC_PTR_SIZE, maxargoffset);
		print("VARSTACKP%d %d\n", XBC_PTR_SIZE, maxoffset);		
		emitcode();
		current_func = 0;
		print("RETV\n");
	} else {
		print("label %s\n", f->x.name);
		if (glevel && &f->src) I(stabline)(&f->src);
		if (maxargoffset) print("ARGSTACKP%d %d\n", XBC_PTR_SIZE, maxargoffset);
		if (maxoffset) print("VARSTACKP%d %d\n", XBC_PTR_SIZE, maxoffset);		
		emitcode();
		current_func = 0;
		print("RETV\n");
	}
	arg_cntr = 0;
}

static void gen02(Node p) {
	assert(p);
	if (generic(p->op) == ARG) {
		assert(p->syms[0]);
		argoffset += (p->syms[0]->u.c.v.i < 4 ? 4 : p->syms[0]->u.c.v.i);
	} else if (generic(p->op) == CALL) {
		maxargoffset = (argoffset > maxargoffset ? argoffset : maxargoffset);
		argoffset = 0;
	}
}

static void gen01(Node p) {
	if (p) {
		gen01(p->kids[0]);
		gen01(p->kids[1]);
		gen02(p);
	}
}

static Node I(gen)(Node p) {
	Node q;

	assert(p);
	for (q = p; q; q = q->link)
		gen01(q);
	return p;
}

static void I(global)(Symbol p) {
	//print_global_debug_info(p);
	print("align %d\n", p->type->align > 4 ? 4 : p->type->align);
	if (vid > 0) {
		print("label %s\n", p->x.name);
	} else {
		print("LABELV %s\n", p->x.name);
	}
}

static void I(import)(Symbol p) {
	print("import %s\n", p->x.name);
}

static void I(local)(Symbol p) {
	offset = roundup(offset, p->type->align);
	p->x.name = stringf("%d", offset);
	p->x.offset = offset;

	print_local_debug_info(p);

	offset += p->type->size;
}

/* 
 * convert hexadecimal string to integer
 */
int hex2bin(char* hex) {
	int i, ret;
	i = ret = 0;
	while(hex[i]) {
		ret <<= 4;
		if (hex[i] >= '0' & hex[i] <= '9') {
			ret += hex[i] - '0';
		} else if (hex[i] >= 'A' & hex[i] <= 'F') {
			ret += hex[i] - 'A' + 10;
		} else if (hex[i] >= 'a' & hex[i] <= 'f') {
			ret += hex[i] - 'a' + 10;
		} else {
			return -1;
		}
		i++;
	}
	return ret;
}

static void I(progbeg)(int argc, char *argv[]) {
	int i;
	
	assert(XBC_SHORT_SIZE);
	assert(XBC_INT_SIZE);
	assert(XBC_LNG_SIZE);
	assert(XBC_LNGLNG_SIZE);
	assert(XBC_PTR_SIZE);
	
	if (XBC_SHORT_SIZE < 2) warning("Conform to the ANSI C standard, a short integer must be at least 16 bits\n");
	if (XBC_INT_SIZE < 2) warning("Conform to the ANSI C standard, an integer must be at least 16 bits\n");
	if (XBC_LNG_SIZE < 4) warning("Conform to the ANSI C standard, a long integer must be at least 32 bits\n");
	if (XBC_SHORT_SIZE > XBC_INT_SIZE) warning("Conform to the ANSI C standard, an integer must be larger then a short integer\n");
	if (XBC_INT_SIZE > XBC_LNG_SIZE) warning("Conform to the ANSI C standard, a long integer must be larger then an integer\n");
	if (XBC_LNG_SIZE < XBC_LNGLNG_SIZE) warning("Conform to the ANSI C standard, a long long integer must be larger then a long integer\n");
	
	for (i = 0; i < argc; i++) {
		if (strncmp(argv[i], "-vid=", 5) == 0) {
			vid = hex2bin(&argv[i][5]);
			if (vid < 0) error("Invalid -vid switch");
		}
	}
	
	
	
	
}

static void I(progend)(void) {}

static void I(space)(int n) {
	print("skip %d\n", n);
}

static void I(stabblock)(int brace, int lev, Symbol *p) {
}

static void I(stabend)(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	print("# End of code: %s:%d\n", cp->file, cp->y);
}

static void I(stabfend)(Symbol p, int lineno) {
	print("# Function end: %s (line %d)\n", p->name, lineno);
}

static void I(stabinit)(char *file, int argc, char *argv[]) {
	int i;
	print("# Compiling %s...\n", file);
	print("# Parameters:\n", file);
	for (i = 0; i < argc; i++) print("#    %s\n", argv[i]);
}

static void I(stabline)(Coordinate *cp) {
	static char *prevfile;
	static int prevline;
	
	if (cp->file && (prevfile == NULL || strcmp(prevfile, cp->file) != 0)) {
		print("file \"%s\"\n", prevfile = cp->file);
		prevline = 0;
	}
	if (cp->y != prevline)
		print("line %d\n", prevline = cp->y);
}

static void I(stabsym)(Symbol p) {
	int i, loc;
	static Symbol prev;
	static Symbol last_func;

	if (!p) return;

	switch(p->scope) {
		case 3:	// global
			if (p->type && p->type->op == 11) {
				// function: p->u.f
				if (p->name && p->type && p->type->type) {
					print("function %s <", p->name);
					print_type_name(p->type->type);
					print(">\n");
				}
				last_func = p;
			} else if (p->name && p->type) {
				print("global %s <", p->name);
				print_type_name(p->type);
				print(">\n");
			}
			break;
		case 4: // parameter
			if (prev && prev->name != p->name && last_func) {
				// some parameters (like char) are printed as char and int,
				//		only print one of them.
				print("param %s:%s %d <", last_func->name, p->name, p->x.offset);
				print_type_name(p->type);
				print(">\n");
			}
			
			break;
		default:
			//TODO
			print("# WARNING: Unknown scope, check %s:%d\n", __FILE__,__LINE__);
			print("#     Scope: %d, Name: %s, Type: <", p->scope, p->name?p->name:"<unkown>");
			print_type_name(p->type);
			print(">\n");
			break;
	}
	prev = p;
}

static void I(stabtype)(Symbol p) {
	Field *q, r;
	int pos;

	if (p->type && p->name) {
		if (p != p->type->u.sym) {
			// typedef to other type
			print("typedef %s:%s\n", p->name, p->type->u.sym->name);
		} else {
			// typedef to primitive
			if (p->type->op == 13) {
				// enum
				if (p->type->type) {
					print("typedef %s:%s(%s[%d])\n", p->name, type_names[p->type->op], 
						type_names[p->type->type->op], p->type->size);
				}
			} else {
				print("typedef %s:%s[%d]\n", p->name, type_names[p->type->op], p->type->size);
			}
		}
		if (p->type->op == 9 || p->type->op == 10 || p->type->op == 13) {
			// struct, enum, union
			q = &p->u.s.flist;
			pos = 0;
			for (r = *q; r; q = &r->link, r = *q) {
				if (r && r->type) {
					pos = roundup(pos, r->type->align);
					print("field %s:%s %d <", p->name, r->name, pos);
					print_type_name(r->type);
					print(">\n");
					pos += r->type->size;
				}
			}
		} 
	}
}

#define b_blockbeg blockbeg
#define b_blockend blockend

Interface xbytecodeIR = {
	1, 1, 0,	/* char */
	XBC_SHORT_SIZE, XBC_SHORT_SIZE, 0,	/* short */
	XBC_INT_SIZE, XBC_INT_SIZE, 0,	/* int */
	XBC_LNG_SIZE, XBC_LNG_SIZE, 0,	/* long */
	XBC_LNGLNG_SIZE, XBC_LNGLNG_SIZE, 0,	/* long long */
	4, 4, 1,	/* float */
	8, 8, 1,	/* double */
	8, 8, 1,	/* long double */
	XBC_PTR_SIZE, XBC_PTR_SIZE, 0,	/* T* */
	0, 4, 0,	/* struct */
	0,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	0,		/* wants_argb */
	1,		/* left_to_right */
	0,		/* wants_dag */
	0,		/* unsigned_char */
	I(address),
	I(blockbeg),
	I(blockend),
	I(defaddress),
	I(defconst),
	I(defstring),
	I(defsymbol),
	I(emit),
	I(export),
	I(function),
	I(gen),
	I(global),
	I(import),
	I(local),
	I(progbeg),
	I(progend),
	I(segment),
	I(space),
	I(stabblock),
	I(stabend),
	I(stabfend),
	I(stabinit),
	I(stabline),
	I(stabsym),
	I(stabtype),
};


/*
 * print debug symbol for a local variable
 */
static void print_local_debug_info(Symbol s) {
	if (glevel && s && !s->generated && s->name) {
		print("local %s:%s %d <", current_func, s->name, s->x.offset);
		print_type_name(s->type);
		print(">\n");
	}
}

/*
 * print debug symbol for a global variable
 */
/*
static void print_global_debug_info(Symbol s) {
	if (glevel && !s->generated) {
		print("# OLD: global %s ", s->name);
		print_type_debug_info(s->type);
		print("\n");
	}
}
*/

/*
 * print debug symbol for a function's return value and parameters
 */
/*
static void print_function_debug_info(Symbol s, Symbol callee[]) {
	int i, loc;
	
	if (glevel && !s->generated) {
		print("# OLD: function %s ", s->name);
		print_type_debug_info(s->type->type);
		print("\n");
		loc = 0;

		for (i = 0; callee[i]; i++) {
			print("# OLD: param %s:%s %d ", s->name, callee[i]->name, loc);
			print_type_debug_info(callee[i]->type);
			print("\n");
			loc += callee[i]->type->size;
		}
	}
}
*/

/*
 * print debug symbol for a type
 */
/*
static void print_type_debug_info(Type t) {
	if(!t) return;

	print("<");
	
	while(t->type) {
		print("%s[%d] to ", type_names[t->op], t->size);
		t = t->type;
	}
	if (t->op == 9) {
		print("%s:%s[%d]>", type_names[t->op], findtype(t)->name, t->size);
	} else {
		print("%s[%d]>", type_names[t->op], t->size);
	}
}
*/

static void print_type_name(Type t) {
	Type q;
	int constant = 0;
	int function = 0;

	if (!t) return;

	// find the end of the type chain:
	q = t;
	while(q->type && (q->op == 0x07 || q->op == 0x0C || q->op == 0x0F || q->op == 0x0B)) {
		if (q->op == 0x0F) constant = 1;
		if (q->op == 0x0B) function = 1;
		q = q->type;
	}

	// typedef name:
	if (q->u.sym && q->u.sym->name) {
		print("%s", q->u.sym->name);
	} else {
		print("<unknown:%d>", q->op);
	} 

	// print array sizes:
	q = t;
	while(q->type && q->op == 0x0C) {
		print("[%d]", q->size / q->type->size);
		q = q->type;
	}

	// print pointer signs:
	while(q->type && q->op == 0x07) {
		print("*");
		q = q->type;
	}

	if (function) print("()");
}

static void print_type_info(Type t) {
	print("# ");
	while(t) {
		if (t->u.sym && t->u.sym->name) {
			print("%s[%d]", t->u.sym->name, t->size);
		} else {
			print("<%s>[%d]", type_names[t->op], t->size);
		} 
		if (t = t->type) print(".");
	}
	print("\n");
}
