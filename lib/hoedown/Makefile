CFLAGS = -g -O3 -ansi -pedantic -Wall -Wextra -Wno-unused-parameter -Isrc

ifneq ($(OS),Windows_NT)
	CFLAGS += -fPIC
endif

HOEDOWN_SRC=\
	autolink.o \
	buffer.o \
	document.o \
	escape.o \
	html.o \
	html_blocks.o \
	html_smartypants.o \
	stack.o \
	version.o

.PHONY:		all

all:	../../libhoedown.a

# Libraries
../../libhoedown.a: $(HOEDOWN_SRC)
	$(AR) rcs ../../libhoedown.a $^

# Housekeeping

clean:
	$(RM) *.o
	$(RM) libhoedown.a

# Generic object compilations

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

html_blocks.o: html_blocks.c
	$(CC) $(CFLAGS) -Wno-static-in-inline -c -o $@ $<
