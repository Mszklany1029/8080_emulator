EXE = invaders

CC = gcc

COMPILER_FLAGS = -Wall -Wextra -ggdb
LINKER_FLAGS = -lSDL2

$(EXE): Platform.o Machine.o Emulator.o
	$(CC) $(COMPILER_FLAGS) $(LINKER_FLAGS) Platform.o Machine.o Emulator.o -o $@

Platform.o: platform.c machine.h emulator.h
	$(CC) $(COMPILER_FLAGS) $(LINKER_FLAGS) -c platform.c -o $@

Machine.o: machine.c machine.h emulator.h
	$(CC) $(COMPILER_FLAGS) $(LINKER_FLAGS) -c machine.c -o $@

Emulator.o: emulator.c emulator.h
	$(CC) $(COMPILER_FLAGS) -c emulator.c -o $@
