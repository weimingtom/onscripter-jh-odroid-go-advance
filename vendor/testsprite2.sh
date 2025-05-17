#!/bin/sh

cd /home/odroid/SDL-master/test/
export LD_LIBRARY_PATH=/home/odroid/SDL-master/test/
./testsprite2 > a.txt 2>&1
