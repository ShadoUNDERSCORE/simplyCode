#ifndef TUI_H
#define TUI_H

#include "editor.h"

#include <notcurses/notcurses.h>

enum key_t {
  WRITEABLE,
  MOVEMENT,
  FUNCTIONAL
};

void tui_run(EditorState *es);
void update_cursor_pos(EditorState *es, uint32_t key);
void vp_v_scroll(EditorState *es, int *vp_cur_row, int vp_row_max);
void vp_h_scroll(EditorState *es, int *vp_cur_col, int vp_cur_max);
void draw_line_nums(EditorState *es, struct ncplane *p, int max_rows);
void draw_screen(EditorState *es, struct ncplane *p, int max_rows);
void handle_ctrl_combo(EditorState *es, uint32_t key, CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *staged_cmd);
void indent(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack, CommandStage *staged_cmd);
enum key_t get_key_type(uint32_t key);


#endif

