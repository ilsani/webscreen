#!/bin/bash

gcc -g -o webscreen  main.c browser.c str_utils.c $(pkg-config --cflags --libs webkit2gtk-4.0)
