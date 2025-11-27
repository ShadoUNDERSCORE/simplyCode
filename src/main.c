#include <stdbool.h>
#include <stdio.h> 
#include <unistd.h> 

#include "../include/buffer.h"
#include "../include/editor.h"
#include "../include/tui.h"

void test();
void debug_print_row(Row *r);


int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "g")) != -1) {
    switch (opt) {
      case 'g':
        printf("No GUI Option Available Yet.\n");
        break;
      default:
        printf("Unknown Option: %c\n", optopt);
    }
  }

  // test();
  TextBuffer *buf = buffer_load("test.txt");

  EditorState *es = malloc(sizeof(EditorState));
  es->buffer = buf;
  es->filename = "test.txt";
  tui_run(es);


  // cleanup memory
  return 0;
}

void debug_print_row(Row *r) {
    int total = r->capacity;
    printf("[row capacity=%d gap_start=%d gap_end=%d]\n",
           r->capacity, r->gap_start, r->gap_end);

    for (int i = 0; i < total; i++) {
        unsigned char c = r->text[i];

        // Print printable ASCII normally, zeros as 00, others as hex
        if (c >= 32 && c <= 126)
            printf("%c ", c);
        else
            printf("%02X ", c);
    }

    printf("\n\n");
}

void test() {

  TextBuffer *buf = buffer_load("test.txt");

  char *new_text = "way ";
  for (int i = 0; i < 4; i++) {
    buffer_insert_char(buf, 7, 9+i, new_text[i]);
  }

  for (int i = 0; i < buf->row_count; i++) {
    debug_print_row(buf->rows[i]);
  } 

  buffer_save(buf, "test2.txt");
  buffer_backspace_char(buf, 7, 41);
  buffer_save(buf, "test3.txt");
  free(buf);
  buf = NULL;
}
