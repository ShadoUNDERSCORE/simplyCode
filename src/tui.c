#include "../include/tui.h"

#include <unistd.h>

void tui_run(EditorState *es) {
  setlocale(LC_ALL, "");
  struct notcurses *nc = notcurses_init(NULL, NULL);
  if(!nc) return;

  struct ncplane_options p_opts = {0};
  p_opts.flags = NCPLANE_OPTION_VSCROLL;
  p_opts.rows = es->buffer->row_count + 1;
  p_opts.cols = 1024;

  struct ncplane *std = notcurses_stdplane(nc);
  struct ncplane *ed_plane = ncplane_create(std, &p_opts);

  for (int i = 0; i < es->buffer->row_count; i++) {
    ncplane_putstr(ed_plane, es->buffer->rows[i]->text);
  }

  notcurses_render(nc);
  sleep(10);
  notcurses_stop(nc);

}
