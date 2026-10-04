# Makefile for qlumpy under POSIX using GCC

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../pvrstudiomdl/utils/common
LDFLAGS = -lm

COMMON_DIR = ../pvrstudiomdl/utils/common

SRC = \
	$(COMMON_DIR)/cmdlib.c \
	$(COMMON_DIR)/scriplib.c \
	$(COMMON_DIR)/wadlib.c \
	lbmlib.c \
	qlumpy.c \
	quakegrb.c

OBJ = cmdlib.o scriplib.o wadlib.o lbmlib.o qlumpy.o quakegrb.o

OUTDIR = ./
BINDIR = $(OUTDIR)

all: $(BINDIR)/qlumpy

$(BINDIR):
	@mkdir -p $(BINDIR)

$(BINDIR)/qlumpy: $(OBJ) | $(BINDIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# Rule for local source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rules for common shared source files
cmdlib.o: $(COMMON_DIR)/cmdlib.c
	$(CC) $(CFLAGS) -c $< -o $@

scriplib.o: $(COMMON_DIR)/scriplib.c
	$(CC) $(CFLAGS) -c $< -o $@

wadlib.o: $(COMMON_DIR)/wadlib.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(BINDIR)/qlumpy

.PHONY: all clean
