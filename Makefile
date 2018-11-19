CXX  := g++ 
PROG := exj_parser
OPTFLAGS  := -O3
CFLAGS   += -Wall -o $(PROG) $(OPTFLAGS) 
CPP := parser.cpp \

all:
	$(CXX) $(CFLAGS) $(CPP)
install:
	cp ${PROG} /usr/local/bin
clean: 
	rm -f *.o $(PROG)
