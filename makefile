main: main.o shared_memory.o admin_console.o
	gcc -Wall -pthread main.o admin_console.o shared_memory.o -o main

main.o: main.c admin_console.h shared_memory.h
	gcc -Wall -c main.c

admin_console.o: admin_console.c admin_console.h shared_memory.h
	gcc -Wall -c admin_console.c

shared_memory.o: shared_memory.c shared_memory.h
	gcc -Wall -c shared_memory.c
