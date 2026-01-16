#include "../include/tui.h"
#include <unistd.h>

const int TAB_LEN = 4;
const int LINE_NUM_SIZE = 7;
const int VIEWPORT_THRESHOLD = 4;

void tui_run(EditorState *es) {
  setlocale(LC_ALL, "");
  struct notcurses_options ncopts = {0};
  ncopts.flags = NCOPTION_SUPPRESS_BANNERS;

  struct notcurses *nc = notcurses_init(&ncopts, NULL);
  if(!nc) return;

  unsigned int max_rows;
  notcurses_stddim_yx(nc, &max_rows, NULL);

  int vp_cur_col = 0;
  int vp_cur_row = 0;

  struct ncplane_options text_p_opts = {0};
  text_p_opts.rows = max_rows;
  text_p_opts.cols = 1024;
  text_p_opts.x = LINE_NUM_SIZE;

  struct ncplane_options nums_p_opts = {0};
  nums_p_opts.rows = max_rows;
  nums_p_opts.cols = LINE_NUM_SIZE;

  struct ncplane *std = notcurses_stdplane(nc);
  struct ncplane *text_plane = ncplane_create(std, &text_p_opts);
  struct ncplane *num_col_plane = ncplane_create(std, &nums_p_opts);

  ncplane_erase(num_col_plane);
  ncplane_set_fg_rgb(num_col_plane, 0x967218);
  draw_line_nums(es, num_col_plane, max_rows);

  ncplane_erase(text_plane);
  draw_screen(es, text_plane, max_rows);
  notcurses_cursor_enable(nc, 0, LINE_NUM_SIZE);
  notcurses_render(nc);

  while (es->main_loop_running) {
    struct ncinput ni = {0};
    uint32_t key = notcurses_get_blocking(nc, &ni);
    if (ni.evtype != NCTYPE_RELEASE) {
      enum key_t key_type = get_key_type(key);
      if (ncinput_ctrl_p(&ni)) {
        handle_ctrl_combo(es, key);
      } else if (key_type == WRITEABLE) {
        editor_insert_char(es, ni.eff_text[0]);
      } else if (key_type == MOVEMENT) {
        update_cursor_pos(es, key);
      } else if (key_type == FUNCTIONAL) {
        if (key == NCKEY_BACKSPACE) {
          editor_backspace_char(es);
        } else if (key == NCKEY_RETURN) {
          if (ncinput_shift_p(&ni)) {
            editor_create_row(es, es->cursor_row - 1);
          } else {
            editor_create_row(es, es->cursor_row);
          }
        } else if (key == NCKEY_TAB) {
          for (int i = 0; i < TAB_LEN; i++) {
            editor_insert_char(es, ' ');
          }
        }
      }
      ncplane_erase(text_plane);
      ncplane_erase(num_col_plane);
      vp_cur_col = es->cursor_col;
      vp_cur_row = es->cursor_row - es->scroll_offset;
      vp_scroll(es, vp_cur_row, max_rows);
      vp_cur_row = es->cursor_row - es->scroll_offset;
      draw_line_nums(es, num_col_plane, max_rows);
      draw_screen(es, text_plane, max_rows);
      notcurses_cursor_enable(nc, vp_cur_row, vp_cur_col + LINE_NUM_SIZE);
      notcurses_render(nc);
    }
  }
  notcurses_stop(nc);
  return;
}

enum key_t get_key_type(uint32_t key) {
  if (key >= NCKEY_UP && key <= NCKEY_LEFT) {
    return MOVEMENT;
  } else if (iswprint((wchar_t)key)) {
    return WRITEABLE;
  } else {
    return FUNCTIONAL;
  }
}

void update_cursor_pos(EditorState *es, uint32_t key) {
  switch (key) {
    case NCKEY_DOWN:
      es->cursor_row++;
      break;
    case NCKEY_UP:
      es->cursor_row--;
      break;
    case NCKEY_LEFT:
      es->cursor_col--;
      break;
    case NCKEY_RIGHT:
      es->cursor_col++;
      break;
    default:
      return;
  }

  if (es->cursor_row < 0) {
    es->cursor_row = 0;
  } else if (es->cursor_row > es->buffer->row_count - 1) {
    es->cursor_row = es->buffer->row_count - 1;
  }

  if (es->cursor_col < 0) {
    es->cursor_col = 0;
  } else if (editor_row_len(es, es->cursor_row) == 0) {
    es->cursor_col = 0;
  } else if (es->cursor_col > editor_row_len(es, es->cursor_row) - 1) { 
    es->cursor_col = editor_row_len(es, es->cursor_row) - 1;
  }
  return;
}

void handle_ctrl_combo(EditorState *es, uint32_t key) {
  switch (key) {
    case 'Q':
      es->main_loop_running = false;
      break;
    case 'W':
      editor_save_to_file(es);
      break;
    case 'S':
      es->main_loop_running = false;
      editor_save_to_file(es);
      break;
    default:
      return;
  }
}

void vp_scroll(EditorState *es, int vp_cur_row, int vp_row_max) {
  if (es->buffer->row_count >= vp_row_max) {
    if (vp_cur_row >= vp_row_max) {
      es->scroll_offset++;
    } else if (vp_cur_row <= VIEWPORT_THRESHOLD - 1) {
      es->scroll_offset--;
    }
    if (es->scroll_offset < 0) {
      es->scroll_offset = 0;
    }
  }
  return;
}

void draw_line_nums(EditorState *es, struct ncplane *p, int max_rows) {
  int n_row_nums = max_rows;
  if (es->buffer->row_count < max_rows) {
    n_row_nums = es->buffer->row_count;
  }
  int ln = es->scroll_offset;
  for (int i = 0; i < n_row_nums; i++) {
    ncplane_cursor_move_yx(p, i, 0);
    char *str_num = malloc(LINE_NUM_SIZE + 1);
    sprintf(str_num, "%u", ln + 1);
    ln++;
    ncplane_putstr(p, str_num);
    free(str_num);
  }
  return;
}

void draw_screen(EditorState *es, struct ncplane *p, int max_rows) {
  int y = 0;
  int n_drawable_rows = max_rows;
  if (es->buffer->row_count < max_rows) {
    n_drawable_rows = es->buffer->row_count;
  }
  for (int i = es->scroll_offset; i < (n_drawable_rows + es->scroll_offset); i++) {
    ncplane_cursor_move_yx(p, y++, 0);
    int len = editor_row_len(es, i) + 1;
    char *logical_text = malloc(len);
    editor_get_row_text(es, i, logical_text);
    logical_text[len - 1] = '\0';
    ncplane_putstr(p, logical_text);
    free(logical_text);
  }
}

