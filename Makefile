CPP=g++
CPPFLAGS=-std=c++11
LDFLAGS=-lncurses

jsnake: jsnake.cpp
	$(CPP)  jsnake.cpp $(CPPFLAGS) $(LDFLAGS) -o jsnake

install: 
	cp jsnake /usr/local/bin/jsnake

clean:
	rm -f jsnake *.o

uninstall:
	rm -f /usr/local/bin/jsnake