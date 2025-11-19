#include <stdbool.h>

#include "buffer.h"

typedef struct {
  TextBuffer *buffer;
  int cursor_row;
  int cursor_col;
  int scroll_offset;
  bool dirty;
  char *filename;
} EditorState;

void editor_insert_char(EditorState *es, char c);
void editor_delete_char(EditorState *es);
void editor_move_cursor(EditorState *es, int row, int col);

