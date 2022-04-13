#编译init进程:
gcc -m32 -Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers -I ../lib -I ../lib/user -I ../fs -o init.o init.c
ld -m elf_i386 -e main init.o ../build/string.o ../build/syscall.o ../build/stdio.o ../build/assert.o -o init
dd if=init of=/home/kdt/elephant/bochs/hd60M.img bs=512 count=15 seek=350 conv=notrunc