#!/bin/bash

# build dll
clang -I ./ --shared -fPIC test.c -o ./code.dll

# build exe
clang -I ./ -g $(sdl2-config --cflags) -fPIC -L$(pwd) -lSDL2 -lGL -lGLEW main.c -o main

