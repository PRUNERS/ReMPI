include ../Makefile.config

SRC_DIR=.
SUB_DIR_ENC=

rempi_SRCS =    $(SRC_DIR)/rempi.cpp \
		$(SRC_DIR)/rempi_err.cpp \
		$(SRC_DIR)/rempi_mem.cpp \
		$(SRC_DIR)/rempi_util.cpp \
		$(SRC_DIR)/rempi_compression_util.cpp \
		$(SRC_DIR)/rempi_config.cpp \
                $(SRC_DIR)/rempi_re.cpp \
	        $(SRC_DIR)/rempi_re_cdc.cpp \
	        $(SRC_DIR)/rempi_sig_handler.cpp \
	        $(SRC_DIR)/rempi_status.cpp \
		$(SRC_DIR)/rempi_recorder_cdc.cpp \
		$(SRC_DIR)/rempi_recorder.cpp \
		$(SRC_DIR)/rempi_message_manager.cpp \
		$(SRC_DIR)/rempi_request_mg.cpp \
		$(SRC_DIR)/rempi_event_list.cpp \
	     	$(SRC_DIR)/rempi_event.cpp \
	     	$(SRC_DIR)/rempi_clock_delta_compression.cpp \
		$(SRC_DIR)/rempi_encoder.cpp \
		$(SRC_DIR)/rempi_encoder_cdc.cpp \
		$(SRC_DIR)/rempi_encoder_cdc_row_wise_diff.cpp \
		$(SRC_DIR)/rempi_encoder_cdc_permutation_diff.cpp \
		$(SRC_DIR)/rempi_encoder_rep.cpp \
		$(SRC_DIR)/rempi_encoder_zlib.cpp \
		$(SRC_DIR)/rempi_encoder_simple_zlib.cpp \
		$(SRC_DIR)/rempi_thread.cpp \
		$(SRC_DIR)/rempi_io_thread.cpp \
		$(SRC_DIR)/rempi_mutex.cpp
#                $(SRC_DIR)/rempi_re_no_comp.cpp \

rempi_OBJS = 	$(rempi_SRCS:%.cpp=%.o) 
rempi_DEPS = 	$(rempi_SRCS:%.cpp=%.d) 

rempi_o_LIBS =	./librempi.o 
rempi_a_LIBS =	../lib/librempi.a 
rempi_so_LIBS =	../lib/librempi.so	
rempilite_a_LIBS =	../lib/librempilite.a 
rempilite_so_LIBS =	../lib/librempilite.so	

CLMPI_DIR  =  ./external/clmpi/
clmpi_OBJS =  $(CLMPI_DIR)/libclmpi.o

rcmpi_so_LIBS=../lib/librcmpi.so	



OBJS = $(rempi_OBJS)
DEPS = $(rempi_DEPS)
LIBS = $(rempilite_so_LIBS) $(rempilite_a_LIBS) $(rempi_so_LIBS) $(rempi_a_LIBS) $(rcmpi_so_LIBS)

#$@: target name
#$<: first dependent file
#$^: all indemendent files


all:  $(LIBS)

$(clmpi_OBJS):
	@$(MAKE) -C ./external all

-include $(DEPS)

$(rempi_a_LIBS):  $(rempi_OBJS)  $(clmpi_OBJS) #$(rempilite_a_LIBS)
	ld    -o $(rempi_o_LIBS) -r $(rempi_OBJS)
	ws rempi.tmp.o 0 $(rempi_o_LIBS) $(clmpi_OBJS)
#	ar cr $@ $(rempi_o_LIBS) $(clmpi_OBJS)
	ar cr $@ rempi.tmp.o
	ranlib $@

#$(rempilite_a_LIBS):  $(rempi_OBJS)  $(clmpi_OBJS)
#	ld    -o $(rempi_o_LIBS) -r $(rempi_OBJS)
#	ws rempilite.tmp.o 1 $(rempi_o_LIBS) $(clmpi_OBJS)
#	ar cr $@ rempilite.tmp.o
#	ranlib $@

$(rempilite_a_LIBS):  $(rempi_OBJS)  $(clmpi_OBJS)
	ld    -o $(rempi_o_LIBS) -r $(rempi_OBJS)
	ar cr $@ librempi.o
	ranlib $@

$(rempi_so_LIBS): $(rempi_a_LIBS) $(rempi_OBJS) #$(rempilite_so_LIBS)
#	$(CC) -shared -o $@ $(rempi_o_LIBS)  $(clmpi_OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -g -shared -o $@ rempi.tmp.o

#$(rempilite_so_LIBS): $(rempilite_a_LIBS) $(rempi_OBJS)
#	$(CC) $(LDFLAGS) $(LDLIBS) -g -shared -o $@ rempilite.tmp.o

$(rempilite_so_LIBS): $(rempilite_a_LIBS) $(rempi_OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -g -shared -o $@ librempi.o



$(rcmpi_so_LIBS):  $(SRC_DIR)/rempi_message_manager.o
	$(CC) -shared -o $(rcmpi_so_LIBS)  $(SRC_DIR)/rempi_message_manager.o


.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS) -c -MMD -MP $< 
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS) -o $@ -c $< 

install: all
	@$(MAKE) -C ./external install
#	pnmpi-patch ../lib/librempi.so /g/g90/sato5/opt/lib/pnmpi-modules/librempi.so
	cp ../lib/librempi.so /g/g90/sato5/opt/lib/librempi.so
	cp ../lib/librempilite.so /g/g90/sato5/opt/lib/librempilite.so


.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS) -c -MMD -MP $<
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS) -o $@ -c $< 

# .SUFFIXES: .cpp .d
# .cpp.d:
# 	$(CC) $(CXXFLAGS) $(LDFLAGS) -c -MMD -MP $<


.PHONY: clean
clean:
	-rm -rf $(OBJS) $(DEPS) $(LIBS) $(rempi_o_LIBS)
	@$(MAKE) -C ./external clean

.PHONY: clean_core
clean_core:
	-rm -rf *.core

