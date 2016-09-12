
OBJS=	clearerr.o fclose.o fdopen.o feof.o ferror.o fflush.o fgetc.o \
        fgetpos.o fgets.o fileno.o flags.o fopen.o fprintf.o \
        fputc.o fputs.o fread.o freopen.o fscanf.o fseek.o fsetpos.o \
        ftell.o fvwrite.o fwrite.o getc.o getchar.o gets.o getw.o \
	makebuf.o perror.o putc.o putchar.o putw.o puts.o refill.o \
        rewind.o rget.o scanf.o setbuf.o setbuffer.o setvbuf.o \
	stdio.o stdioLib.o stdioShow.o tmpnam.o tmpfile.o ungetc.o \
	wbuf.o wsetup.o vfprintf.o 

CC_INCLUDE =  -I.  -I../../../h  -I..

CC_DEFINES	= -DCPU=PENTIUM \
		  -DTOOL_FAMILY=gnu \
		  -DTOOL=gnu 

CFLAGS		= -march=pentium -ansi -g -gdwarf-2 -gstrict-dwarf -O2 -nostdlib -fno-builtin -fno-defer-pop -Wall $(CC_INCLUDE) $(CC_DEFINES)

.c.o :
	$(RM) $@
	cc $(CFLAGS) -c $<
###############################################################################build targets###############################################

libstdio.a : $(OBJS)
	$(RM) $@ 
	$(RM) *.dump
	$(RM) $@.nm
	ar -r $@ $(OBJS)
	nm $@ > $@.nm 






