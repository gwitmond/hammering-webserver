#!/usr/bin/env bash

while true
do
    inotifywait -q -e modify *.[ch] Makefile
    clear
    rm *.o
    make btest && ./btest || true
done


