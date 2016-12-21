#!/bin/sh

grep -R "MPI_" * | awk '{print substr($0, index($0, "MPI"), index($0, "(") - index($0, "MPI"))}' | sort | uniq
grep -R "mpi_" * | awk '{print substr($0, index($0, "mpi"), index($0, "(") - index($0, "mpi"))}' | sort | uniq

