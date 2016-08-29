
if ENABLE_CDC
SUBDIRS = ./external/
endif

AM_CFLAGS = -fPIC -g -Wall -pthread -std=c++0x -I./external/clmpi -lstdc++ -I$(libz_a_INCLUDE)
AM_CXXFLAGS = $(AM_CFLAGS)

rempi_SOURCES =  \
		rempi_pmpi.cpp \
		rempi_err.cpp \
		rempi_mem.cpp \
		rempi_util.cpp \
		rempi_compression_util.cpp \
		rempi_config.cpp \
                rempi_re.cpp \
		rempi_re_cdc.cpp \
	        rempi_sig_handler.cpp \
	        rempi_status.cpp \
	        rempi_mf.cpp \
		rempi_recorder_cdc.cpp \
		rempi_recorder.cpp \
		rempi_message_manager.cpp \
		rempi_mpi_init.cpp \
		rempi_request_mg.cpp \
		rempi_event_list.cpp \
	     	rempi_event.cpp \
		rempi_cp.cpp \
	     	rempi_clock_delta_compression.cpp \
		rempi_encoder.cpp \
		rempi_encoder_basic.cpp \
		rempi_encoder_cdc.cpp \
		rempi_encoder_cdc_row_wise_diff.cpp \
		rempi_encoder_cdc_permutation_diff.cpp \
		rempi_encoder_zlib.cpp \
		rempi_encoder_simple_zlib.cpp \
		rempi_thread.cpp \
		rempi_io_thread.cpp \
		rempi_mutex.cpp

lib_LTLIBRARIES = librempi.la
librempi_la_SOURCES  = $(rempi_SOURCES)
librempi_la_LIBADD =  $(libz_a_LIB)
librempi_la_CFLaGS = -lz -lstdc++ -lintlc
librempi_la_CPPFLAGS = -DREMPI_LITE

if ENABLE_CDC
librempix_a_LIB = librempix.a
lib_LIBRARIES = $(librempix_a_LIB)
librempix_so_LIB = librempix.so
librempix_a_SOURCES = $(rempi_SOURCES)
#librempix_a_OBJECTS = $(librempix_a_SOURCES:%.cpp=%.o) <= automake will create this variable
librempix_a_CFLAGS = -lz -lstdc++ -lintlc
librempix_a_LD_OBJECT = librempix.ar.o
librempix_a_STACK_PMPI_OBJECT = librempix.stack_pmpi.o
clmpi_o_OBJECT = $(SUBDIRS)/clmpi/libclmpi_stack_pmpi.o
workaround_libdir = $(libdir)
dist_workaround_lib_SCRIPTS = $(librempix_so_LIB)


$(librempix_a_LIB): $(librempix_a_OBJECTS) $(clmpi_o_OBJECT)
	ld -o $(librempix_a_LD_OBJECT) -r $(librempix_a_OBJECTS) $(libz_a_LIB)
	../scripts/external/pmpis/stack_pmpi $(librempix_a_STACK_PMPI_OBJECT) 0 $(librempix_a_LD_OBJECT) $(clmpi_o_OBJECT)
	ar cr $@ $(librempix_a_STACK_PMPI_OBJECT)
	ranlib $@

$(librempix_so_LIB): $(librempix_a_LIB)
	$(MPICC) $(AM_CXXFLAGS) $(librempix_a_CXXFLAGS) $(CXXFLAGS) $(AM_LDFLAGS) -shared -o $@ $(librempix_a_STACK_PMPI_OBJECT)

all-local: $(librempix_a_LIB) $(librempix_so_LIB)

clean-local:
	-rm -f $(librempix_a_LIB) $(librempix_so_LIB) $(librempix_a_OBJECTS) $(librempix_a_LD_OBJECT) $(librempix_a_STACK_PMPI_OBJECT)

endif

