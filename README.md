
# Introduction

 * ReMPI is a record-and-replay tool for MPI applications.
 * ReMPI implements Clock Delta Compression (CDC) for compressing records

# Quick Start

 * Assuming catalyst system

## Get source code

    git clone --recursive https://sato5@lc.llnl.gov/stash/scm/prun/rempi.git # It requires authentications 7 times

## Build ReMPI

    cd rempi
    use openmpi-intel-1.8.4
    export PATH=/path/to/scripts/external/pmpis/stack_pmpi:$PATH
    make


## Build test code: MCB

    cd test/external/mcb
    ./build-linux-x86_64.sh

## Run examples

     cd ../..
     salloc -N4 -ppbatch
     ./example.sh 64
     srun ls -ltr /l/ssd # lists record files

# Environmental valiables

 * `REMPI_MODE`: Record mode OR Replay mode
     * `0`: Record mode
     * `1`: Replay mode
 * `REMPI_DIR`: Directory path for record files
 * `REMPI_ENCODE`: Encoding mode
     * `0`: Simple recording 
     * `1`: `0` + record format optimization
     * `2` and `3`: (Experimental encoding)
     * `4`: Clock Delta Compression
     * `5`: Same as `4`
 * `REMPI_GZIP`: Enable gzip compression
     * `0`: Disable zlib
     * `1`: Enable zlib
 * `REMPI_TEST_ID`: Enable Matching Function (MF) Identification
     * `0`: Disable MF Identification
     * `1`: Enable MF Identification

# References

    Kento Sato, Dong H. Ahn, Ignacio Laguna, Gregory L. Lee, and Martin Schulz. 2015. Clock delta compression for scalable order-replay of non-deterministic parallel applications. In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis (SC '15). ACM, New York, NY, USA, , Article 62 , 12 pages. DOI=http://dx.doi.org/10.1145/2807591.2807642


# Note
## Note 1
 * MPI_VERSION == 3 && defined(REMPI_LITE)
     * Simple ReMPI => librempilite.so

 * MPI_VERSION == 3 && !defined(REMPI_LITE)
     * Clock + CDC ReMPI => libremp.so

 * MPI_VERSION != 3 && defined(REMPI_LITE)
     * Simple ReMPI => librempilite.so

 * MPI_VERSION != 3 && !defined(REMPI_LITE)
     * Clock ReMPI => libremp.so


## Note 2

 * Non-piggyback
     * Simple ReMPI:  defined(REMPI_LITE)
 * Piggyback
     * Clock  ReMPI: !defined(REMPI_LITE)
     * CDC    ReMPI: !defined(REMPI_LITE) && MPI_VERSION == 3