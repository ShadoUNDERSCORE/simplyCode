#include "../include/editor.h"

void editor_insert_char(EditorState *es, char c) {
  buffer_insert_char(es->buffer, es->cursor_row, es->cursor_col, c);
}

void editor_delete_char(EditorState *es) {
  TextBuffer *buf = es->buffer;
  es->cursor_col++;
  buffer_backspace_char(buf, es->cursor_row, es->cursor_col);
}

void editor_backspace_char(EditorState *es) {
  TextBuffer *buf = es->buffer;
  buffer_backspace_char(buf, es->cursor_row, es->cursor_col);
  es->cursor_col--;
}

void editor_move_cursor(EditorState *es, int row, int col) {
  es->cursor_row = row;
  es->cursor_col = col;
  // if cursor is off screen scroll to cursor
}
