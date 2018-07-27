#!/usr/bin/env bash

while true
do
    inotifywait -q -e modify *.[ch] Makefile
    clear
    make clean btest && ./btest || true
done


