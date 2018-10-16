CXX=g++
CXXFLAGS=-std=c++11 -O2
LDFLAGS=-lncurses

jsnake: jsnake.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

install: jsnake 
	cp jsnake /usr/local/bin/jsnake

clean:
	rm -f jsnake *.o

uninstall:
	rm -f /usr/local/bin/jsnake
