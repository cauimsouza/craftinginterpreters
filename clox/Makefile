VPATH=src

CC=gcc
CFLAGS=#-DDEBUG_PRINT_CODE -DDEBUG_STRESS_GC
LIBS=-lm

clox: main.c chunk.c memory.c debug.c value.c lines.c vm.c compiler.c scanner.c object.c table.c
	$(CC) $(LIBS)  -o $@ $^ $(CFLAGS)

