# Windows Makefile for OpenMP C programs (MSYS2/MinGW)
# Usage:
#   - Build all .c files into .exe:   make
#   - Clean generated executables:     make clean
#
# Mirrors the macOS Makefile behavior: one executable per .c file.

CC       = gcc
CFLAGS   =-fopenmp
LDFLAGS  =

EXEEXT   = .exe
SOURCES  = $(wildcard *.c)
TARGETS  = $(SOURCES:.c=$(EXEEXT))

all: $(TARGETS)

%$(EXEEXT): %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	-$(RM) $(TARGETS)
