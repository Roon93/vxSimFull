DOC_FILES=	cacheLib.c clockLib.c dirLib.c dspLib.c dspShow.c \
		envLib.c errnoLib.c \
		excLib.c fioLib.c floatLib.c fppLib.c fppShow.c intLib.c \
		ioLib.c iosLib.c iosShow.c logLib.c memLib.c \
		memPartLib.c memShow.c ntPassFsLib.c pipeDrv.c ptyDrv.c \
		rebootLib.c rt11FsLib.c \
		scsiLib.c scsi1Lib.c cdromFsLib.c \
		scsi2Lib.c scsiCommonLib.c scsiDirectLib.c scsiSeqLib.c \
		scsiMgrLib.c scsiCtrlLib.c \
		selectLib.c sigLib.c \
		symLib.c taskHookLib.c taskHookShow.c \
		tapeFsLib.c \
		taskVarLib.c timerLib.c tyLib.c vmBaseLib.c \
		passFsLib.c unixDrv.c \
		ttyDrv.c hashLib.c

LIB_BASE_NAME	= os

OBJS_COMMON=	cacheLib.o classLib.o classShow.o clockLib.o copyright.o \
	dirLib.o envLib.o errnoLib.o excLib.o \
	ffsLib.o fioLib.o floatLib.o fppLib.o fppShow.o funcBind.o \
	hashLib.o intLib.o ioLib.o iosLib.o iosShow.o logLib.o \
	memLib.o memPartLib.o memShow.o objLib.o pathLib.o \
	pipeDrv.o ptyDrv.o rebootLib.o rt11FsLib.o \
	scsiLib.o scsi1Lib.o cdromFsLib.o \
	scsi2Lib.o scsiCommonLib.o scsiDirectLib.o scsiSeqLib.o \
	scsiMgrLib.o scsiCtrlLib.o \
	selectLib.o sigLib.o smLib.o smPktLib.o \
	symLib.o symShow.o taskHookLib.o taskHookShow.o taskVarLib.o \
	tapeFsLib.o \
	timerLib.o ttyDrv.o tyLib.o vmBaseLib.o vmData.o \
	passFsLib.o unixDrv.o ntPassFsLib.o

OBJS		= $(OBJS_COMMON) 

CC_INCLUDE =  -I.  -I../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libos.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 



