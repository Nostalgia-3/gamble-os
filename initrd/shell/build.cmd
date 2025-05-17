@echo off

mkdir build

i686-elf-gcc -I include -O2 -m32 -fno-pie -nostdlib -ffreestanding -Wall -Werror -c src/main.c -o build/main.o
i686-elf-gcc -I include -O2 -m32 -fno-pie -nostdlib -ffreestanding -Wall -Werror -c src/printf.c -o build/printf.o

nasm -felf32 src/gstd.asm -o build/gstd.o

i686-elf-gcc -I include -O2 -m32 -fno-pie -nostdlib -ffreestanding -Wall -Werror -z noexecstack -T linker.ld build/main.o build/printf.o -o ../init

@echo on