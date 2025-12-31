#include "../include/tui.h"
#include <unistd.h>

void tui_run(EditorState *es) {
  setlocale(LC_ALL, "");
  struct notcurses_options ncopts = {0};
  ncopts.flags = NCOPTION_SUPPRESS_BANNERS;

  struct notcurses *nc = notcurses_init(&ncopts, NULL);
  if(!nc) return;

  struct ncplane_options p_opts = {0};
  p_opts.flags = NCPLANE_OPTION_VSCROLL;
  p_opts.rows = es->buffer->row_count + 1;
  p_opts.cols = 1024;

  struct ncplane *std = notcurses_stdplane(nc);
  struct ncplane *ed_plane = ncplane_create(std, &p_opts);

  ncplane_erase(ed_plane);
  draw_screen(es, ed_plane);
  ncplane_cursor_move_yx(ed_plane, 0, 0);
  notcurses_cursor_enable(nc, 0, 0);
  notcurses_render(nc);

  bool running = true;
  while (running) {
    struct ncinput ni = {0};
    uint32_t key = notcurses_get_blocking(nc, &ni);
    if (ni.evtype != NCTYPE_RELEASE) {
      if (key == 'q') running = false;
      if (key == 'd') editor_backspace_char(es);
      ncplane_erase(ed_plane);
      draw_screen(es, ed_plane);
      update_cursor_pos(es, key);
      ncplane_cursor_move_yx(ed_plane, es->cursor_row, es->cursor_col);
      notcurses_cursor_enable(nc, es->cursor_row, es->cursor_col);
      notcurses_render(nc);
    }
  }
  notcurses_stop(nc);
  return;
}

// TODO:Solve Random Characters appearing
void update_cursor_pos(EditorState *es, char key) {
  switch (key) {
    case 'j':
      es->cursor_row++;
      break;
    case 'k':
      es->cursor_row--;
      break;
    case 'h':
      es->cursor_col--;
      break;
    case 'l':
      es->cursor_col++;
      break;
    default:
      return;
  }
  if (es->cursor_row < 0) es->cursor_row = 0;
  if (es->cursor_col < 0) es->cursor_col = 0;

  if (es->cursor_row > es->buffer->row_count -1) {
    es->cursor_row = es->buffer->row_count -1;
  }
  if (es->cursor_col > editor_row_len(es, es->cursor_row) -1) { 
    es->cursor_col = editor_row_len(es, es->cursor_row) -1;
  }
  return;
}

void draw_screen(EditorState *es, struct ncplane *p) {
  ncplane_cursor_move_yx(p, 0, 0);
  for (int i = 0; i < es->buffer->row_count; i++) {
    int len = editor_row_len(es, i) + 1;
    char *logical_text = malloc(len);
    editor_get_row_text(es, i, logical_text);
    logical_text[len - 1] = '\0';
    ncplane_putstr(p, logical_text);
    free(logical_text);
  }
}

