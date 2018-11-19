CXX  := g++ 
PROG := exj_parser
OPTFLAGS  := -O3
CFLAGS   += -Wall -o $(PROG) $(OPTFLAGS) 
CPP := parser.cpp \

all:
	$(CXX) $(CFLAGS) $(CPP)
clean: 
	rm -f *.o $(PROG)
