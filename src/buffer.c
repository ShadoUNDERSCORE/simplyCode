#include "../include/buffer.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const int N_NEW_LINES = 64;
const int INIT_CAPACITY = 128;

TextBuffer *buffer_load(const char *path) {

  bool healthy = true;
  bool new_file = false;
 
  FILE *stream = fopen(path, "r");
  if (!stream) {
    new_file = true;
  }

  TextBuffer *buf = malloc(sizeof(TextBuffer));
  buf->rows = malloc(sizeof(Row *) * INIT_CAPACITY);
  if (!buf->rows) healthy = false;
  buf->row_count = 0;
  buf->capacity = INIT_CAPACITY;

  if (new_file) {
    if (!healthy) {
      buffer_free(buf);
      return NULL;
    }
    buffer_create_row(buf, -1, 0);
    return buf;
  }

  char *line = NULL;
  size_t line_size = 0;
  ssize_t line_len;

  while ((line_len = getline(&line, &line_size, stream)) != -1) {
    int text_len = line_len;
    int gap_size = text_len < 32 ? 32 : text_len / 2 + 16;
    int row_size = text_len + gap_size;

    Row *new_row = malloc(sizeof(Row));
    if (!new_row) {
      healthy = false;
      break;
    }
    new_row->text = malloc(row_size);
    if (!new_row->text) {
      free(new_row);
      healthy = false;
      break;
    }
    // remove newlines
    memcpy(new_row->text, line, text_len);
    memset(new_row->text + text_len, 0, gap_size);
    if (new_row->text[text_len - 1] == '\n') {
      new_row->gap_start = text_len - 1;
    } else {
      new_row->gap_start = text_len;
    }
    new_row->gap_end   = row_size - 1;
    new_row->capacity  = row_size;

    if (buf->row_count == buf->capacity - 1) {
      buf->rows = realloc(buf->rows, (buf->capacity + N_NEW_LINES) * sizeof(Row *));
      if (!buf->rows) {
        healthy = false;
        break;
      }
      buf->capacity += N_NEW_LINES;
    }
    buf->rows[buf->row_count] = new_row;
    buf->row_count++;
  }

  free(line);
  fclose(stream);
  if (!healthy) {
    buffer_free(buf);
    return NULL;
  }
  return buf;
}

void buffer_free(TextBuffer *buf) {
  if (!buf) return;

  for (int i = 0; i < buf->row_count; i++) {
    Row *r = buf->rows[i];
    free(r->text);
    free(r);
  }

  free(buf->rows);
  free(buf);
}

void buffer_save(TextBuffer *buf, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return;
  for (int i = 0; i < buf->row_count; i++) {
    int prefix_len = buf->rows[i]->gap_start;
    char *prefix_ptr = buf->rows[i]->text;

    int tail_len = buf->rows[i]->capacity - buf->rows[i]->gap_end;
    char *tail_ptr = buf->rows[i]->text + buf->rows[i]->gap_end;

    fprintf(f, "%.*s", prefix_len, prefix_ptr);
    fprintf(f, "%.*s\n", tail_len, tail_ptr);
  }
  fclose(f);
  return;
}

void buffer_create_row(TextBuffer *buf, int preceeding_row, int col) {
  // Shift rows if not at end of array
  if (buf->row_count > 0) {
    if (buf->capacity > buf->row_count + 8) {
      memmove(&buf->rows[preceeding_row + 2], &buf->rows[preceeding_row + 1], (buf->row_count - preceeding_row) * sizeof(char *));
    } else {
      // Reallocate if necissary
      buf->rows = realloc(buf->rows, (buf->capacity + N_NEW_LINES) * sizeof(Row *));
      buf->capacity += N_NEW_LINES;
      memmove(&buf->rows[preceeding_row + 2], &buf->rows[preceeding_row + 1], (buf->row_count - preceeding_row) * sizeof(char *));
    }
  }
  Row *new_row = malloc(sizeof(Row));
  new_row->text = malloc(INIT_CAPACITY);
  new_row->gap_start = 0;
  new_row->gap_end = (buf->row_count == 0) ? INIT_CAPACITY - 1 : INIT_CAPACITY;
  new_row->capacity = INIT_CAPACITY;
  buf->rows[preceeding_row + 1] = new_row;
  buf->row_count++;
  if (preceeding_row > -1) {
    int preceeding_row_len = buffer_row_logical_len(buf->rows[preceeding_row]);
    if (col < preceeding_row_len) {
      char *text = malloc(preceeding_row_len);
      buffer_get_row_text(buf, preceeding_row, text);
      gap_move(buf->rows[preceeding_row], col);

      for (int i = col; i < preceeding_row_len; i++) {
        buffer_insert_char(buf, preceeding_row + 1, i - col, text[i]);
      }
      buf->rows[preceeding_row]->gap_end = buf->rows[preceeding_row]->capacity - 1;
    }
  }
}

void buffer_delete_row(TextBuffer *buf, int row) {
  if (row > 0 && row <= buf->row_count) {
    Row *preceeding_row = buf->rows[row - 1];
    Row *target_row = buf->rows[row];
    gap_move(preceeding_row, buffer_row_logical_len(preceeding_row));
    int trg_row_log_len = buffer_row_logical_len(target_row);
    if (preceeding_row->capacity < buffer_row_logical_len(preceeding_row) + trg_row_log_len + 16) {
      gap_grow(preceeding_row);
    }
    char *target_logical_text = malloc(trg_row_log_len);
    buffer_get_row_text(buf, row, target_logical_text);
    for (int i = 0; i < trg_row_log_len - 1; i++) {
      buffer_insert_char(buf, row - 1, buffer_row_logical_len(preceeding_row) - 1, target_logical_text[i]);
    }
    memmove(&buf->rows[row], &buf->rows[row + 1], (buf->row_count - row) * sizeof(Row *));
    free(target_row->text);
    free(target_row);
    buf->row_count--;
  }
  return;
}

static int logical_to_physical(Row *row, int logical_index) {
  if (logical_index <= row->gap_start) {
    return logical_index;
  }
  return logical_index + (row->gap_end - row->gap_start);
}

int buffer_row_logical_len(Row *row) {
  return row->gap_start + (row->capacity - row->gap_end);
}

void buffer_get_row_text(TextBuffer *buf, int row, char *dest) {
  int logical_index = 0;
  if (buf->rows[row]->gap_start != 0) {
    for (int i = 0; i < buf->rows[row]->gap_start; i++) {
      dest[i] = buf->rows[row]->text[i];
      logical_index = i;
    }
    logical_index++;
  }
  for (int i = buf->rows[row]->gap_end; i < buf->rows[row]->capacity; i++) {
    dest[logical_index] = buf->rows[row]->text[i];
    logical_index++;
  }
  return;
}

void gap_move(Row *row, int logical_index) {
  int logical_len = buffer_row_logical_len(row);
  if (logical_index < 0) logical_index = 0;
  if (logical_index > logical_len) logical_index = logical_len;

  int index = logical_to_physical(row, logical_index);

  if (index > row->gap_end) {
    // move gap right
    int n = index - row->gap_end; // distance between gap_end and index
    memmove(row->text + row->gap_start, row->text + row->gap_end, n);
    row->gap_start += n;
    row->gap_end += n;
  } else if (index < row->gap_start) {
    // move gap left
    int n = row->gap_start - index; // distance between index and gap_start
    memmove(row->text + row->gap_end - n, row->text + index, n);
    row->gap_start -= n;
    row->gap_end -= n;
  }
  return;
}

void gap_grow(Row *row) {
  int old_gap_size = row->gap_end - row->gap_start;
  int right_size = row->capacity - row->gap_end;

  int grow_amount = old_gap_size;
  int new_gap_size = old_gap_size + grow_amount;
  int new_capacity = row->capacity + grow_amount;

  char *right_text = malloc(right_size);
  memcpy(right_text, row->text + row->gap_end, right_size);

  row->text = realloc(row->text, new_capacity);
  row->gap_end = row->gap_start + new_gap_size;
  row->capacity = new_capacity;
  memcpy(row->text + row->gap_end, right_text, right_size);

  free(right_text);
 
  return;
}

void buffer_insert_char(TextBuffer *buf, int row, int col, char c) {
  const int GAP_MIN = 8;
  Row *r = buf->rows[row];

  int logical_len = buffer_row_logical_len(r);
  if (col < 0) col = 0;
  if (col > logical_len) col = logical_len;

  if (col != r->gap_start) {
    gap_move(r, col);
  }

  int gap_size = r->gap_end - r->gap_start;
  if (gap_size < GAP_MIN) {
    gap_grow(r);
  }

  r->text[r->gap_start] = c;
  r->gap_start++;
  return;
}

void buffer_backspace_char(TextBuffer *buf, int row, int col) {
  Row *r = buf->rows[row];

  int logical_len = buffer_row_logical_len(r);
  if (col < 0) col = 0;
  if (col > logical_len) col = logical_len;

  if (r->gap_start < 0) {
    return;
  }
  if (r->gap_start != col) {
    gap_move(r, col);
  }
  r->gap_start--;
  return;
}
