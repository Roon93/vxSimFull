
OBJS=usrLib.o statTbl.o memDrv.o ramDrv.o devSplit.o	\
	usrDosFsOld.o usrFsLib.o usrFdiskPartLib.o ramDiskCbio.o \
        tarLib.o

LIB_NAME = usr
		
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
