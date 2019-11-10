#!/bin/sh

gcc -I../include -fPIC -c zslib.c
gcc -shared zslib.o -lpthread -o libzsystem.so

cp ../include/*.h /usr/include/zsystem
cp libzsystem.h /usr/lib64
