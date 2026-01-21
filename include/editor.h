#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>

#include "buffer.h"

typedef struct {
  TextBuffer *buffer;
  int cursor_row;
  int cursor_col;
  int v_scroll_offset;
  int h_scroll_offset;
  bool dirty;
  char *filename;
  bool main_loop_running;
} EditorState;

void editor_save_to_file(EditorState *es);
void editor_create_row(EditorState *es, int preceeding_row);
void editor_delete_row(EditorState *es);
void editor_insert_char(EditorState *es, char c);
void editor_delete_char(EditorState *es);
void editor_backspace_char(EditorState *es);
void editor_move_cursor(EditorState *es, int row, int col);
int editor_row_len(EditorState *es, int row);
void editor_get_row_text(EditorState *es, int row, char *logical_text);

#endif

