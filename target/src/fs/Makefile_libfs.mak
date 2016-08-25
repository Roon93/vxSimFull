
LIB_NAME	= fs

OBJS=	\
    cbioLib.o	\
    dcacheCbio.o	\
    dosChkLib.o		\
    dosDirOldLib.o	\
    dosFsFat.o		\
    dosFsFmtLib.o	\
    dosFsLib.o		\
    dosVDirLib.o	\
    dpartCbio.o		\
    print64Lib.o	\
    rawFsLib.o


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
	$(RM) *.o
