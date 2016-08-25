

OBJS=	nec765Fd.o	

CC_INCLUDE =  -I.  -I../../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
	
$(OBJS) : $(OBJS)