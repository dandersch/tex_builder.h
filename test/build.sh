#!/bin/bash

# build dll
clang -I ./ -Wall -Wshadow -Wno-unused-variable --shared -fPIC test.c -o ./code.dll -pthread

# build exe
clang -I ./ -g $(sdl2-config --cflags) -fPIC -L$(pwd) -lSDL2 -lGL -lGLEW main.c -o main

