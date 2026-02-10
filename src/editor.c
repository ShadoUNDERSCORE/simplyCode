#include "../include/editor.h"

#include <stdbool.h>

void editor_save_to_file(EditorState *es) {
  buffer_save(es->buffer, es->filename);
  return;
}

void editor_create_row(EditorState *es, int preceeding_row) {
  buffer_create_row(es->buffer, preceeding_row, es->cursor_col);
  es->cursor_row = preceeding_row + 1;
  es->cursor_col = 0;
  return;
}

void editor_delete_row(EditorState *es) {
  if (es->cursor_row > 0) {
    int len = editor_row_len(es, es->cursor_row);
    buffer_delete_row(es->buffer, es->cursor_row);
    es->cursor_row--;
    es->cursor_col = editor_row_len(es, es->cursor_row) - len;
  }
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

int editor_row_len(EditorState *es, int row) {
  return buffer_row_logical_len(es->buffer->rows[row]);
}

void editor_get_row_text(EditorState *es, int row, char *logical_text) {
  buffer_get_row_text(es->buffer, row, logical_text);
  return;
}

