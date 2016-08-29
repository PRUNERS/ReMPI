
# Introduction

 * ReMPI is a record-and-replay tool for MPI applications.
 * (Optional) ReMPI implements Clock Delta Compression (CDC) for compressing records

# Quick Start

## 1. Get source code 

### From git repogitory

    $ git clone --recursive ssh://git@cz-stash.llnl.gov:7999/prun/rempi.git
    OR
    $ git clone ssh://git@cz-stash.llnl.gov:7999/prun/rempi.git
    $ git submodule update --init
  
    ./autogen.sh

### From tarball

    $ tar zxvf ./rempi_xxxxx.tar.bz

## 2. Build ReMPI

### Build (General)

    cd rempi
    ./configure --prefix=<path to installation directory>
    make 
    make install

### Build (BG/Q)

    cd rempi
    ./configure --prefix=<path to installation directory> --with-glugene --with-zlib-static=/usr/local/tools/zlib-1.2.6/
    make 
    make install


## 3. Run examples

Assuming SLURM

     cd example
     sh ./example_x86.sh 16
     ls -ltr .rempi # lists record files


# Configuration Option

For more details, run ./configure -h  

  * `--enable-cdc`: (Experimental) enables CDC (clock delta compression), and output librempix.a and .so
  * `--with-bluegene`: build test codd with static library
  * `--with-zlib-static`: path to installation directory for libz.a


# Environmental valiables

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

    Kento Sato, Dong H. Ahn, Ignacio Laguna, Gregory L. Lee, and Martin Schulz. 2015. Clock delta compression for scalable order-replay of non-deterministic parallel applications. In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis (SC '15). ACM, New York, NY, USA, , Article 62 , 12 pages. DOI=http://dx.doi.org/10.1145/2807591.2807642
