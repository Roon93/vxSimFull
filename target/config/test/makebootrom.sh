rm -f bootrom_uncmp.elf
rm -f *.o
rm -f bootrom_uncmp.*
make -f Makefile_wxy.mak bootrom_uncmp.iso
