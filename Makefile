CPP=g++
CPPFLAGS=-std=c++11
LDFLAGS=-lncurses

jsnake: jsnake.cpp
	$(CPP)  jsnake.cpp $(CPPFLAGS) $(LDFLAGS) -o jsnake
