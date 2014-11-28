TARGET = mdpdf
LIBS = -lwkhtmltox -lhoedown -L ./ -L/usr/lib/x86_64-linux-gnu -lQtWebKit -lQtSvg -lQtXmlPatterns -lQtGui -lQtNetwork -lQtCore -lpthread -lstdc++
CC = gcc
CFLAGS=-g -O3 -Wall -Wextra -Wno-unused-parameter -fPIC -Ilib/wkhtmltopdf/include -Ilib/hoedown
LDFLAGS=-g -O3 -Wall -Werror
OBJLIBS	= libhoedown.a libwkhtmltox.a

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

libhoedown.a:
	cd lib/hoedown; make

lib/wkhtmltopdf:
	cd lib; git clone --recursive https://github.com/wkhtmltopdf/wkhtmltopdf.git

libwkhtmltox.a: lib/wkhtmltopdf
	cd lib/wkhtmltopdf; qmake wkhtmltopdf.pro; cd src/lib; make staticlib;
	cp lib/wkhtmltopdf/bin/libwkhtmltox.a ./

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(OBJLIBS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f *.a
	-rm -f $(TARGET)
