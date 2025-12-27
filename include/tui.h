#ifndef TUI_H
#define TUI_H

#include <notcurses/notcurses.h>
#include <locale.h>
#include <time.h>

#include "editor.h"

void tui_run(EditorState *es);
void update_cursor_pos(EditorState *es, char key);
void draw_screen(EditorState *es, struct ncplane *p);

#endif

