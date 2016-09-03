if WITH_BLUEGENE	
LIBS += $(libz_a_LIB) 
endif

noinst_PROGRAMS = \
	rempi_test_master_worker \
	rempi_test_master_worker_fortran \
	rempi_test_mini_mcb \
	rempi_test_hypre_parasails \
	rempi_test_units \
	rempi_test_msg_race  	
if ENABLE_CDC
if WITH_BLUEGENE
noinst_PROGRAMS += rempi_test_master_workerx \
	rempi_test_master_worker_fortranx \
	rempi_test_mini_mcbx \
	rempi_test_hypre_parasailsx \
	rempi_test_unitsx \
	rempi_test_msg_racex 
endif
endif


if ENABLE_CDC
if WITH_BLUEGENE	
librempix_LIB = @prefix@/lib/librempix.a
rempi_test_master_workerx_SOURCES = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_workerx_LIBS = $(librempix_LIB)
rempi_test_master_worker_fortranx_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortranx_LIBS = $(librempix_LIB)
rempi_test_mini_mcbx_SOURCES = rempi_test_mini_mcb.c rempi_test_util.c
rempi_test_mini_mcbx_LIBS = $(librempix_LIB)
rempi_test_hypre_parasailsx_SOURCES = rempi_test_hypre_parasails.c rempi_test_util.c
rempi_test_hypre_parasailsx_LIBS = $(librempix_LIB)
rempi_test_unitsx_SOURCES = rempi_test_units.c rempi_test_util.c
rempi_test_unitsx_LIBS = $(librempix_LIB)
rempi_test_msg_racex_SOURCES = rempi_test_msg_race.c rempi_test_util.c
rempi_test_msg_racex_LIBS = $(librempix_LIB)

target_fortranx=rempi_test_master_worker_fortranx$(EXEEXT)
$(target_fortranx):
	mpif90  -g -c -o rempi_test_master_worker_fortranx rempi_test_master_worker_fortranx.f90 -lmpi

endif
endif

if WITH_BLUEGENE
librempi_LIB = @prefix@/lib/librempi.a
endif
rempi_test_master_worker_SOURCES = rempi_test_master_worker.c rempi_test_util.c
rempi_test_master_worker_LIBS = $(librempi_LIB)
rempi_test_master_worker_fortran_SOURCES = rempi_test_master_worker_fortran.f90
rempi_test_master_worker_fortran_LIBS = $(librempi_LIB)
rempi_test_mini_mcb_SOURCES = rempi_test_mini_mcb.c rempi_test_util.c
rempi_test_mini_mcb_LIBS = $(librempi_LIB)
rempi_test_hypre_parasails_SOURCES = rempi_test_hypre_parasails.c rempi_test_util.c
rempi_test_hypre_parasails_LIBS = $(librempi_LIB)
rempi_test_units_SOURCES = rempi_test_units.c rempi_test_util.c
rempi_test_units_LIBS = $(librempi_LIB)
rempi_test_msg_race_SOURCES = rempi_test_msg_race.c rempi_test_util.c
rempi_test_msg_race_LIBS = $(librempi_LIB)


target_fortran=rempi_test_master_worker_fortran$(EXEEXT)
$(target_fortran):
	mpif90  -g -o rempi_test_master_worker_fortran rempi_test_master_worker_fortran.f90 -lmpi



.PHONY: clean_core
clean_core:
	-rm -f *.core
	-rm -f *.btr


