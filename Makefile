TARGET = mdpdf
LIBS = -lwkhtmltox -L/lib -lhoedown -L ./
CC = gcc
CFLAGS=-g -O3 -Wall -Wextra -Wno-unused-parameter -fPIC -ggdb -I/include -Ilib/hoedown
LDFLAGS=-g -O3 -Wall -Werror
OBJLIBS	= libhoedown.a

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

libhoedown.a: force_look
	cd lib/hoedown; $(MAKE) $(MFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(OBJLIBS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

force_look:
	true