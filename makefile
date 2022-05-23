CC = gcc
FLAGS = -Wall
LIBS = -pthread
OBJS1 = main.o shared_memory.o stock_server.o admin_console.o
OBJS2 = operations_terminal.o
PROG1 = stock_server
PROG2 = operations_terminal

# GENERIC

all: ${PROG1} ${PROG2}

clean:
		rm ${OBJS1} ${OBJS2} ${PROG1} ${PROG2}

${PROG1}: ${OBJS1}
		${CC} ${FLAGS} ${LIBS} ${OBJS1} -o $@

${PROG2}: ${OBJS2}
		${CC} ${FLAGS} ${OBJS2} -o $@

.c.o:
		${CC} ${FLAGS} $< -c
	
	
###############################################

main.o: main.c admin_console.h shared_memory.h

admin_console.o: admin_console.c admin_console.h shared_memory.h

stock_server.o: stock_server.c stock_server.h shared_memory.h

shared_memory.o: shared_memory.c shared_memory.h

operations_terminal.o: operations_terminal.c operations_terminal.h shared_memory.h
