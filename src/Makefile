include ../Makefile.config

PROG		= librempi.so
SRC_FILES	= Wrappers.h

O_FILES		= $(SRC_FILES:%.C=%.o)

all: $(PROG)

$(PROG): $(O_FILES)
	$(CXX) -shared -o $(PROG) $(O_FILES)

clean:
	$(RM) *.o $(PROG)
