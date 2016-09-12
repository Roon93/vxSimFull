OBJS=	memchr.o memcmp.o memcpy.o memmove.o memset.o strcat.o strchr.o \
	strcmp.o strcoll.o strcpy.o strcspn.o strerror.o strlen.o strncat.o \
        strncmp.o strncpy.o strpbrk.o strrchr.o strspn.o strstr.o strtok.o \
        strtok_r.o strxfrm.o xstate.o

CC_INCLUDE =  -I.  -I../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################

libstring.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 




