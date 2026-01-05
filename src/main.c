#include <stdbool.h>
#include <stdio.h> 
#include <unistd.h> 

#include "../include/buffer.h"
#include "../include/editor.h"
#include "../include/tui.h"

void test();
void debug_print_row(Row *r);


int main(int argc, char *argv[]) {
  // int opt;
  // while ((opt = getopt(argc, argv, "g")) != -1) {
  //   switch (opt) {
  //     case 'g':
  //       printf("No GUI Option Available Yet.\n");
  //       break;
  //     default:
  //       printf("Unknown Option: %c\n", optopt);
  //   }
  // }

  // test();
  if (argc == 2) {
    TextBuffer *buf = buffer_load(argv[1]);

    EditorState *es = malloc(sizeof(EditorState));
    es->cursor_row = 0;
    es->cursor_col = 0;
    es->buffer = buf;
    es->filename = argv[1];
    es->main_loop_running = true;
    tui_run(es);
  }


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
  for (int i = 0; i < buf->row_count; i++) {
    debug_print_row(buf->rows[i]);
  } 
  free(buf);
  buf = NULL;
}
