COMPORT=${X32_PACKAGE_SERIAL}

default: ex2
.PHONY : clean default test ex1 

ex1: ex1.ce
	x32-upload ex1.ce -c ${COMPORT} 
	x32-term start

ex2: ex2.ce
	x32-upload ex2.ce -c ${COMPORT}
	x32-term start

ex3: ex3.ce
	x32-upload ex3.ce -c ${COMPORT}
	x32-term start

ex4: ex4.ce
	x32-upload ex4.ce -c ${COMPORT}
	x32-term start

test: test.ce
	x32-upload test.ce -c ${COMPORT} 
	x32-term start

ex2.ce : ex2.c
	lcc-x32 -g ex2.c -Wl-lib -Wl. -o ex2.ce 

ex3.ce : ex3.c
	lcc-x32 -g ex3.c -Wl-lib -Wl. -o ex3.ce 

test.ce: test.c 
	lcc-x32 -g test.c -Wl-lib -Wl. -o test.ce 

clean : 
	rm test.ce ex1.ce ex2.ce test.dbg ex1.dbg ex2.dbg
