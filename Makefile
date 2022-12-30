CXX?=g++
CXXFLAGS+=-Wall -ansi -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -DELPP_SYSLOG -DELPP_NO_DEFAULT_LOG_FILE -DELPP_THREAD_SAFE -std=c++11 `xml2-config --cflags`
LDFLAGS+=-Wall -ansi -lpcre2-8 -lfuse `xml2-config --libs` -lpthread
srcdir=src
easyloggingdir=vendor/github.com/muflihun/easyloggingpp/src
builddir=build

all: $(builddir) loggedfs

$(builddir):
	mkdir $(builddir)

loggedfs: $(builddir)/loggedfs.o $(builddir)/Config.o $(builddir)/Filter.o $(builddir)/easylogging.o
	$(CXX) $(CPPFLAGS) -o loggedfs $(builddir)/loggedfs.o $(builddir)/Config.o $(builddir)/Filter.o $(builddir)/easylogging.o $(LDFLAGS)

$(builddir)/loggedfs.o: $(builddir)/Config.o $(builddir)/Filter.o $(srcdir)/loggedfs.cpp
	$(CXX) $(CPPFLAGS) -o $(builddir)/loggedfs.o -c $(srcdir)/loggedfs.cpp $(CXXFLAGS) -I$(easyloggingdir)

$(builddir)/Config.o: $(builddir)/Filter.o $(srcdir)/Config.cpp $(srcdir)/Config.h
	$(CXX) $(CPPFLAGS) -o $(builddir)/Config.o -c $(srcdir)/Config.cpp $(CXXFLAGS)

$(builddir)/Filter.o: $(srcdir)/Filter.cpp $(srcdir)/Filter.h
	$(CXX) $(CPPFLAGS) -o $(builddir)/Filter.o -c $(srcdir)/Filter.cpp $(CXXFLAGS)

$(builddir)/easylogging.o: $(easyloggingdir)/easylogging++.cc $(easyloggingdir)/easylogging++.h
	$(CXX) $(CPPFLAGS) -o $(builddir)/easylogging.o -c $(easyloggingdir)/easylogging++.cc $(CXXFLAGS)

clean:
	rm -rf $(builddir)/

install:
	mkdir -p $(DESTDIR)/usr/share/man/man1 $(DESTDIR)/usr/bin $(DESTDIR)/etc
	gzip < loggedfs.1 > $(DESTDIR)/usr/share/man/man1/loggedfs.1.gz
	cp loggedfs $(DESTDIR)/usr/bin/
	cp loggedfs.xml $(DESTDIR)/etc/


mrproper: clean
	rm -rf loggedfs

release:
	tar -c --exclude="CVS" $(srcdir)/ loggedfs.xml LICENSE loggedfs.1.gz Makefile | bzip2 - > loggedfs.tar.bz2
