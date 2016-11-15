#!/bin/sh

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

