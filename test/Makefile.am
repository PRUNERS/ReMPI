SUBDIRS=../src

AM_CFLAGS = -g -rdynamic
AM_CXXFLAGS = -std=c++11

if WITH_BLUEGENE	
LIBS += $(libz_a_LIB) 
endif

noinst_PROGRAMS = \
	rempi_test_master_worker \
	rempi_test_master_worker_fortran \
	rempi_test_mini_mcb \
	rempi_test_hypre_parasails \
	rempi_test_units \
	rempi_test_msg_race \
	reomp_test_units

if ENABLE_CDC
if WITH_BLUEGENE
noinst_PROGRAMS += \
		rempi_test_master_workerx \
		rempi_test_master_worker_fortranx \
		rempi_test_mini_mcbx \
		rempi_test_hypre_parasailsx \
		rempi_test_unitsx \
		rempi_test_msg_racex 
endif
endif


if ENABLE_CDC
if WITH_BLUEGENE	
librempix_LIB = ../src/.libs/librempix.a
rempi_test_master_workerx_SOURCES = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_workerx_LDADD = $(librempix_LIB)
rempi_test_master_worker_fortranx_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortranx_LDADD = $(librempix_LIB)
rempi_test_mini_mcbx_SOURCES = rempi_test_mini_mcb.c rempi_test_util.c
rempi_test_mini_mcbx_LDADD = $(librempix_LIB)
rempi_test_hypre_parasailsx_SOURCES = rempi_test_hypre_parasails.c rempi_test_util.c
rempi_test_hypre_parasailsx_LDADD = $(librempix_LIB)
rempi_test_unitsx_SOURCES = rempi_test_units.c rempi_test_util.c
rempi_test_unitsx_LDADD = $(librempix_LIB)
rempi_test_msg_racex_SOURCES = rempi_test_msg_race.c rempi_test_util.c
rempi_test_msg_racex_LDADD = $(librempix_LIB)

#target_fortranx=rempi_test_master_worker_fortranx$(EXEEXT)
#$(target_fortranx):
#	mpif90  -g -c -o rempi_test_master_worker_fortranx rempi_test_master_worker_fortranx.f90 -lmpi
endif
endif

if WITH_BLUEGENE
librempi_LIB = ../src/.libs/librempi.a
endif

rempi_test_master_worker_SOURCES = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_worker_LDADD = $(librempi_LIB)
rempi_test_master_worker_fortran_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortran_LDADD = $(librempi_LIB)
rempi_test_mini_mcb_SOURCES = rempi_test_mini_mcb.c rempi_test_util.c
rempi_test_mini_mcb_LDADD = $(librempi_LIB)
rempi_test_hypre_parasails_SOURCES = rempi_test_hypre_parasails.c rempi_test_util.c
rempi_test_hypre_parasails_LDADD = $(librempi_LIB)
rempi_test_units_SOURCES = rempi_test_units.c rempi_test_util.c
rempi_test_units_LDADD = $(librempi_LIB)
rempi_test_msg_race_SOURCES = rempi_test_msg_race.c rempi_test_util.c
rempi_test_msg_race_LDADD = $(librempi_LIB)
reomp_test_units_SOURCES = reomp_test_units.cpp rempi_test_util.c
#reomp_test_units_CCFLAGS = -O0 -g -Wall -fopenmp `llvm-config --cflags` -fno-inline
reomp_test_units_CCFLAGS = -O0 -g -Wall -fopenmp -fno-inline -Wno-deprecated -std=c++11
reomp_test_units_LDFLAGS = -L./ `llvm-config --ldflags` -L../src/llvm/.libs/ -lreomprr
reomp_test_units_LLVMIRPASS = ../src/llvm/.libs/libreomp.so 	

reomp_test_units$(EXEEXT): $(reomp_test_units_OBJECTS) $(reomp_test_units_DEPENDENCIES) $(EXTRA_reomp_test_units_DEPENDENCIES) $(reomp_test_units_LLVMIRPASS)
	@rm -f reomp_test_units$(EXEEXT)
	clang++ -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@.ll $@.cpp
	mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@.o -c $@.cpp 2> /dev/null
	mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@ $(reomp_test_units_OBJECTS)
	clang++ $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@_native.ll $@.cpp
	clang++ $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@_native $(reomp_test_units_SOURCES) 


#target_fortran=rempi_test_master_worker_fortran$(EXEEXT)
#$(target_fortran):
#	mpif90  -g -o rempi_test_master_worker_fortran rempi_test_master_worker_fortran.f90 -lmpi



.PHONY: clean_core
clean_core:
	-rm -f *.core
	-rm -f *.btr


