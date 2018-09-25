[![Build Status](https://travis-ci.org/PRUNERS/ReMPI.svg?branch=master)](https://travis-ci.org/PRUNERS/ReMPI)


<img src="files/rempi_logo.png" height="60%" width="60%" alt="ReMPI logo" title="ReMPI" align="middle" />


# Introduction

 * ReMPI is a record-and-replay tool for MPI+OpenMP applications written in C/C++ and/or fortra
     * In a broad sense, "ReMPI" means a record-and-replay tool for MPI+OpenMP applications 
     * In a narrow sense,  "ReMPI" means MPI record-and-replay and "ReOMP" means OpenMP record-and-replay
 * (Optional) ReMPI implements Clock Delta Compression (CDC) for compressing records.

# Quick Start

## 1. Building ReMPI

### From Spack

    $ git clone https://github.com/LLNL/spack
    $ ./spack/bin/spack install rempi

### From git repository

    $ git clone git@github.com:PRUNERS/ReMPI.git
    $ cd ReMPI
    $ ./autogen.sh
    $ ./configure --prefix=<path to installation directory> 
    $ make 
    $ make install

### From tarball

    $ tar zxvf ./rempi_xxxxx.tar.bz
    $ cd <rempi directory>
    $ ./configure --prefix=<path to installation directory> 
    $ make 
    $ make install

### Note on building for BG/Q

To build on the IBM BG/Q platform, you will need to add the --with-blugene option and specify the path to zlib with the --with-zlib-static flag. You may also need to specify the MPICC and MPIFC variables. For example:

    $ ./configure --prefix=<path to installation directory> --with-bluegene --with-zlib-static=/usr/local/tools/zlib-1.2.6/ MPICC=/usr/local/tools/compilers/ibm/mpicxx-fastmpi-mpich-312 MPIFC=/usr/local/tools/compilers/ibm/mpif90-fastmpi-mpich-312
    $ make 
    $ make install

## 2. Running with ReMPI
    $ cd test/rempi
    $ mkdir rempi_record
    
### Record mode (REMPI_MODE=0)
    
    $ REMPI_MODE=0 REMPI_DIR=./rempi_record LD_PRELOAD=<path to installation directory>/lib/librempi.so srun(or mpirun) -n 4 ./rempi_test_units matching
    
For its convenience, ReMPI also provides a wapper script which execute the same command as the above. If you install ReMPI to a custom directory, you need to add "<path to installation directory>/bin/" path to the PATH environment variable.

    $ rempi_record srun(or mpirun) -n 4 ./rempi_test_units matching
    
ReMPI produces one file per MPI process.

    
### Replay mode (REMPI_MODE=1)
    
    $ REMPI_MODE=1 REMPI_DIR=./rempi_record LD_PRELOAD=<path to installation directory>/lib/librempi.so srun(or mpirun) -n 4 ./rempi_test_units matching

For its convenience, ReMPI also provides a wapper script which execute the same command as the above

    $ rempi_replay srun(or mpirun) -n 4 ./rempi_test_units matching
    
"REMPI::<hostname>:  0:  Global validation code: 1939202000" is a hash value computed based on the order of MPI events (e.g., Message receive order, message test results and etc.). If you run this example code several times with REMPI_MODE=0, you will see that this hash value changes from run to run. This means this example code is MPI non-deterministic. Once you run this example code and record MPI events with REMPI_MODE=0, you can reproduce this hash value with REMPI_MODE=1. This means MPI events are reproduced.

## 3. Running other examples

The following example script assumes the resource manager is SLURM and that ReMPI is installed in /usr/local. You must edit the example_x86.sh file othewise.

     cd example
     sh ./example_x86.sh 16
     ls -ltr .rempi # lists record files
     
## 4. Running with ReOMP
Let us take the program below and follow the steps to compile, run the proram, record and replay.
This example code is in test/reomp/reomp_example.cpp and the seriease of the steps are scripted in test/reomp/build_run_reomp_example.sh



	#include <stdlib.h>
	#include <stdio.h>
	#include <omp.h>
	#include <stdint.h>
	 
	static int reomp_example_omp_critical(int nth)
	{ 
	  	uint64_t i;
	  	volatile int sum;
	#pragma omp parallel for private(i)
		for (i = 0; i < 10000000L / nth; i++) {
	#pragma omp critical
			{ 
      			sum = sum * omp_get_thread_num() + 1;
    			}
		}
		return sum;
	}
	 
	static int reomp_example_data_race(int nth)
	{ 
	uint64_t i;
	volatile int sum = 1;
	#pragma omp parallel for private(i)
		for (i = 0; i < 3000000L / nth ; i++) {
			sum += nth;
		}
		return sum;
	}
	 
	int main(int argc, char **argv)
	{ 
		int nth = atoi(argv[1]);
		omp_set_num_threads(nth);
		int ret1 = reomp_example_omp_critical(nth);
		int ret2 = reomp_example_data_race(nth);
		fprintf(stderr, "omp_critical: ret = %15d\n", ret1);
		fprintf(stderr, "data_race:    ret = %15d\n", ret2);
		return 0;
	}

First let's compile and run without ReOMP.
Note that two functions, reomp_example_omp_critical and reomp_example_data_race, return non-deterministic values (i.e., sum).
If you run the program several times, you will see the different numerical results from run to run. 
In reomp_example_omp_critical, the numerical resutls changes depending on the order of threads entering the critical section.
In reomp_example_data_race, the non-deterministic numerical reuslts are produceds due to data races.   

	$ clang++ -O3 -fopenmp -o reomp_example_without_reomp reomp_example.cpp
	$ ./reomp_example_without_reomp 16   # 16 is the number of threads
	omp_critical: ret =           17116
	data_race:    ret =          191889	
	$ ./reomp_example_without_reomp 16   
	omp_critical: ret =      -456407940
	data_race:    ret =          188801
	
To reproduce the numerical results, compile the program with the ReOMP IR pass shared library.
Now, we can reproduce the numerical reuslt in reomp_example_omp_critical since ReOMP find the critical sections and record the order of threads entering the critical sections.
However, we still see inconsistent numerical results in reomp_example_data_race sicne ReOMP itself cannnot find where the data races occur.

	$ clang++ -Xclang -load -Xclang ../../src/reomp/.libs/libreompir.so -L../../src/reomp/.libs/ -lreomp -O3 -fopenmp -o reomp_example_with_reomp reomp_example.cpp
	$ export LD_LIBRARY_PATH=../../src/reomp/.libs/
	$ REOMP_MODE=0 ./reomp_example_with_reomp 16   # REOMP_MODE=0 means the ReOMP record mode.
	omp_critical: ret =     -2116977392
	data_race:    ret =          198769
	$ REOMP_MODE=1 ./reomp_example_with_reomp 16   # REOMP_MODE=0 means the ReOMP record mode.
	omp_critical: ret =     -2116977392
	data_race:    ret =          187489
	
ReOMP replys on a data race detector to find data races.
Let's detect the data races with Thread Sanitizer (or Archer).

	$ clang++ -g -fomit-frame-pointer -fsanitize=thread -O3 -fopenmp -o reomp_example_with_tsan reomp_example.cpp
	$ export 'TSAN_OPTIONS=log_path=reomp_tsan.log history_size=7'
	$ ./reomp_example_with_tsan 2
	
Let's re-compile the probram with ReOMP IR pass and the report file (reomp_tsan.log.xxxxx) from Thread Sanitizer and run.
Now, you will see the consistent numerical resutls from run to run.

	$ export TSAN_OPTIONS=log_path=reomp_tsan.log  # To let the ReOMP IR pass know where the TSAN report file is.
	$ clang++ -Xclang -load -Xclang ../../src/reomp/.libs/libreompir.so -L../../src/reomp/.libs/ -lreomp -L/usr/tce/packages/clang/clang-4.0.0/lib -O3 -fopenmp -o reomp_example_with_reomp_data_race reomp_example.cpp
	$ REOMP_MODE=0 ./reomp_example_with_reomp_data_race 16
	omp_critical: ret =     -1833974251
	data_race:    ret =          191793
	$ REOMP_MODE=1 ./reomp_example_with_reomp_data_race 16
	omp_critical: ret =     -1833974251
	data_race:    ret =          191793
	$ REOMP_MODE=1 ./reomp_example_with_reomp_data_race 16
	omp_critical: ret =     -1833974251
	data_race:    ret =          191793	

# Environment variables
## ReMPI
 * `REMPI_MODE`: Record mode OR Replay mode
     * `0`: Record mode
     * `1`: Replay mode
 * `REMPI_DIR`: Directory path for record files
 * `REMPI_ENCODE`: Encoding mode
     * `0`: Simple recording 
     * `1`: `0` + record format optimization
     * `2` and `3`: (Experimental encoding)
     * `4`: Clock Delta Compression (only when built with `--enable-cdc` option)
     * `5`: Same as `4` (only when built with `--enable-cdc` option)
 * `REMPI_GZIP`: Enable gzip compression
     * `0`: Disable zlib
     * `1`: Enable zlib
 * `REMPI_TEST_ID`: Enable Matching Function (MF) Identification
     * `0`: Disable MF Identification
     * `1`: Enable MF Identification
     
By default, ReMPI stores record files to the current working directory. If you want to change the record directory (e.g., /tmp), use the REMPI_DIR environment variable.

    $ rempi_record REMPI_DIR=/tmp srun(or mpirun) -n 4 ./rempi_test_units matching
    $ rempi_replay REMPI_DIR=/tmp srun(or mpirun) -n 4 ./rempi_test_units matching
    
Record data is all interger values. If you enables gzip compression capability via REMPI_GZIP, you can reduce the record size while a certain runtime overhead due to compression engine.

    $ rempi_record REMPI_DIR=/tmp REMPI_GZIP=1 srun(or mpirun) -n 4 ./rempi_test_units matching
    $ rempi_replay REMPI_DIR=/tmp REMPI_GZIP=1 srun(or mpirun) -n 4 ./rempi_test_units matching
    
## ReOMP
 * `REOMP_MODE`: Record mode OR Replay mode
     * `0` or `record`: Record mode
     * `1` or `replay`: Replay mode
     * `2` or `diable`: Disable ReOMP (Run your applicaiton with instrumented binary but ReOMP doest not record adn replay anything)
 * `REOMP_DIR`: Directory path for record files (Default is current directory)
 * `REOMP_METHOD`: Record-and-Replay method
     * `0`: Distributed epoch reocrding (default) 
     * `1`: Distributed clock recording
     * `2`: Serialized thread ID recording
     
# Non-determinism that ReMPI records and relays
ReMPI record and replay results of following MPI functions.

### MPI: Blocking Receive

  * MPI_Recv

### MPI: Message Completion Wait/Test

  * MPI_{Wait|Waitany|Waitsome|Waitall}
  * MPI_{Test|Testany|Testsome|Testall}

In current ReMPI, MPI_Request must be initialized by following "Supported" MPI functions. Wait/Test Message Completion functions using MPI_Request initializaed by "Unsupported" MPI functions are not recorded and replayed (Unsupported MPI functions will be supporeted in future).

  * Supported
    * MPI_Irecv
    * MPI_{Isend|Ibsend|Irsend|Issend}
  * Unsupported
    * MPI_Recv_init
    * MPI_{Send|Ssend|Rsend|Bsend}_init
    * MPI_{Start|Startall}
    * All non-blocking collectives (e.g., MPI_Ibarrier) 
      
### MPI: Message Arrival Probe

  * MPI_{Probe|Iprobe}
  
### MPI: Other sources of non-determinism

Current ReMPI version record and replay only MPI and does not record and repaly other sources of non-determinism suca as OpenMP and other non-deterministic libc functions (e.g., gettimeofday(), clock() and etc.).

### OpenMP:
ReOMP records and replays
  * OpenMP clauses 
    * Critical Section (#omp critical)
    * Reduction (#omp reduction)
    * Master (#omp master)
    * Single (#omp single)
  * OpenMP runtime
    * omp_set_lock() and omp_unset_lock()
    * omp_set_nest_lock() and omp_unset_nest_lock()
  * Atomic instructions
    * Atomic load/store
    * Atomic operations (cmpxchg and atomicrmw)
  * Data-racy load/store instructions (If TSAN data-race report files are provided when compiling)


# Using ReMPI with TotalView

Since ReMPI is implemented via a PMPI wrapper, ReMPI works with Totalvew (Parallel debugger). The common use case is that you first record a buggy behavior in ReMPI record mode without TotalView and then replay this buggy behavior with TotalView in ReMPI replay mode. There are two methods to use ReMPI with TotalView.

 * Command Line Options: http://docs.roguewave.com/codedynamics/2017.0/html/index.html#page/TotalViewLH/TotalViewCommandLineOptions.html

### Method 1: Command line

You can simply launch the TotalVew GUI with the "totalview -args" command. (LD_PRELOAD must be set thorught a TotalView command line option: -env variable=value)

    $ REMPI_MODE=1 REMPI_DIR=./rempi_record totalview -env LD_PRELOAD=<path to installation directory>/lib/librempi.so -args srun(or mpirun) -n 4 ./rempi_test_units matching

or

    $ export REMPI_MODE=1
    $ export REMPI_DIR=./rempi_record
    $ totalview -env LD_PRELOAD=<path to installation directory>/lib/librempi.so -args srun(or mpirun) -n 4 ./rempi_test_units matching
  
For its convenience, ReMPI provides a wapper script to lunch Totaiveiw with ReMPI.

Firs, record a particular execution that you want to diagnose with Totaiview

    $ rempi_record srun -n 4 ./rempi_test_units matching 
 
Then, diagnose this recorded execution with Totalview under ReMPI replay

    $ rempi_replay totalview -args srun -n 4 ./rempi_test_units matching
    
### Method 2: GUI

You can also set the REMPI_MODE, REMPI_DIR and LD_PRELOAD variable after launching TotalView. 
(Step 0) Record a particular execution that you want to diagnose with Totalview
(Step 1) Run your application with TotalView

    $ REMPI_MODE=1 totalview -args srun(or mpirun) -n 4 ./rempi_test_units matching
    
(Step 2) Select [Process] => [Startup Parameters] in the GUI menu, and then select [Arguments] tab

(Step 3) Specify the environment variables in the "Environment variables" textbox (One environment variable per line)
  
    LD_PRELOAD=<path to installation directory>/lib/librempi.so
    
(Step 4) Press "Run" button to execute


# Configuration Options

For more details, run ./configure -h  

  * `--enable-cdc`: (Optional) enables CDC (clock delta compression), and output librempix.a and .so. When CDC is enabled, ReMPI requires MPI3 and below two software
    * `--with-stack-pmpi`: (Required when `--enable-cdc` is specified) path to stack_pmpi directory (STACKP)
    * `--with-clmpi`: (Required when `--enable-cdc` is specified) path to CLMPI directory
  * `--with-bluegene`: (Required in BG/Q) build codes with static library for BG/Q system
  * `--with-zlib-static`: (Required in BG/Q) path to installation directory for libz.a

When the `--enable-cdc` option is specified, ReMPI require dependent software below:

 * STACKP: A static MPI tool enabling to run multiple PMPI tools.
   * https://github.com/PRUNER/StackP.git
 * CLMPI: A PMPI tool for piggybacking Lamport clocks.
   * https://github.com/PRUNER/CLMPI.git 



# References

  * Kento Sato, Dong H. Ahn, Ignacio Laguna, Gregory L. Lee, and Martin Schulz. 2015. Clock delta compression for scalable order-replay of non-deterministic parallel applications. In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis (SC '15). ACM, New York, NY, USA, , Article 62 , 12 pages. DOI=http://dx.doi.org/10.1145/2807591.2807642
