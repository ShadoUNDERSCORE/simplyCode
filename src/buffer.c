#include "../include/buffer.h"

const int N_NEW_LINES = 64;
const int INIT_CAPACITY = 128;

TextBuffer *buffer_load(const char *path) {

  bool healthy = true;

  FILE *stream = fopen(path, "r");
  if (!stream) {return NULL;}

  TextBuffer *buf = malloc(sizeof(TextBuffer));
  buf->rows = malloc(sizeof(Row *) * INIT_CAPACITY);
  if (!buf->rows) healthy = false;
  buf->row_count = 0;
  buf->capacity = INIT_CAPACITY;

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
    new_row->gap_end   = row_size;
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

  // TODO: Fix free() invalid size error
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

void buffer_create_row(TextBuffer *buf, int preceeding_row) {
  // Shift rows if not at end of array
  if (buf->capacity > buf->row_count + 8) {
    memmove(&buf->rows[preceeding_row + 2], &buf->rows[preceeding_row + 1], buf->capacity - preceeding_row);
  } else {
    // Reallocate if necissary
    buf->rows = realloc(buf->rows, (buf->capacity + N_NEW_LINES) * sizeof(Row *));
    buf->capacity += N_NEW_LINES;
    memmove(&buf->rows[preceeding_row + 2], &buf->rows[preceeding_row + 1], buf->capacity - preceeding_row);
  }
  Row *new_row = malloc(sizeof(Row));
  new_row->text = malloc(INIT_CAPACITY);
  new_row->gap_start = 0;
  new_row->gap_end = INIT_CAPACITY - 1;
  new_row->capacity = INIT_CAPACITY;
  buf->rows[preceeding_row + 1] = new_row;
  buf->row_count++;
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
