Folder in4073_xufo contains the resources needed for this project, 
Folder ufo is reserved for your source codes.


Please execute these after logging on:
$ cd in4073_xufo
$ source setup_in4073 
$ source setup_xilinx

Try running the compiler. The output should look something like:

$ lcc-x32 
lcc-x32 [ option | file ]...
	except for -l, options are processed left-to-right before files
	unrecognized options are taken to be linker options
-A	warn about nonANSI usage; 2nd -A warns more
-b	emit expression-level profiling code; see bprint(1)
-Bdir/	use the compiler named `dir/rcc'
-c	compile only
...
...

if the lcc-x32 executable cannot be executed it will show this error:

$ lcc-x32 
bash: ~/in4073/in4073TE300/in4073_xufo/x32-tools/bin/lcc-x32: cannot execute binary file

You need to rebuild the tools to work on your computer.

$ make clean
$ make all


Also read:
./ufo/fpga/readme.txt 
./ufo/pc/readme.txt.
./in4073_xufo/x32/readme.txt


And remember our project resources page: http://www.st.ewi.tudelft.nl/~gemund/Courses/In4073/Resources/index.html



Cheers, 
Lab Assistants.
