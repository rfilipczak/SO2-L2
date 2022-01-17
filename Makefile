CC = g++
CFLAGS = -std=c++17 -Wpedantic -Wall -Wextra -Wconversion -O2

SRC = ./main.cpp
INCLUDE = ./iohelp.h ./mythreading.h
EXEC = ./prog

all: prog

prog: $(SRC) $(INCLUDE)
	$(CC) $(CFLAGS) $(SRC) -pthread -o $(EXEC)

clean:
	rm -rf $(EXEC)

.PHONY: all clean

