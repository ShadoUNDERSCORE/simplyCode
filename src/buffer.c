#include "../include/buffer.h"

TextBuffer *buffer_load(const char *path) {

  enum break_state {OK, LEVEL1_ERR, LEVEL2_ERR};
  enum break_state health = OK;
  
  FILE *stream = fopen(path, "r");
  if (!stream) {return NULL;}

  int init_capacity = 128;
  TextBuffer *buf = malloc(sizeof(TextBuffer));
  buf->rows = malloc(sizeof(Row*) * init_capacity);
  if (!buf->rows) {health = LEVEL1_ERR;}
  buf->row_count = 0;
  buf->capacity = init_capacity;

  const int BASE_ALLOC = 128;
  char *s = malloc(BASE_ALLOC + 1);
  if (!s) {health = LEVEL1_ERR;}

  if (health == OK) {
    while (fgets(s, BASE_ALLOC, stream)) { 
      int len = strlen(s);
      int buffer_size = BASE_ALLOC;
      if (len > 0) {
        while (s[len - 1] != '\n') {
          buffer_size += BASE_ALLOC;
          s = realloc(s, buffer_size + 1);
          if (!s) {health = LEVEL1_ERR; break;}
          if (!fgets(s + len, buffer_size - len + 1, stream)) {
            len += strlen(s + len);
            break;
          }
          len += strlen(s + len);
        }
      }
      if (health != OK) {break;}

      int init_gap = len / 2 + 16; // TODO: make this smarter.
      int row_size = len + init_gap;

      Row *new_row = malloc(sizeof(Row));
      if (!new_row) {health = LEVEL2_ERR; break;}
      new_row->text = malloc(row_size);
      if (!new_row->text) {health = LEVEL2_ERR; break;}
      strcpy(new_row->text, s);
      memset(new_row->text + len, 0, row_size - len);
      new_row->gap_start = len;
      new_row->gap_end = row_size;
      new_row->capacity = row_size;
      
      if (buf->row_count == buf->capacity - 1) {
        const int N_NEW_LINES = 64;
        buf->rows = realloc(buf->rows, (buf->capacity + N_NEW_LINES) * sizeof(Row*));
        if (!buf->rows) {health = LEVEL2_ERR; break;}
        buf->capacity += N_NEW_LINES;
      }
      buf->rows[buf->row_count] = new_row;
      buf->row_count++;

    }
  }

  if (health == LEVEL1_ERR) {
    buffer_free(buf);
    fclose(stream);
    return NULL;
  } else if (health == LEVEL2_ERR) {
    buffer_free(buf);
    free(s);
    s = NULL;
    fclose(stream);
    return NULL;
  } else {
    free(s);
    s = NULL;
    fclose(stream);
    return buf;
  }
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
  for (int i = 0; i < buf->row_count; i++) {
    int prefix_len = buf->rows[i]->gap_start;
    char *prefix_ptr = buf->rows[i]->text;

    int tail_len = buf->rows[i]->capacity - buf->rows[i]->gap_end;
    char *tail_ptr = buf->rows[i]->text + buf->rows[i]->gap_end;

    fprintf(f, "%.*s", prefix_len, prefix_ptr);
    fprintf(f, "%.*s", tail_len, tail_ptr);
  }
  fclose(f);
  return;
}

static int logical_to_physical(Row *row, int logical_index) {
  if (logical_index <= row->gap_start) {
    return logical_index;
  }
  return logical_index + (row->gap_end - row->gap_start);
}

void gap_move(Row *row, int logical_index) {
  int index = logical_to_physical(row, logical_index);

  if (index > row->gap_start) {
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

void buffer_insert_c(TextBuffer *buf, int row, int col, char c) {
  const int GAP_MIN = 8;
  Row *r = buf->rows[row];

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

void buffer_backspace_c(TextBuffer *buf, int row, int col) {
  Row *r = buf->rows[row];

  if (r->gap_start == 0) {
    return;
  }
  if (r->gap_start != col) {
    gap_move(r, col);
  }
  r->gap_start--;
  return;
}
