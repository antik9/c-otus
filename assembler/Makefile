AS = nasm  # NOTE: yasm тоже подойдёт

all: asm-prog asm-optimized main

asm-prog: main.o
	$(CC) -no-pie $^ -o $@

main.o: main.asm
	$(AS) -felf64 $^

asm-optimized: optimized.o
	$(CC) -no-pie $^ -o $@

optimized.o: optimized.asm
	$(AS) -felf64 $^

main: main.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 main.c -o main

clean:
	rm -f asm-prog asm-optimized main *.o core

PHONY: clean
