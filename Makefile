CFLAGS := $(shell pkg-config --cflags notcurses)
LDFLAGS := $(shell pkg-config --libs notcurses)

simplyCode: src/main.c
	gcc src/main.c src/buffer.c src/editor.c src/tui.c -o simplyCode -Wall -Wextra -pedantic $(CFLAGS) $(LDFLAGS)
