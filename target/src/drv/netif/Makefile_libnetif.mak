
OBJS = if_ei.o \
	      if_ln.o if_lnPci.o if_loop.o \
	      if_sl.o if_sm.o if_sn.o smNetLib.o smNetShow.o \
	      if_elc.o if_dc.o if_ultra.o if_eex.o if_fei.o \
	      if_elt.o if_ene.o if_esmc.o \
	      if_cs.o if_eidve.o if_cpm.o

CC_INCLUDE =  -I.  -I../../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################	

libnetif.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 

