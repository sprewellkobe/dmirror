#updated by kobe, 2007,4,25
#---------------------------------------------------------------------------------------------
OUTPUTFILES=dmirror
CXXFLAGS=-I/usr/include/ -I./ -DMYDEBUG
LIBS=-L/usr/lib64/libevent -lpthread -levent -lcurl
CXX=g++ -g -Wall -O2 -fno-strict-aliasing #-DNDEBUG 
CC=gcc -g -Wall -O2 -fno-strict-aliasing -DNDEBUG
#---------------------------------------------------------------------------------------------

all: $(OUTPUTFILES)

.SUFFIXES: .o .cpp .hpp
#---------------------------------------------------------------------------------------------

dmirror: main.o ae.o Common.o MyCurl.o IniParser.o HTTPProtocol.o UnixSocketProtocol.o Watcher.o Sender.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) -levent

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
