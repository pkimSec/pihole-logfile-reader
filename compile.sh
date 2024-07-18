#!/bin/bash
gcc -o pihole-logreader pihole-logreader.c `pkg-config --cflags --libs gtk+-3.0` -lmagic
