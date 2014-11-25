include ./Makefile.config

rempi_OBJS = rempi.o rempi_record_replay.o rempi_record.o rempi_replay.o rempi_event_list.o rempi_thread.o rempi_record_thread.o rempi_mutex.o

rempi_test_OBJS = rempi_test.o $(rempi_OBJS)
rempi_test_PROGRAM = rempi_test

rempi_test_native_OBJS = rempi_test.o
rempi_test_native_PROGRAM = rempi_test_native

PROGRAMS= $(rempi_test_PROGRAM)  $(rempi_test_native_PROGRAM)
OBJS= $(rempi_test_OBJS)   $(rempi_test_native_OBJS)


.SUFFIXES: .c .o

all: $(PROGRAMS) 

library: $(rempi_OBJS)
	ar cr librempi.a $(rempi_OBJS)
	ranlib librempi.a

lib.so: $(rempi_OBJS)
	$(CC) -shared -o librempi.so $(rempi_OBJS)

$(rempi_test_PROGRAM): $(rempi_test_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(rempi_test_native_PROGRAM): $(rempi_test_native_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<



.PHONY: clean
clean:
	-rm -rf $(PROGRAMS) $(OBJS)

clean_core:
	-rm -rf *.core

