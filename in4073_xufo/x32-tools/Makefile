#####################################################################
### CONFIGURATION SECTION
#####################################################################
## C Compiler
CC=gcc
## flags for c compiler
CFLAGS= -g

## build directory (MUST EXIST!)
BINDIR=bin

## executable file extention
EXE=
## extra flags to compile the libraries, use empty or -g to compile
## with debug symbols
LIB_COMPILE_FLAGS=

#####################################################################
### SOURCE LOCATIONS
#####################################################################
## source directory
SRCDIR=src

## library directory (for x32 libraries, not the standard c libraries)
LIBDIR=lib-x32

## LCC X32 Linux Driver
LCCDRIVER=$(SRCDIR)/lcc/x32-linux.c

## source files:
COMMON_SOURCES=$(SRCDIR)/hashtable.c $(SRCDIR)/list.c $(SRCDIR)/memstream.c \
	$(SRCDIR)/instructions.c $(SRCDIR)/textparser.c
ASSEMBLER_SOURCES=$(SRCDIR)/assembler.c 
LINKER_SOURCES=$(SRCDIR)/linker.c $(SRCDIR)/library.c $(SRCDIR)/segments.c 
INTERPRETER_SOURCES=$(SRCDIR)/interpreter.c $(SRCDIR)/interpreter_engine.c \
	$(SRCDIR)/io.c $(SRCDIR)/interpreter_peripherals.c
BIN2VHD_SOURCES=$(SRCDIR)/bin2vhd.c
BIN2BIT_SOURCES=$(SRCDIR)/bin2bit.c
UPLOADER_SOURCES=$(SRCDIR)/uploader.c $(SRCDIR)/serial.c \
	$(SRCDIR)/upload_lib.c 
RS232MEMSERVER_SOURCES=$(SRCDIR)/rs232memserver.c $(SRCDIR)/serial.c \
	$(SRCDIR)/io.c $(SRCDIR)/instructions.c $(SRCDIR)/debug_decoder.c
RCC_SOURCES=$(SRCDIR)/lcc/alloc.c $(SRCDIR)/lcc/bind.c $(SRCDIR)/lcc/dag.c \
	$(SRCDIR)/lcc/decl.c $(SRCDIR)/lcc/enode.c $(SRCDIR)/lcc/error.c \
	$(SRCDIR)/lcc/expr.c $(SRCDIR)/lcc/event.c $(SRCDIR)/lcc/init.c \
	$(SRCDIR)/lcc/inits.c $(SRCDIR)/lcc/input.c $(SRCDIR)/lcc/lex.c \
	$(SRCDIR)/lcc/list.c $(SRCDIR)/lcc/main.c $(SRCDIR)/lcc/output.c \
	$(SRCDIR)/lcc/prof.c $(SRCDIR)/lcc/profio.c $(SRCDIR)/lcc/simp.c \
	$(SRCDIR)/lcc/stmt.c $(SRCDIR)/lcc/string.c $(SRCDIR)/lcc/sym.c \
	$(SRCDIR)/lcc/trace.c $(SRCDIR)/lcc/tree.c $(SRCDIR)/lcc/types.c \
	$(SRCDIR)/lcc/null.c $(SRCDIR)/lcc/gen.c $(SRCDIR)/lcc/xbytecode.c 
LCC_SOURCES=$(SRCDIR)/lcc/lcc.c $(LCCDRIVER)
CPP_SOURCES=$(SRCDIR)/cpp/cpp.c $(SRCDIR)/cpp/lex.c $(SRCDIR)/cpp/nlist.c \
	$(SRCDIR)/cpp/tokens.c $(SRCDIR)/cpp/macro.c $(SRCDIR)/cpp/eval.c \
	$(SRCDIR)/cpp/include.c $(SRCDIR)/cpp/hideset.c $(SRCDIR)/cpp/getopt.c \
	$(SRCDIR)/cpp/unix.c

BOOTSTRAP_SOURCE=$(SRCDIR)/bootstrap.bc

LIB_IO_SOURCES= \
	$(LIBDIR)/stdio/puts.c $(LIBDIR)/stdio/printf.c \
	$(LIBDIR)/stdio/xprintf.c $(LIBDIR)/stdio/sprintf.c
LIB_LIB_SOURCES=$(LIBDIR)/stdlib/rand.c $(LIBDIR)/stdlib/exit.c \
	$(LIBDIR)/stdlib/return_from_main.bc \
	$(LIBDIR)/stdlib/abs.c $(LIBDIR)/stdlib/labs.c \
	$(LIBDIR)/stdlib/div.c $(LIBDIR)/stdlib/ldiv.c \
	$(LIBDIR)/stdlib/malloc.c $(LIBDIR)/stdlib/calloc.c \
	$(LIBDIR)/stdlib/atoi.c $(LIBDIR)/stdlib/atol.c \
	$(LIBDIR)/stdlib/strtol.c $(LIBDIR)/stdlib/strtoul.c
LIB_FLOAT_SOURCES=$(LIBDIR)/softfloat/softfloat.c
LIB_STRING_SOURCES=$(LIBDIR)/string/memchr.c $(LIBDIR)/string/memcmp.c \
	$(LIBDIR)/string/memcpy.c $(LIBDIR)/string/memmove.c \
	$(LIBDIR)/string/memset.c $(LIBDIR)/string/strcat.c \
	$(LIBDIR)/string/strchr.c $(LIBDIR)/string/strcmp.c \
	$(LIBDIR)/string/strcpy.c $(LIBDIR)/string/strerror.c \
	$(LIBDIR)/string/strcspn.c $(LIBDIR)/string/strlen.c \
	$(LIBDIR)/string/strlwr.c $(LIBDIR)/string/strspn.c \
	$(LIBDIR)/string/strrchr.c $(LIBDIR)/string/strupr.c \
	$(LIBDIR)/string/strncpy.c $(LIBDIR)/string/strncmp.c \
	$(LIBDIR)/string/strtok.c 
LIB_ASSERT_SOURCES=$(LIBDIR)/assert/assert.c
LIB_SETJMP_SOURCES=${LIBDIR}/setjmp/setjmp.c ${LIBDIR}/setjmp/longjmp.c \
	${LIBDIR}/setjmp/get_fp.bc ${LIBDIR}/setjmp/set_fp.bc 
LIB_STDARG_SOURCES=${LIBDIR}/stdarg/stdarg.c
LIB_HEADERS=$(LIBDIR)/assert.h $(LIBDIR)/stdlib.h $(LIBDIR)/stdio.h \
	$(LIBDIR)/string.h $(LIBDIR)/time.h $(LIBDIR)/setjmp.h \
	$(LIBDIR)/stdarg.h \
	$(LIBDIR)/softfloat.h $(LIBDIR)/x32.h 

#####################################################################
### MAKEFILE RULES
#####################################################################

.PHONY : assembler linker bootstrap interpreter \
	bin2vhd bin2bit bootcode libraries uploader \
	all clean lcc rcc rs232memserver

all: assembler linker interpreter libraries \
	bootstrap bin2vhd bin2bit uploader lcc rcc rs232memserver

## all tools required to compile x32 executables or libraries
COMPILE_TOOLS=$(BINDIR)/x32-asm$(EXE) $(BINDIR)/x32-link$(EXE) \
	$(BINDIR)/lcc-x32$(EXE) $(BINDIR)/rcc$(EXE)

assembler: $(BINDIR)/x32-asm$(EXE)
linker: $(BINDIR)/x32-link$(EXE)
bootstrap: $(BINDIR)/bootstrap.co
interpreter: $(BINDIR)/x32-sim$(EXE)
bin2vhd: $(BINDIR)/bin2vhd$(EXE)
bin2bit: $(BINDIR)/bin2bit$(EXE)
uploader: $(BINDIR)/x32-upload$(EXE) 
rs232memserver: $(BINDIR)/x32-memserv$(EXE) 
lcc: $(BINDIR)/lcc-x32$(EXE)
cpp: $(BINDIR)/cpp$(EXE)
rcc: $(BINDIR)/rcc$(EXE)
	
libraries: $(LIBDIR)/stdio.cl $(LIBDIR)/softfloat.cl \
	$(LIBDIR)/string.cl $(LIBDIR)/stdlib.cl \
	$(LIBDIR)/assert.cl $(LIBDIR)/setjmp.cl $(LIBDIR)/stdarg.cl

#####################################################################
### ASSEMBLER
#####################################################################
$(BINDIR)/x32-asm$(EXE) : $(ASSEMBLER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(ASSEMBLER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o)  -o $@

#####################################################################
### LINKER
#####################################################################
$(BINDIR)/x32-link$(EXE) : $(LINKER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(LINKER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o) -o $@

#####################################################################
### INTERPRETER
#####################################################################
$(BINDIR)/x32-sim$(EXE) : $(INTERPRETER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(INTERPRETER_SOURCES:%.c=%.o) $(COMMON_SOURCES:%.c=%.o)  -o $@

#####################################################################
### BIN2VHD
#####################################################################
$(BINDIR)/bin2vhd$(EXE) : $(BIN2VHD_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(BIN2VHD_SOURCES:%.c=%.o) -o $@

#####################################################################
### BIN2BIT
#####################################################################
$(BINDIR)/bin2bit$(EXE) : $(BIN2BIT_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(BIN2BIT_SOURCES:%.c=%.o) -o $@

#####################################################################
### UPLOADER
#####################################################################
$(BINDIR)/x32-upload$(EXE) : $(UPLOADER_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(UPLOADER_SOURCES:%.c=%.o) -o $@

#####################################################################
### UPLOADER
#####################################################################
$(BINDIR)/x32-memserv$(EXE) : $(RS232MEMSERVER_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(RS232MEMSERVER_SOURCES:%.c=%.o) -o $@

#####################################################################
### RCC
#####################################################################
$(BINDIR)/rcc$(EXE) : $(RCC_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(RCC_SOURCES:%.c=%.o) -o $@

#####################################################################
### LCC
#####################################################################
$(BINDIR)/lcc-x32$(EXE) : $(LCC_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(LCC_SOURCES:%.c=%.o) -o $@
	
#####################################################################
### CPP
#####################################################################
$(BINDIR)/cpp$(EXE) : $(CPP_SOURCES:%.c=%.o) 
	$(CC) $(CFLAGS) $(CPP_SOURCES:%.c=%.o) -o $@

#####################################################################
### BOOTSTRAP CODE
#####################################################################
$(BINDIR)/bootstrap.co: $(BOOTSTRAP_SOURCE) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -c -Wo-lccdir=$(BINDIR) \
		$(BOOTSTRAP_SOURCE) -o $@

#####################################################################
### LIBRARIES
#####################################################################
$(LIBDIR)/stdio.cl: $(LIB_IO_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_IO_SOURCES) -o $@

$(LIBDIR)/softfloat.cl: $(LIB_FLOAT_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_FLOAT_SOURCES) -o $@

$(LIBDIR)/string.cl: $(LIB_STRING_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_STRING_SOURCES) -o $@

$(LIBDIR)/stdlib.cl: $(LIB_LIB_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_LIB_SOURCES) -o $@

$(LIBDIR)/assert.cl: $(LIB_ASSERT_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_ASSERT_SOURCES) -o $@

$(LIBDIR)/setjmp.cl: $(LIB_SETJMP_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_SETJMP_SOURCES) -o $@

$(LIBDIR)/stdarg.cl: $(LIB_STDARG_SOURCES) $(COMPILE_TOOLS)
	$(BINDIR)/lcc-x32$(EXE) -Wo-lccdir=$(BINDIR) -Wo-buildlib \
		$(LIB_COMPILE_FLAGS) $(LIB_STDARG_SOURCES) -o $@

#####################################################################
### OBJECT FILES
#####################################################################
%.o: %.c
	$(CC) $(CFLAGS) -c $(@:%.o=%.c) -o $@

#####################################################################
### CLEAN
#####################################################################
clean:
	-$(RM) $(BINDIR)/x32-asm$(EXE)
	-$(RM) $(BINDIR)/x32-link$(EXE)
	-$(RM) $(BINDIR)/x32-sim$(EXE)
	-$(RM) $(BINDIR)/x32-upload$(EXE)
	-$(RM) $(BINDIR)/bin2vhd$(EXE)
	-$(RM) $(BINDIR)/bin2bit$(EXE)
	-$(RM) $(BINDIR)/rcc$(EXE)
	-$(RM) $(BINDIR)/lcc-x32$(EXE)
	-$(RM) $(BINDIR)/x32-memserv$(EXE)
	-$(RM) $(LIBDIR)/stdio.cl
	-$(RM) $(LIBDIR)/softfloat.cl
	-$(RM) $(LIBDIR)/time.cl
	-$(RM) $(LIBDIR)/stdlib.cl
	-$(RM) $(LIBDIR)/assert.cl
	-$(RM) $(LIBDIR)/string.cl
	-$(RM) $(LIBDIR)/setjmp.cl
	-$(RM) $(LIBDIR)/stdarg.cl
	-$(RM) $(BINDIR)/bootstrap.co
	-$(RM) $(COMMON_SOURCES:%.c=%.o)
	-$(RM) $(ASSEMBLER_SOURCES:%.c=%.o)
	-$(RM) $(LINKER_SOURCES:%.c=%.o)
	-$(RM) $(INTERPRETER_SOURCES:%.c=%.o)
	-$(RM) $(CONVERTER_SOURCES:%.c=%.o)
	-$(RM) $(UPLOADER_SOURCES:%.c=%.o)
	-$(RM) $(BIN2VHD_SOURCES:%.c=%.o)
	-$(RM) $(BIN2BIT_SOURCES:%.c=%.o)
	-$(RM) $(RCC_SOURCES:%.c=%.o)
	-$(RM) $(LCC_SOURCES:%.c=%.o)
	-$(RM) $(RS232MEMSERVER_SOURCES:%.c=%.o)

	
