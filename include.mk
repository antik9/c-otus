CC=gcc
CFLAGS=-c -Wall -Wextra -Wpedantic -std=c11
LFLAGS=-Wall

SOURCES=$(wildcard *.c)
INCLUDES=$(wildcard *.h)

OBJDIR := obj
OBJECTS := $(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE) .gitignore

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

.gitignore:
	@echo $(EXECUTABLE) > .gitignore

clean:
	rm -rf *.o $(EXECUTABLE)
