include ../Makefile.config

rempi_test_master_worker_OBJS = rempi_test_master_worker.o rempi_test_util.o
rempi_test_master_worker_PROGRAM = rempi_test_master_worker

rempi_test_neighbor_OBJS = rempi_test_neighbor.o rempi_test_util.o
rempi_test_neighbor_PROGRAM = rempi_test_neighbor

PROGRAMS= $(rempi_test_master_worker_PROGRAM)  $(rempi_test_neighbor_PROGRAM)
OBJS= $(rempi_test_master_worker_OBJS)   $(rempi_test_neighbor_OBJS)

#$@: target name
#$<: first dependent file
#$^: all indemendent files

all: src_make $(PROGRAMS) 

src_make:
	@$(MAKE) -C ../src

$(rempi_test_master_worker_PROGRAM): $(rempi_test_master_worker_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor_PROGRAM): $(rempi_test_neighbor_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 

#.SUFFIXES: .cpp .o
#.cpp.o: 
#	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ -c $< 


.PHONY: clean
clean:
	-rm -f $(PROGRAMS) $(OBJS)

clean_core:
	-rm -f *.core

clean_record:
	-rm -f rank_*.rempi

