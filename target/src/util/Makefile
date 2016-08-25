

DOC_FILES=	bLib.c bootLib.c lstLib.c rngLib.c inflateLib.c rBuffLib.c

OBJS=	bLib.o bootLib.o bufLib.o cksumLib.o dllLib.o lstLib.o qFifoLib.o \
	qJobLib.o qLib.o qPriBMapLib.o qPriDeltaLib.o \
	qPriHeapLib.o qPriListLib.o rngLib.o sllLib.o smUtilLib.o \
	uncompress.o inflateLib.o rBuffLib.o rBuffShow.o

CC_INCLUDE =  -I.  -I../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu -DPORTABLE

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################

libutil.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 


