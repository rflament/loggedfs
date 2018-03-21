CC=g++
CFLAGS=-Wall -ansi -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -DELPP_SYSLOG -DELPP_NO_LOG_TO_FILE -DELPP_THREAD_SAFE -std=c++11 `xml2-config --cflags` 
LDFLAGS=-Wall -ansi -lpcre -lfuse `xml2-config --libs`
srcdir=src
easyloggingdir=vendor/github.com/muflihun/easyloggingpp/src
builddir=build

all: $(builddir) loggedfs

$(builddir):
	mkdir $(builddir)

loggedfs: $(builddir)/loggedfs.o $(builddir)/Config.o $(builddir)/Filter.o $(builddir)/easylogging.o
	$(CC) -o loggedfs $(builddir)/loggedfs.o $(builddir)/Config.o $(builddir)/Filter.o $(builddir)/easylogging.o $(LDFLAGS)

$(builddir)/loggedfs.o: $(builddir)/Config.o $(builddir)/Filter.o $(srcdir)/loggedfs.cpp
	$(CC) -o $(builddir)/loggedfs.o -c $(srcdir)/loggedfs.cpp $(CFLAGS) -Ivendor/github.com/muflihun/easyloggingpp/src

$(builddir)/Config.o: $(builddir)/Filter.o $(srcdir)/Config.cpp $(srcdir)/Config.h
	$(CC) -o $(builddir)/Config.o -c $(srcdir)/Config.cpp $(CFLAGS)

$(builddir)/Filter.o: $(srcdir)/Filter.cpp $(srcdir)/Filter.h
	$(CC) -o $(builddir)/Filter.o -c $(srcdir)/Filter.cpp $(CFLAGS)

$(builddir)/easylogging.o: $(easyloggingdir)/easylogging++.cc $(easyloggingdir)/easylogging++.h
	$(CC) -o $(builddir)/easylogging.o -c $(easyloggingdir)/easylogging++.cc $(CFLAGS)	

clean:
	rm -rf $(builddir)/
	
install:
	gzip --keep loggedfs.1
	cp loggedfs.1.gz /usr/share/man/man1/
	cp loggedfs /usr/bin/
	cp loggedfs.xml /etc/


mrproper: clean
	rm -rf loggedfs
			
release:
	tar -c --exclude="CVS" $(srcdir)/ loggedfs.xml LICENSE loggedfs.1.gz Makefile | bzip2 - > loggedfs.tar.bz2

