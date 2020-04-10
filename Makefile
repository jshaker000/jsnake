INSTALLDIR=/usr/local/bin

CXX     :=g++
CXXFLAGS:=-std=c++11 -Wall -Werror -Wextra -O3
LDFLAGS :=-lncurses

MAIN:=jsnake

.PHONY: run all install clean uninstall

run: all
	./${MAIN}

all: ${MAIN}

install: all 
	cp ${MAIN} ${INSTALLDIR}/${MAIN}

clean:
	rm -f ${MAIN} *.o

uninstall:
	rm -f ${INSTALLDIR}/${MAIN}

${MAIN}: % : ${MAIN}.cpp
	${CXX} -o $@ $^ ${CXXFLAGS} ${LDFLAGS}
