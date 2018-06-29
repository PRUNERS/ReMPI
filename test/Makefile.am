SUBDIRS=../src

#CALIPER= -I/g/g90/sato5/repo/caliper/install_catalyst/include -L/g/g90/sato5/repo/caliper/install_catalyst/lib64 -lcaliper
AM_CFLAGS = -g -rdynamic $(CALIPER)
AM_CXXFLAGS = -std=c++11 $(CALIPER)

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
reomp_test_units_CCFLAGS = -O0 -g -Wall -fopenmp -fno-inline -Wno-deprecated -std=c++11 $(CALIPER)
reomp_test_units_LDFLAGS = -L./ `llvm-config --ldflags` -L../src/llvm/.libs/ -lreomp $(CALIPER)
reomp_test_units_LLVMIRPASS = ../src/llvm/.libs/libreompir.so

reomp_test_units$(EXEEXT): $(reomp_test_units_OBJECTS) $(reomp_test_units_DEPENDENCIES) $(EXTRA_reomp_test_units_DEPENDENCIES) $(reomp_test_units_LLVMIRPASS)
	@rm -f reomp_test_units$(EXEEXT)
	-rm rempi_drace_test.log.*
#make binary with archer
	/usr/global/tools/pruners/toss_3_x86_64/archer-1.0.0b-clang-3.9.1/bin/clang-archer++ -I/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/include -L/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/lib -lmpicxx -Wl,-rpath -Wl,/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/lib -lmpi -lpmi2 -fomit-frame-pointer -fsanitize=thread $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@_archer $(reomp_test_units_SOURCES) 
	-export TSAN_OPTIONS="log_path=rempi_drace_test.log history_size=7" && ./$@_archer 16 0.01


#make .ll for test with instrumentation
	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@.ll $@.cpp

#make binary for test with instrumentation
	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@.o -c $@.cpp 

	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@ $(reomp_test_units_OBJECTS)

#make .ll for test without instrumentation
	mpicxx $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@_native.ll $@.cpp
#make binary for test without instrumentation
	mpicxx $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@_native $(reomp_test_units_SOURCES) 


#target_fortran=rempi_test_master_worker_fortran$(EXEEXT)
#$(target_fortran):
#	mpif90  -g -o rempi_test_master_worker_fortran rempi_test_master_worker_fortran.f90 -lmpi



.PHONY: clean_core
clean_core:
	-rm -f *.core 2> /dev/null
	-rm -f *.btr 2> /dev/null
	-rm reomp_test_units.o-* 2> /dev/null
	-rm reomp_test_units.ll-* 2> /dev/null
#	-rm rempi_drace_test.log.* 2> /dev/null


