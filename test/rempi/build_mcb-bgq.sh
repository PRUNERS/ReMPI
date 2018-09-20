#!/bin/bash


#make -j12; make install; cd /g/g90/sato5/repo/MCBdouble/src/ && rm MCBenchmark.exe ; make -f Makefile-bgq-gnu; ls -ltr ./MCBenchmark.exe
cd ../src/; make -j12; make install; cd -;
cd /g/g90/sato5/repo/MCBdouble/src/ && rm MCBenchmark.exe ; make -f Makefile-bgq-gnu; ls -ltr ./MCBenchmark.exe; cd -;
#rm ./rempi_test_unitsx; make rempi_test_unitsx; ls -ltr ./rempi_test_unitsx
rm ./rempi_test_master_workerx; make rempi_test_master_workerx; ls -ltr ./rempi_test_master_workerx
