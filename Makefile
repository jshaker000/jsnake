CXX=g++
CXXFLAGS=-std=c++11 -O2
LDFLAGS=-lncurses

jsnake: jsnake.cpp
	$(CXX)  jsnake.cpp $(CXXFLAGS) $(LDFLAGS) -o jsnake

install: jsnake 
	cp jsnake /usr/local/bin/jsnake

clean:
	rm -f jsnake *.o

uninstall:
	rm -f /usr/local/bin/jsnake
