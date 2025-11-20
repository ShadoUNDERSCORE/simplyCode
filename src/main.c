#include <stdbool.h>
#include <stdio.h> 
#include <unistd.h> 

#include "../include/buffer.h"


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

  bool running = true;
  while (running) {
    // redraw tui state every itteration
    // handle tui input every itteration
    running = false;
  }

  // TEST CODE:

  TextBuffer *buf = buffer_load("test.txt");

  char *new_text = "way ";
  for (int i = 0; i < 4; i++) {
    buffer_insert_c(buf, 7, 9+i, new_text[i]);
  }

  for (int i = 0; i < buf->row_count; i++) {
    debug_print_row(buf->rows[i]);
  } 

  buffer_save(buf, "test2.txt");
  buffer_backspace_c(buf, 7, 41);
  buffer_save(buf, "test3.txt");
  free(buf);
  buf = NULL;
  // cleanup memory
  return 0;
}
