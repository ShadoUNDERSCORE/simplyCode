#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#include "../include/common.h"

typedef struct {
  char *text;
  int gap_start;
  int gap_end;
  int capacity;
} Row;

typedef struct {
  Row **rows;
  int row_count;
  int capacity;
} TextBuffer;

TextBuffer *buffer_load(const char *path);
void buffer_free(TextBuffer *buf);
void buffer_save(TextBuffer *buf, const char *path);
void gap_move(Row *row, int index);
void gap_grow(Row *row);
void buffer_insert_char(TextBuffer *buf, int row, int col, char c);
void buffer_backspace_char(TextBuffer *buf, int row, int col);
