main: main.o shared_memory.o
	gcc main.o shared_memory.o -pthread -o main

shared_memory: shared_memory.c shared_memory.h
	gcc -c -pthread shared_memory.c