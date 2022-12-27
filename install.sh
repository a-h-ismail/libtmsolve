#!/bin/bash
mkdir /usr/include/tmsolve
cp *.h /usr/include/tmsolve
gcc -fpic -lm -c -O2 -D LOCAL_BUILD *.c
gcc -shared -lm -o libtmsolve.so *.o
rm *.o
mv ./libtmsolve.so /usr/lib64
echo Done!