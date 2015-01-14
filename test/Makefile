include ../Makefile.config

rempi_test_master_worker_SRCS = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_worker_OBJS = $(rempi_test_master_worker_SRCS:%.c=%.o)
rempi_test_master_worker_DEPS = $(rempi_test_master_worker_SRCS:%.c=%.d)
rempi_test_master_worker_PROGRAM = rempi_test_master_worker

rempi_test_neighbor_SRCS = rempi_test_neighbor.c rempi_test_util.c
rempi_test_neighbor_OBJS =  $(rempi_test_neighbor_SRCS:%.c=%.o)
rempi_test_neighbor_DEPS =  $(rempi_test_neighbor_SRCS:%.c=%.d)
rempi_test_neighbor_PROGRAM = rempi_test_neighbor

rempi_test_compress_SRCS = rempi_test_compress.cpp rempi_test_util.c
rempi_test_compress_OBJS = rempi_test_compress.o rempi_test_util.o
rempi_test_compress_DEPS = rempi_test_compress.d rempi_test_util.d
rempi_test_compress_PROGRAM = rempi_test_compress

PROGRAMS= $(rempi_test_master_worker_PROGRAM)  $(rempi_test_neighbor_PROGRAM)  $(rempi_test_compress_PROGRAM)
OBJS= $(rempi_test_master_worker_OBJS) $(rempi_test_neighbor_OBJS)  $(rempi_test_compress_OBJS)
DEPS= $(rempi_test_master_worker_DEPS) $(rempi_test_neighbor_DEPS)  $(rempi_test_compress_DEPS)

#$@: target name
#$<: first dependent file
#$^: all indemendent files

all: src_make $(PROGRAMS) 

-include $(DEPS)

src_make:
	@$(MAKE) -C ../src

$(rempi_test_master_worker_PROGRAM): $(rempi_test_master_worker_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor_PROGRAM): $(rempi_test_neighbor_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_compress_PROGRAM): $(rempi_test_compress_OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) -L/g/g90/sato5/repo/rempi/lib -lrempi -o $@ $^

.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -c -MMD -MP $<
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 

.SUFFIXES: .cpp .o
.cpp.o: 
	$(CC) $(CXXFLAGS) $(LDFLAGS) -c -MMD -MP $<
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ -c $< 


.PHONY: clean 
clean:
	@$(MAKE) clean -C ../src
	-rm -f $(PROGRAMS) $(OBJS) $(DEPS)

.PHONY: clean_core
clean_core:
	-rm -f *.core

.PHONY: clean_btr
clean_btr:
	-rm -f *.btr

.PHONY: clean_record
clean_record:
	-rm -f rank_*.rempi

.PHONY: clean_all
clean_all: clean clean_core clean_record clean_btr


# DO NOT DELETE
