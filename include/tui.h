#ifndef TUI_H
#define TUI_H

#include <notcurses/notcurses.h>
#include <locale.h>
#include <time.h>

#include "editor.h"

enum key_t {
  WRITEABLE,
  MOVEMENT,
  FUNCTIONAL
};

void tui_run(EditorState *es);
void update_cursor_pos(EditorState *es, uint32_t key);
void vp_scroll(EditorState *es, int vp_cur_row, int vp_row_max);
void draw_line_nums(EditorState *es, struct ncplane *p, int max_rows);
void draw_screen(EditorState *es, struct ncplane *p, int max_rows);
void handle_ctrl_combo(EditorState *es, uint32_t key);
enum key_t get_key_type(uint32_t key);


#endif

