#!/bin/sh

CFLAGS="$(pkg-config --cflags sdl2)"
LIBS="$(pkg-config --libs sdl2) -lm"

cc $CFLAGS $LIBS src/main.c -o ./ray
