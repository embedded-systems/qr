
# to upload one of the executables (.ce) type:
#  x32-upload file.ce -c /dev/ttyS0
# to upload and execute immediately type:
#  x32-upload file.ce -c /dev/ttyS0 -e
 


all: buttons.ce console.ce critical.ce leds.ce reaction.ce slicing.ce ticsdemo.ce timing.ce


%.ce: %.c
	lcc-x32 $(@:%.ce=%.c) -I${X32INC} -lib ${X32INC} -Wl-lib -o $@

ticsdemo.ce: ticsdemo.c ../tics/tics.cl
	lcc-x32 -I${X32INC} -lib ${X32INC} -I../tics ticsdemo.c -Wl-lib -Wl../tics/tics.cl -Wl-lib -o ticsdemo.ce

clean:
	rm -f *.ce