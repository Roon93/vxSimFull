DOC_FILES=	ledLib.c loadLib.c loginLib.c moduleLib.c \
		remShellLib.c shellLib.c spyLib.c timexLib.c unldLib.c dbgLib.c

YACCOUT=y.tab.c

CFLAGS_repeatHost.o	= $(LONGCALL)
CFLAGS_periodHost.o	= $(LONGCALL)
CFLAGS_spyLib.o		= $(LONGCALL)
CFLAGS_ttHostLib.o	= $(LONGCALL)

OBJS=	bootAoutLib.o bootElfLib.o \
	bootLoadLib.o \
	dbgLib.o dbgTaskLib.o\
	ledLib.o \
	loadAoutLib.o \
	loadElfLib.o loadLib.o loadPecoffLib.o\
	loginLib.o moduleLib.o periodHost.o repeatHost.o \
	remShellLib.o shell.o shellLib.o spyLib.o \
	timexLib.o ttHostLib.o \
	unldLib.o 

LIB_NAME = ostool	

CC_INCLUDE =  -I.  -I../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

lib$(LIB_NAME).a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 
