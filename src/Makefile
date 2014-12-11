include ../Makefile.config

rempi_OBJS = 	$(SRC_DIR)/rempi.o \
		$(SRC_DIR)/rempi_err.o \
		$(SRC_DIR)/rempi_mem.o \
		$(SRC_DIR)/rempi_util.o \
		$(SRC_DIR)/rempi_config.o \
                $(SRC_DIR)/rempi_record_replay.o \
		$(SRC_DIR)/rempi_record.o \
		$(SRC_DIR)/rempi_event_list.o \
	     	$(SRC_DIR)/rempi_event.o \
		$(SRC_DIR)/rempi_encoder.o \
		$(SRC_DIR)/rempi_thread.o \
		$(SRC_DIR)/rempi_io_thread.o \
		$(SRC_DIR)/rempi_mutex.o

OBJS = $(rempi_OBJS)

#$@: target name
#$<: first dependent file
#$^: all indemendent files

all: lib.a lib.so

lib.a: $(rempi_OBJS)
	ar cr ../lib/librempi.a $(rempi_OBJS)
	ranlib ../lib/librempi.a

lib.so: $(rempi_OBJS)
	$(CC) -shared -o ../lib/librempi.so $(rempi_OBJS)

.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 

.SUFFIXES: .cpp .o
.cpp.o: 
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ -c $< 


.PHONY: clean
clean:
	-rm -rf $(OBJS) ../lib/librempi.a ../lib/librempi.so

clean_core:
	-rm -rf *.core

