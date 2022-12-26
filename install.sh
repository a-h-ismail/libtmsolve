#!/bin/bash
mkdir /usr/include/tmsolve
cp *.h /usr/include/tmsolve
gcc -fpic -c -O2 *.c -lm
gcc -shared -o libtmsolve.so *.o
rm *.o
mv ./libtmsolve.so /usr/lib64
echo Done!