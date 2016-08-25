OBJS= abort.o abs.o atexit.o atof.o atoi.o atol.o bsearch.o div.o labs.o \
      ldiv.o multibyte.o qsort.o rand.o strtod.o strtol.o strtoul.o system.o

LIB_NAME = stdlib	  
	  
CC_INCLUDE =  -I.  -I../../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libstdlib.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm

