
CXX=mpicxx

all: test.o
	$(CXX) -o test test.o -lrempi -L/Users/lagunaperalt1/projects/PRUNER/ReMPI/code/src

test.o: test.C
	$(CXX) -c test.C

clean:
	rm -f *.o test
