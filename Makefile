CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -fsanitize=address -g -std=c99
CFILES=main.c scanner.c vm.c compiler.c

main clean:
	$(CC) $(CFLAGS) $(CFILES) -o main