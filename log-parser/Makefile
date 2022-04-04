EXECUTABLE=log-parser

include ../include.mk

SOURCES=$(wildcard *.c) ../hash-table/ht.c
OBJECTS := $(SOURCES:.c=.o)

LFLAGS=-Wall -pthread

hash-table:
	@cd ../hash-table && make ht.o
