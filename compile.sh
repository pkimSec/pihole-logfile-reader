#!/bin/bash
gcc -o file-search file-reader.c `pkg-config --cflags --libs gtk+-3.0` -lmagic
