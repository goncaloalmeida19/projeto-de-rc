CC = gcc
FLAGS = -Wall
LIBS = -pthread
OBJS = main.o shared_memory.o admin_console.o
PROG = stock_server

# GENERIC

all: ${PROG}

clean:
		rm ${OBJS} ${PROG}
		
${PROG}: ${OBJS}
		${CC} ${FLAGS} ${LIBS} ${OBJS} -o $@

.c.o:
		${CC} ${FLAGS} $< -c
	
	
###############################################

main.o: main.c admin_console.h shared_memory.h

admin_console.o: admin_console.c admin_console.h shared_memory.h

shared_memory.o: shared_memory.c shared_memory.h
