

#OBJS=	pppLib.o magic.o fsm.o lcp.o ipcp.o upap.o chap.o ppp_md5.o crypt.o \
#	auth.o options.o ppp_vxworks.o if_ppp.o random.o \
#	pppShow.o pppSecretLib.o pppHookLib.o

OBJS = random.o

CC_INCLUDE =  -I.  -I../../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libppp.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 


