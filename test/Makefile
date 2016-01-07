include ../Makefile.config

rempi_test_master_worker_SRCS = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_worker_OBJS = $(rempi_test_master_worker_SRCS:%.c=%.o)
rempi_test_master_worker_DEPS = $(rempi_test_master_worker_SRCS:%.c=%.d)
rempi_test_master_worker_PROGRAM = rempi_test_master_worker

rempi_test_isendrecv_SRCS = rempi_test_isendrecv.c rempi_test_util.c
rempi_test_isendrecv_OBJS =  $(rempi_test_isendrecv_SRCS:%.c=%.o)
rempi_test_isendrecv_DEPS =  $(rempi_test_isendrecv_SRCS:%.c=%.d)
rempi_test_isendrecv_PROGRAM = rempi_test_isendrecv

rempi_test_neighbor_SRCS = rempi_test_neighbor.c rempi_test_util.c
rempi_test_neighbor_OBJS =  $(rempi_test_neighbor_SRCS:%.c=%.o)
rempi_test_neighbor_DEPS =  $(rempi_test_neighbor_SRCS:%.c=%.d)
rempi_test_neighbor_PROGRAM = rempi_test_neighbor

rempi_test_neighbor-send_SRCS = rempi_test_neighbor-send.c rempi_test_util.c
rempi_test_neighbor-send_OBJS =  $(rempi_test_neighbor-send_SRCS:%.c=%.o)
rempi_test_neighbor-send_DEPS =  $(rempi_test_neighbor-send_SRCS:%.c=%.d)
rempi_test_neighbor-send_PROGRAM = rempi_test_neighbor-send

rempi_test_neighbor-tree_SRCS = rempi_test_neighbor-tree.c rempi_test_util.c
rempi_test_neighbor-tree_OBJS =  $(rempi_test_neighbor-tree_SRCS:%.c=%.o)
rempi_test_neighbor-tree_DEPS =  $(rempi_test_neighbor-tree_SRCS:%.c=%.d)
rempi_test_neighbor-tree_PROGRAM = rempi_test_neighbor-tree

rempi_test_neighbor-collective_SRCS = rempi_test_neighbor-collective.c rempi_test_util.c
rempi_test_neighbor-collective_OBJS =  $(rempi_test_neighbor-collective_SRCS:%.c=%.o)
rempi_test_neighbor-collective_DEPS =  $(rempi_test_neighbor-collective_SRCS:%.c=%.d)
rempi_test_neighbor-collective_PROGRAM = rempi_test_neighbor-collective


# rempi_test_clmpi_SRCS = rempi_test_clmpi.c rempi_test_util.c
# rempi_test_clmpi_OBJS =  $(rempi_test_clmpi_SRCS:%.c=%.o)
# rempi_test_clmpi_DEPS =  $(rempi_test_clmpi_SRCS:%.c=%.d)
# rempi_test_clmpi_PROGRAM = rempi_test_clmpi

rempi_test_compress_SRCS = rempi_test_compress.cpp rempi_test_util.c ../src/rempi_compression_utilcpp
rempi_test_compress_OBJS = rempi_test_compress.o rempi_test_util.o
rempi_test_compress_DEPS = rempi_test_compress.d rempi_test_util.d
rempi_test_compress_PROGRAM = rempi_test_compress

PROGRAMS= $(rempi_test_master_worker_PROGRAM) \
	  $(rempi_test_isendrecv_PROGRAM) \
	  $(rempi_test_neighbor_PROGRAM) \
	  $(rempi_test_neighbor-send_PROGRAM) \
	  $(rempi_test_neighbor-tree_PROGRAM) \
	  $(rempi_test_neighbor-collective_PROGRAM) 

OBJS= $(rempi_test_master_worker_OBJS) \
      $(rempi_test_isendrecv_OBJS) \
      $(rempi_test_neighbor_OBJS) \
      $(rempi_test_neighbor-send_OBJS) \
      $(rempi_test_neighbor-tree_OBJS) \
      $(rempi_test_neighbor-collective_OBJS) 

DEPS= $(rempi_test_master_worker_DEPS) \
      $(rempi_test_isendrecv_DEPS) \
      $(rempi_test_neighbor_DEPS) \
      $(rempi_test_neighbor-send_DEPS) \
      $(rempi_test_neighbor-tree_DEPS) \
      $(rempi_test_neighbor-collective_DEPS) 


#$@: target name
#$<: first dependent file
#$^: all indemendent files

all: src_make $(PROGRAMS) 

-include $(DEPS)

src_make:
	@$(MAKE) -C ../src

install: all
	@$(MAKE) -C ../src install

$(rempi_test_master_worker_PROGRAM): $(rempi_test_master_worker_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_isendrecv_PROGRAM): $(rempi_test_isendrecv_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor_PROGRAM): $(rempi_test_neighbor_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor-send_PROGRAM): $(rempi_test_neighbor-send_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor-collective_PROGRAM): $(rempi_test_neighbor-collective_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_neighbor-tree_PROGRAM): $(rempi_test_neighbor-tree_OBJS)
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
cleanc:
	-rm -f *.core
	-rm -f *.btr


.PHONY: clean_record
clean_record:
	-rm -f rank_*.rempi

.PHONY: clean_all
clean_all: clean clean_core clean_record clean_btr


# DO NOT DELETE
