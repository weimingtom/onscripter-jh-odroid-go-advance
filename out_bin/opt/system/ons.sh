#!/bin/sh

cd /home/odroid/onscripter_cn_test
export LD_LIBRARY_PATH=/home/odroid
./onscripter > a.txt 2>&1
