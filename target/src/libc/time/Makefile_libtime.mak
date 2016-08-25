
OBJS= locTime.o asctime.o clock.o ctime.o difftime.o gmtime.o localtime.o \
      mktime.o strftime.o time.o

LIB_NAME = time
	  
CC_INCLUDE =  -I.  -I../../../h  -I..

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

