LIB_NAME=drv

OBJS= nec765Fd.o  l64853Dma.o i8237Dma.o i8250Sio.o ln97xEnd.o endLib.o

CC_INCLUDE =  -I.  -I../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
	
nec765Fd.o:
	$(RM) $@
	cc $(CFLAGS) -c ./fdisk/nec765Fd.c
	
i8237Dma.o:
	$(RM) $@
	cc $(CFLAGS) -c ./dma/i8237Dma.c
	
l64853Dma.o:
	$(RM) $@
	cc $(CFLAGS) -c ./dma/l64853Dma.c
	

i8250Sio.o:
	$(RM) $@
	cc $(CFLAGS) -c ./sio/i8250Sio.c
	
ln97xEnd.o:
	$(RM) $@
	cc $(CFLAGS) -c ./end/ln97xEnd.c
	
endLib.o:
	$(RM) $@
	cc $(CFLAGS) -c ./end/endLib.c
	

###############################################################################build targets###############################################	

lib$(LIB_NAME).a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 


