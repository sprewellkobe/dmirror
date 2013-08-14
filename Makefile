#updated by kobe, 2007,4,25
#---------------------------------------------------------------------------------------------
OUTPUTFILES=dmirror
BUILDVERSION=$(shell date +%Y%m%d)
CXXFLAGS=-I/usr/include/ -I./ -DMYDEBUG -DBUILDVERSION=$(BUILDVERSION)
LIBS=-L/usr/lib64/libevent -lpthread -lcurl
CXX=g++ -g -Wall -O2 -fno-strict-aliasing 
CC=gcc -g -Wall -O2 -fno-strict-aliasing
#---------------------------------------------------------------------------------------------

all: $(OUTPUTFILES)

.SUFFIXES: .o .cpp .hpp
#---------------------------------------------------------------------------------------------

dmirror: main.o ae.o Common.o MyCurl.o IniParser.o HTTPProtocol.o UnixSocketProtocol.o Watcher.o Sender.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CXXFLAGS) -c $<
#---------------------------------------------------------------------------------------------
install:
	#mkdir -p $(DESTDIR)/usr/local/sae/
	#install -D rdc $(DESTDIR)/usr/local/sae/
clean:
	wc -l *.cpp *.hpp
	rm -rf $(OUTPUTFILES) *.o *.so *.a *~
