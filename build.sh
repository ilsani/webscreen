#!/bin/bash

mkdir bin
cd src

gcc -g -o ../bin/webscreen main.c browser.c str_utils.c $(pkg-config --cflags --libs webkit2gtk-4.0) -lX11
