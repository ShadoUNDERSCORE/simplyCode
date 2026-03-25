CFLAGS := $(shell pkg-config --cflags notcurses)
LDFLAGS := $(shell pkg-config --libs notcurses)

# -fsanitize=address -g
simplyCode: src/main.c
	gcc src/main.c src/buffer.c src/editor.c src/config.c src/tui.c -o simplyCode -Wall -Wextra -Wpedantic -Wshadow $(CFLAGS) $(LDFLAGS)
