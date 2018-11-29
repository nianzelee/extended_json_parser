CXX  := g++ 
PROG := bin/exj_parser
OPTFLAGS  := -O3
CFLAGS   += -Wall -o $(PROG) $(OPTFLAGS) 
CPP := src/parser.cpp \

all:
	$(CXX) $(CFLAGS) $(CPP)
install:
	cp ${PROG} /usr/local/bin
clean: 
	rm -f $(PROG)
