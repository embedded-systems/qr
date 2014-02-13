COMPORT=$(SERIALDEVICE)

SOURCES=\
	test.c \
	test_math.c \
	test_comp.c \
	test_array_struct.c \
	test_lib_string.c \
	test_lib_std_setjmp.c \
	test_lib_io.c \
	test_alu_errors.c \
	test_trap.c \
	test_oom.c

SOURCES_DURA=\
	test_dura.c \
	test_math.c \
	test_comp.c \
	test_array_struct.c \
	test_lib_string.c \
	test_lib_std_setjmp.c \
	test_lib_io.c \
	test_alu_errors.c \
	test_trap.c \
	test_oom.c

.PHONY : clean all default upload sim uploaddura simdura

default: test.ce
all: test.ce test.gcc

clean:
	rm -rf test.ce
	rm -rf test.gcc

test.ce : $(SOURCES)
	lcc-x32 $(SOURCES) -g -o $@

test-dura.ce : $(SOURCES_DURA)
	lcc-x32 $(SOURCES_DURA) -g -o $@ -DDURA

test.gcc : $(SOURCES)
	gcc $(SOURCES) -o $@ -DIX86

upload : test.ce
	x32-upload -c $(COMPORT) test.ce
sim : test.ce
	x32-sim test.ce

uploaddura : test-dura.ce
	x32-upload -c $(COMPORT) test-dura.ce
simdura : test-dura.ce
	x32-sim test-dura.ce
