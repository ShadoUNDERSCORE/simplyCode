#include "../include/editor.h"

void editor_save_to_file(EditorState *es) {
  buffer_save(es->buffer, es->filename);
  return;
}

void editor_create_row(EditorState *es) {
  buffer_create_row(es->buffer, es->cursor_row);
  es->cursor_col = 0;
  es->cursor_row++;
  return;
}

void editor_insert_char(EditorState *es, char c) {
  buffer_insert_char(es->buffer, es->cursor_row, es->cursor_col, c);
  es->cursor_col++;
  return;
}

void editor_delete_char(EditorState *es) {
  es->cursor_col++;
  buffer_backspace_char(es->buffer, es->cursor_row, es->cursor_col);
  return;
}

void editor_backspace_char(EditorState *es) {
  if (es->cursor_col > 0) {
    buffer_backspace_char(es->buffer, es->cursor_row, es->cursor_col);
    es->cursor_col = es->buffer->rows[es->cursor_row]->gap_start;
  } // else if (es->cursor_col == 0) {

  //}
  return;
}

void editor_move_cursor(EditorState *es, int row, int col) {
  es->cursor_row = row;
  es->cursor_col = col;
  // if cursor is off screen scroll to cursor
  return;
}

int editor_row_len(EditorState *es, int row) {
  return buffer_row_logical_len(es->buffer->rows[row]);
}

void editor_get_row_text(EditorState *es, int row, char *logical_text) {
  int logical_index = 0;
  if (es->buffer->rows[row]->gap_start != 0) {
    for (int i = 0; i < es->buffer->rows[row]->gap_start; i++) {
      logical_text[i] = es->buffer->rows[row]->text[i];
      logical_index = i;
    }
    logical_index++;
  }
  for (int i = es->buffer->rows[row]->gap_end; i < es->buffer->rows[row]->capacity; i++) {
    logical_text[logical_index] = es->buffer->rows[row]->text[i];
    logical_index++;
  }
  return;
}

