INSTALLDIR=/usr/local/bin

CXX=g++
CXXFLAGS=-std=c++11 -O2
LDFLAGS=-lncurses

MAIN=jsnake


$(MAIN): $(MAIN).cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

install: $(MAIN)
	cp $(MAIN) $(INSTALLDIR)/$(MAIN)

clean:
	rm -f $(MAIN) *.o

uninstall:
	rm -f $(INSTALLDIR)/$(MAIN)
