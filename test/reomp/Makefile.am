#SUBDIRS=../../src

AM_CFLAGS = -g -rdynamic $(CALIPER) -I../common
AM_CXXFLAGS = -std=c++11 $(CALIPER) -I../common
REOMP_LIBRARY_PATH=../../src/reomp/.libs/

noinst_PROGRAMS = \
	reomp_test_units_without_reomp \
	reomp_test_units_with_reomp 


REOMP_LLVM_IR_PASS = ../../src/reomp/.libs/libreompir.so

reomp_test_units_without_reomp_SOURCES = reomp_test_units.cpp ../common/rempi_test_util.c
reomp_test_units_without_reomp_CFLAGS = -O3 -g -Wall -fopenmp -fno-inline -Wno-deprecated $(CALIPER)
reomp_test_units_without_reomp_CXXFLAGS = $(reomp_test_units_without_reomp_CFLAGS) -std=c++11 
reomp_test_units_without_reomp_LDFLAGS = -fopenmp -L./ `llvm-config --ldflags` -L../../src/reomp/.libs/ -lreomp $(CALIPER)

reomp_test_units_with_reomp_SOURCES  = reomp_test_units.cpp ../common/rempi_test_util.c
reomp_test_units_with_reomp_CPPFLAGS = -Xclang -load -Xclang $(REOMP_LLVM_IR_PASS)
reomp_test_units_with_reomp_CFLAGS   = $(reomp_test_units_without_reomp_CFLAGS)
reomp_test_units_with_reomp_CXXFLAGS = $(reomp_test_units_with_reomp_CFLAGS) -std=c++11 
reomp_test_units_with_reomp_LDFLAGS  = $(reomp_test_units_without_reomp_LDFLAGS) 


# reomp_test_units$(EXEEXT): $(reomp_test_units_OBJECTS) $(reomp_test_units_DEPENDENCIES) $(EXTRA_reomp_test_units_DEPENDENCIES) $(reomp_test_units_LLVMIRPASS)
# 	@rm -f reomp_test_units$(EXEEXT)
# 	-rm rempi_drace_test.log.*
# #make binary with archer
# 	/usr/global/tools/pruners/toss_3_x86_64/archer-1.0.0b-clang-3.9.1/bin/clang-archer++ -I/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/include -L/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/lib -L$(REOMP_LIBRARY_PATH) -lmpicxx -Wl,-rpath -Wl,/usr/tce/packages/mvapich2/mvapich2-2.2-gcc-4.9.3/lib -lmpi -lpmi2 -fomit-frame-pointer -fsanitize=thread $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@_archer $(reomp_test_units_SOURCES) 
# 	-export TSAN_OPTIONS="log_path=rempi_drace_test.log history_size=7" && ./$@_archer 16 0.01


# #make .ll for test with instrumentation
# 	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@.ll $@.cpp

# #make binary for test with instrumentation
# 	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@.o -c $@.cpp 

# 	export TSAN_OPTIONS="log_path=rempi_drace_test.log" && mpicxx -Xclang -load -Xclang $(reomp_test_units_LLVMIRPASS) $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@ $(reomp_test_units_OBJECTS)

# #make .ll for test without instrumentation
# 	mpicxx $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -emit-llvm -S -o $@_native.ll $@.cpp
# #make binary for test without instrumentation
# 	mpicxx $(reomp_test_units_CCFLAGS) $(reomp_test_units_LDFLAGS) -o $@_native $(reomp_test_units_SOURCES) 


#target_fortran=rempi_test_master_worker_fortran$(EXEEXT)
#$(target_fortran):
#	mpif90  -g -o rempi_test_master_worker_fortran rempi_test_master_worker_fortran.f90 -lmpi


.PHONY: clean_core
clean_core:
	-rm -f *.core 2> /dev/null
	-rm -f *.btr 2> /dev/null
	-rm reomp_test_units.o-* 2> /dev/null
	-rm reomp_test_units.ll-* 2> /dev/null



