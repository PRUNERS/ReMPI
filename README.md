[![Build Status](https://travis-ci.org/PRUNERS/ReMPI.svg?branch=master)](https://travis-ci.org/PRUNERS/ReMPI)


<img src="files/rempi_logo.png" height="60%" width="60%" alt="ReMPI logo" title="ReMPI" align="middle" />


# Introduction

 * ReMPI is a record-and-replay tool for MPI applications written in C/C++ and/or fortran.
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
    $ cd test
    $ mkdir rempi_record
    
Record mode (REMPI_MODE=0)
    
    $ REMPI_MODE=0 REMPI_DIR=./rempi_record LD_PRELOAD=<path to installation directory>/lib/librempi.so srun(or mpirun) -n 4 ./rempi_test_units matching
    
Replay mode (REMPI_MODE=1)
    
    $ REMPI_MODE=1 REMPI_DIR=./rempi_record LD_PRELOAD=<path to installation directory>/lib/librempi.so srun(or mpirun) -n 4 ./rempi_test_units matching
    
"REMPI::<hostname>:  0:  Global validation code: 1939202000" is a hash value computed based on the order of MPI events (e.g., Message receive order, message test results and etc.). If you run this example code several times with REMPI_MODE=0, you will see that this hash value changes from run to run. This means this example code is MPI non-deterministic. Once you run this example code and record MPI events with REMPI_MODE=0, you can reproduce this hash value with REMPI_MODE=1. This means MPI events are reproduced.

## 3. Running other examples

The following example script assumes the resource manager is SLURM and that ReMPI is installed in /usr/local. You must edit the example_x86.sh file othewise.

     cd example
     sh ./example_x86.sh 16
     ls -ltr .rempi # lists record files
     
# MPI functions that ReMPI records and relays
ReMPI record and replay results of following MPI functions.

## Blocking Receive

  * MPI_Recv

## Message Completion Wait/Test

  * MPI_Wait/Waitany/Waitsome/Waitall
  * MPI_Test/Testany/Testsome/Testall

In current ReMPI, MPI_Request must be initialized by following "Supported" MPI functions. Wait/Test Message Completion functions using MPI_Request initializaed by "Unsupported" MPI functions are not recorded and replayed (Unsupported MPI functions will be supporeted in future).

  * Supported
    * MPI_Irecv
    * MPI_Isend/Ibsend/Irsend/Issend
  * Unsupported
    * MPI_Recv_init
    * MPI_Send/Ssend/Rsend/Bsend_init
    * MPI_Start/Startall
    * All non-blocking collectives (e.g., MPI_Ibarrier) 
      
## Message Arrival Probe

  * MPI_Probe/Iprobe
  
## Other sources of non-determinism

Current ReMPI version record and replay only MPI and does not record and repaly other sources of non-determinism suca as OpenMP and other non-deterministic libc functions (e.g., gettimeofday(), clock() and etc.).


# Using ReMPI with TotalView

Since ReMPI is implemented via a PMPI wrapper, ReMPI works with Totalvew (Parallel debugger). The common use case is that you first record a buggy behavior in ReMPI record mode without TotalView and then replay this buggy behavior with TotalView in ReMPI replay mode. There are two methods to use ReMPI with TotalView.

 * Command Line Options: http://docs.roguewave.com/codedynamics/2017.0/html/index.html#page/TotalViewLH/TotalViewCommandLineOptions.html

## Method 1: Command line

You can simply launch the TotalVew GUI with the "totalview -args" command. (LD_PRELOAD must be set thorught a TotalView command line option: -env variable=value)

    $ REMPI_MODE=1 REMPI_DIR=./rempi_record totalview -env LD_PRELOAD=<path to installation directory>/lib/librempi.so -args srun(or mpirun) -n 4 ./rempi_test_units matching
OR

    $ export REMPI_MODE=1
    $ export REMPI_DIR=./rempi_record
    $ totalview -env LD_PRELOAD=<path to installation directory>/lib/librempi.so -args srun(or mpirun) -n 4 ./rempi_test_units matching
    
    
## Method 2: GUI

You can also set the REMPI_MODE, REMPI_DIR and LD_PRELOAD variable after launching TotalView. 

(Step 1) Run yoru application with TotalView

    $ REMPI_MODE=1 REMPI_DIR=./rempi_record LD_PRELOAD=<path to installation directory>/lib/librempi.so totalview -args srun(or mpirun) -n 4 ./rempi_test_units matching
    
(Step 2) Select [Process] => [Startup Parameters] in the GUI menu, and then select [Arguments] tab

(Step 3) Specify the environment variables in the "Environment variables" textbox (One environment variable per line).

    REMPI_MODE=1 
    REMPI_DIR=./rempi_record
    LD_PRELOAD=<path to installation directory>/lib/librempi.so


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

# Environment variables

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

# References

  * Kento Sato, Dong H. Ahn, Ignacio Laguna, Gregory L. Lee, and Martin Schulz. 2015. Clock delta compression for scalable order-replay of non-deterministic parallel applications. In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis (SC '15). ACM, New York, NY, USA, , Article 62 , 12 pages. DOI=http://dx.doi.org/10.1145/2807591.2807642
