TARGET = mdpdf
LIBS = -lwkhtmltox -L/lib -lhoedown -L../hoedown
CC = gcc
CFLAGS=-g -O3 -Wall -Wextra -Wno-unused-parameter -fPIC -ggdb -I/include -I../hoedown/src
LDFLAGS=-g -O3 -Wall -Werror

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)