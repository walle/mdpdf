TARGET = mdpdf
LIBS = -lwkhtmltox -lhoedown -L ./ -L/usr/lib/x86_64-linux-gnu -lQtWebKit -lQtSvg -lQtXmlPatterns -lQtGui -lQtNetwork -lQtCore -lpthread -lstdc++
CC = gcc
CFLAGS=-g -O3 -Wall -Wextra -Wno-unused-parameter -fPIC -Ilib/wkhtmltopdf/include -Ilib/hoedown/src
LDFLAGS=-g -O3 -Wall -Werror
OBJLIBS	= libhoedown.a libwkhtmltox.a
PREFIX ?= /usr/local
BINPREFIX ?= "$(PREFIX)/bin"
MANPREFIX ?= "$(PREFIX)/share/man/man1"
VERSION="1.0.0"
DIST="$(TARGET)-$(VERSION)"
DIST_SRC="$(TARGET)-src-$(VERSION)"

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

libhoedown.a:
	cd lib/hoedown; make libhoedown.a
	cp lib/hoedown/libhoedown.a ./

libwkhtmltox.a:
	cd lib/wkhtmltopdf; qmake wkhtmltopdf.pro; make; cd src/lib; make staticlib;
	cp lib/wkhtmltopdf/bin/libwkhtmltox.a ./

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(OBJLIBS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean: clean-dist
	-rm -f *.o
	-rm -f *.a
	-rm -f $(TARGET)

clean-dist:
	-rm -rf $(DIST)
	-rm -f $(DIST).tar.gz
	-rm -rf $(DIST_SRC)
	-rm -f $(DIST_SRC).tar.gz

test: $(TARGET)
	./run-tests

install: $(TARGET)
	install -m 0755 $(TARGET) $(BINPREFIX)
	install man/mdpdf.1 $(MANPREFIX)

uninstall:
	rm -f $(BINPREFIX)/mdpdf
	rm -f $(MANPREFIX)/mdpdf.1

dist: $(TARGET)
	mkdir $(DIST)
	cp -r $(TARGET) LICENSE README.md man $(DIST)
	tar cfz $(DIST).tar.gz $(DIST)

dist-src:
	mkdir $(DIST_SRC)
	cp -r lib man LICENSE README.md html_data.h main.c run-tests Makefile $(DIST_SRC)
	tar cfz $(DIST_SRC).tar.gz $(DIST_SRC)

.PHONY: install uninstall dist dist-src clean clean-dist