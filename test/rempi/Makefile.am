#SUBDIRS=../../src

#CALIPER= -I/g/g90/sato5/repo/caliper/install_catalyst/include -L/g/g90/sato5/repo/caliper/install_catalyst/lib64 -lcaliper
AM_CFLAGS = -g -rdynamic $(CALIPER) -I../common
AM_CXXFLAGS = -std=c++11 $(CALIPER) -I../common



if WITH_BLUEGENE	
LIBS += $(libz_a_LIB) 
endif

noinst_PROGRAMS = \
	rempi_test_master_worker \
	rempi_test_master_worker_fortran \
	rempi_test_mini_mcb \
	rempi_test_hypre_parasails \
	rempi_test_units \
	rempi_test_units_fortran \
	rempi_test_msg_race 


if ENABLE_CDC
if WITH_BLUEGENE
noinst_PROGRAMS += \
		rempi_test_master_workerx \
		rempi_test_master_worker_fortranx \
		rempi_test_mini_mcbx \
		rempi_test_hypre_parasailsx \
		rempi_test_unitsx \
		rempi_test_units_fortranx \
		rempi_test_msg_racex 
endif
endif


if ENABLE_CDC
if WITH_BLUEGENE	
librempix_LIB = ../src/.libs/librempix.a
rempi_test_master_workerx_SOURCES = rempi_test_master_worker.c ../common/rempi_test_util.c
rempi_test_master_workerx_LDADD = $(librempix_LIB)
rempi_test_master_worker_fortranx_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortranx_LDADD = $(librempix_LIB)
rempi_test_mini_mcbx_SOURCES = rempi_test_mini_mcb.c ../common/rempi_test_util.c
rempi_test_mini_mcbx_LDADD = $(librempix_LIB)
rempi_test_hypre_parasailsx_SOURCES = rempi_test_hypre_parasails.c ../common/rempi_test_util.c
rempi_test_hypre_parasailsx_LDADD = $(librempix_LIB)
rempi_test_unitsx_SOURCES = rempi_test_units.c ../common/rempi_test_util.c
rempi_test_unitsx_LDADD = $(librempix_LIB)
rempi_test_units_fortranx_SOURCES = rempi_test_units_fortran.f90 ../common/rempi_test_util.c
rempi_test_units_fortranx_LDADD = $(librempix_LIB)
rempi_test_msg_racex_SOURCES = rempi_test_msg_race.c ../common/rempi_test_util.c
rempi_test_msg_racex_LDADD = $(librempix_LIB)

#target_fortranx=rempi_test_master_worker_fortranx$(EXEEXT)
#$(target_fortranx):
#	mpif90  -g -c -o rempi_test_master_worker_fortranx rempi_test_master_worker_fortranx.f90 -lmpi
endif
endif

if WITH_BLUEGENE
librempi_LIB = ../src/.libs/librempi.a
endif

rempi_test_master_worker_SOURCES = rempi_test_master_worker.c ../common/rempi_test_util.c
rempi_test_master_worker_LDADD = $(librempi_LIB)
rempi_test_master_worker_fortran_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortran_LDADD = $(librempi_LIB)
rempi_test_mini_mcb_SOURCES = rempi_test_mini_mcb.c ../common/rempi_test_util.c
rempi_test_mini_mcb_LDADD = $(librempi_LIB)
rempi_test_hypre_parasails_SOURCES = rempi_test_hypre_parasails.c ../common/rempi_test_util.c
rempi_test_hypre_parasails_LDADD = $(librempi_LIB)
rempi_test_units_SOURCES = rempi_test_units.c ../common/rempi_test_util.c
rempi_test_units_LDADD = $(librempi_LIB)
rempi_test_units_fortran_SOURCES = rempi_test_units_fortran.f90 ../common/rempi_test_util.c
rempi_test_units_fortran_LDADD = $(librempi_LIB)
rempi_test_msg_race_SOURCES = rempi_test_msg_race.c ../common/rempi_test_util.c
rempi_test_msg_race_LDADD = $(librempi_LIB)


.PHONY: clean_core
clean_core:
	-rm -f *.core 2> /dev/null
	-rm -f *.btr 2> /dev/null



