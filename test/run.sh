#!/bin/sh

bin="./rempi_test_units"
REMPI_ENCODE=0 REMPI_MODE=${mode} REMPI_DIR=${prefix} LD_PRELOAD=/g/g90/sato5/repo/rempi/src/.libs/librempi.so ${bin}
exit

rm -rf /p/lscratchv/sato5/test/.rempi/*
./test-bgq.sh 0 16 > log16r
du -sh /p/lscratchv/sato5/test/.rempi/ >> log16r
rm -rf /p/lscratchv/sato5/test/.rempi/*
exit

./test-bgq.sh 0 32 > log32r
du -sh /p/lscratchv/sato5/test/.rempi/ >> log32r
rm -rf /p/lscratchv/sato5/test/.rempi/*


./test-bgq.sh 0 64 > log64r
du -sh /p/lscratchv/sato5/test/.rempi/ >> log64r
rm -rf /p/lscratchv/sato5/test/.rempi/*

