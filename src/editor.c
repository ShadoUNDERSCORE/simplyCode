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
  // es->cursor_col--;
  es->cursor_col = es->buffer->rows[es->cursor_row]->gap_start;
}

void editor_move_cursor(EditorState *es, int row, int col) {
  es->cursor_row = row;
  es->cursor_col = col;
  // if cursor is off screen scroll to cursor
}

int editor_row_len(EditorState *es, int row) {
  return buffer_row_logical_len(es->buffer->rows[row]);
}

void editor_get_row_text(EditorState *es, int row, char *logical_text) {
  int logical_index = 0;
  for (int i = 0; i < es->buffer->rows[row]->gap_start; i++) {
    logical_text[i] = es->buffer->rows[row]->text[i];
    logical_index = i;
  }
  logical_index++;
  for (int i = es->buffer->rows[row]->gap_end; i < es->buffer->rows[row]->capacity; i++) {
    logical_text[logical_index] = es->buffer->rows[row]->text[i];
    logical_index++;
  }
}

