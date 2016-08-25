DOC_FILES=	eventLib.c kernelLib.c msgQEvLib.c msgQLib.c msgQShow.c \
		semBLib.c semCLib.c semEvLib.c semLib.c semMLib.c \
		semOLib.c semShow.c taskInfo.c taskLib.c taskShow.c \
		tickLib.c wdLib.c wdShow.c

OBJS=	eventLib.o eventShow.o kernelLib.o msgQEvLib.o msgQLib.o msgQShow.o \
	schedLib.o semBLib.o semCLib.o semEvLib.o semLib.o \
	semMLib.o semOLib.o semShow.o taskLib.o \
	taskInfo.o taskShow.o tickLib.o wdLib.o wdShow.o windLib.o workQLib.o

CC_INCLUDE =  -I.  -I../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu -DPORTABLE

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################

libwind.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 


