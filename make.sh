#!/bin/sh

[ $# -lt 1 ] && PLATFORM="linux" || PLATFORM="$1"

[ "$PLATFORM" == "linux" ] && {
	CFLAGS="$(pkg-config --cflags sdl2)"
	LIBS="$(pkg-config --libs sdl2) -lm"

	cc $CFLAGS $LIBS src/main.c -o ./ray
}

[ "$PLATFORM" == "web" ] && {
	emcc -lm -s USE_SDL=2 -g src/main.c -o web/index.html --preload-file assets --use-preload-plugins --shell-file src/shell_minimal.html
}

exit 0
