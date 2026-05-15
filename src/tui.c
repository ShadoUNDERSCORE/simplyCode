#include "../include/tui.h"
#include "../include/config.h"

#include <locale.h>
#include <unistd.h>

#define ESC_CMD_LEN 5
const int LINE_NUM_SIZE = 7;
const int VIEWPORT_THRESHOLD = 4;
int TAB_SIZE;
bool AUTO_INDENT;
bool AUTO_CLOSE;
const int AUTO_CLOSE_N_SYMBOLS = 5;

const uint32_t AUTO_CLOSE_OPENS[] = {'(', '[', '{', '\'', '"'};
const uint32_t AUTO_CLOSE_CLOSES[] = {')', ']', '}', '\'', '"'};

void tui_run(EditorState *es) {
  SettingsBucket **config = config_load();

  ConfigValue item_value;

  hashmap_get_value(config, "tab_size", &item_value);
  TAB_SIZE = item_value.i;

  hashmap_get_value(config, "auto_indent", &item_value);
  AUTO_INDENT = item_value.b;

  hashmap_get_value(config, "autoclose_brackets", &item_value);
  AUTO_CLOSE = item_value.b;

  setlocale(LC_ALL, "");
  struct notcurses_options ncopts = {0};
  ncopts.flags = NCOPTION_SUPPRESS_BANNERS;

  struct notcurses *nc = notcurses_init(&ncopts, NULL);
  if(!nc) return;
  printf("\x1b[?2004h");
  fflush(stdout);
  struct esc_cmd_t {
    char cmd[ESC_CMD_LEN];
    int len;
  };
  struct esc_cmd_t esc_cmd = {};

  unsigned int max_rows, max_cols;
  notcurses_stddim_yx(nc, &max_rows, &max_cols);

  int vp_cur_col = 0;
  int vp_cur_row = 0;

  struct ncplane_options text_p_opts = {0};
  text_p_opts.rows = max_rows;
  text_p_opts.cols = max_cols;
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

  CommandStack undo_stack = {0};
  CommandStack redo_stack = {0};
  history_stack_init(&undo_stack);
  history_stack_init(&redo_stack);
  CommandStage staged_cmd = {0};

  while (es->main_loop_running) {
    struct ncinput ni = {0};
    uint32_t key = notcurses_get_blocking(nc, &ni);
    if (ni.evtype != NCTYPE_RELEASE) {
      enum key_t key_type = get_key_type(key);
      if (ncinput_ctrl_p(&ni)) {
        handle_ctrl_combo(es, key, &undo_stack, &redo_stack, &staged_cmd.command);
      } else if (key_type == WRITEABLE || key == NCKEY_ESC) {
        if (key == NCKEY_ESC) {
          es->esc_mode = true;
        } else if (es->esc_mode == true) {
          FILE *f = fopen("log", "a"); fprintf(f, "%c\n", key); fclose(f);
          esc_cmd.cmd[esc_cmd.len] = (char)ni.eff_text[0];
          esc_cmd.len++;
          FILE *f2 = fopen("log", "a"); fprintf(f2, "%s\n", esc_cmd.cmd); fclose(f2);
          if (esc_cmd.len == ESC_CMD_LEN) {
            if (!strcmp(esc_cmd.cmd, "[200~")) {
              FILE *f1 = fopen("log", "a"); fprintf(f1, "PASTE_MODE\n"); fclose(f1);
              es->paste_mode = true;
              es->esc_mode = false;
              esc_cmd.len = 0;
            } else if (!strcmp(esc_cmd.cmd, "[201~")) {
              FILE *f3 = fopen("log", "a"); fprintf(f3, "EXT_PASTE_MODE\n"); fclose(f3);
              es->paste_mode = false;
              es->esc_mode = false;
              esc_cmd.len = 0;
            } else {
              // add esc_cmd to buffer
              es->esc_mode = false;
            }
          }
        } else {
          history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd,
                                                  es->cursor_row, es->cursor_col, INSERT, ni.eff_text[0]);
          if (AUTO_CLOSE && !es->paste_mode) {
            editor_insert_char(es, ni.eff_text[0]);
            for (int i = 0; i < AUTO_CLOSE_N_SYMBOLS; i++) {
              if (AUTO_CLOSE_OPENS[i] == ni.eff_text[0]) {
                history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd,
                                                        es->cursor_row, es->cursor_col, INSERT, AUTO_CLOSE_CLOSES[i]);
                editor_insert_char(es, AUTO_CLOSE_CLOSES[i]);
                es->cursor_col--;
              }
            }
          } else {
            editor_insert_char(es, ni.eff_text[0]);
          }
        }
      } else if (key_type == MOVEMENT) {
        update_cursor_pos(es, key);
      } else if (key_type == FUNCTIONAL) {
        if (key == NCKEY_BACKSPACE) {
          char *logical_text = malloc(editor_row_len(es, es->cursor_row));
          editor_get_row_text(es, es->cursor_row, logical_text);
          if (es->cursor_col == 0) {
            editor_delete_row(es);
            history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd, es->cursor_row,
                                                   es->cursor_col + 1, DELETE, '\n');
          } else {
            history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd, es->cursor_row,
                                                   es->cursor_col, DELETE, logical_text[es->cursor_col - 1]);
            editor_backspace_char(es);
          }
          free(logical_text);
        } else if (key == NCKEY_RETURN) {
          history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd,
                                                  es->cursor_row + 1, es->cursor_col, INSERT, '\n');
          if (ncinput_shift_p(&ni)) {
            editor_create_row(es, es->cursor_row - 1);
          } else {
            int prev_col = es->cursor_col;
            int prev_len = editor_row_len(es, es->cursor_row) - 1;
            editor_create_row(es, es->cursor_row);
            if (AUTO_INDENT && !es->paste_mode) {
              char *logical_text = malloc(prev_len);
              editor_get_row_text(es, es->cursor_row - 1, logical_text);
              if (prev_col == prev_len) {
                if (logical_text[prev_len - 1] == '{' || logical_text[prev_len - 1] == ':') {
                  indent(es, &undo_stack, &redo_stack, &staged_cmd);
                }
              } // else if (prev_col == prev_len - 1) {
                // if (logical_text[prev_len - 2] == '{') {
                //     indent(es, &undo_stack, &redo_stack, &staged_cmd);
                //     history_update_and_check_staged_command(&undo_stack, &redo_stack, &staged_cmd, es->cursor_row + 1, 0, INSERT, '\n');
                //     editor_create_row(es, es->cursor_row);
                //     es->cursor_row--;
                //     es->cursor_col += TAB_SIZE;
                // }
              // }
              int sp = 0;
              while (logical_text[sp] == ' ') {
                sp++;
              }
              int n_indents = sp / TAB_SIZE;
              for (int i = 0; i < n_indents; i++) {
                indent(es, &undo_stack, &redo_stack, &staged_cmd);
              }
              free(logical_text);
            }
          }
        } else if (key == NCKEY_TAB) {
          indent(es, &undo_stack, &redo_stack, &staged_cmd);
        }
      }
      ncplane_erase(text_plane);
      ncplane_erase(num_col_plane);
      vp_v_scroll(es, &vp_cur_row, max_rows);
      vp_h_scroll(es, &vp_cur_col, max_cols);
      draw_line_nums(es, num_col_plane, max_rows);
      draw_screen(es, text_plane, max_rows);
      notcurses_cursor_enable(nc, vp_cur_row, vp_cur_col);
      notcurses_render(nc);
    }
  }
  printf("\x1b[?2004l");
  fflush(stdout);
  notcurses_stop(nc);
  if (staged_cmd.command.data) free(staged_cmd.command.data);
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

void handle_ctrl_combo(EditorState *es, uint32_t key, CommandStack *undo_stack, CommandStack *redo_stack,
                       InputCommand *staged_cmd) {
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
    case 'Z':
      if (staged_cmd->data) history_add_command(undo_stack, redo_stack, staged_cmd);
      if (undo_stack->len > 0) history_undo(es, undo_stack, redo_stack);
      break;
    case 'R':
      if (redo_stack->len > 0) history_redo(es, undo_stack, redo_stack);
      break;
    default:
      return;
  }
  return;
}

void vp_v_scroll(EditorState *es, int *vp_cur_row, int vp_row_max) {
  *vp_cur_row = es->cursor_row - es->v_scroll_offset;
  if (es->buffer->row_count >= vp_row_max) {
    if (*vp_cur_row >= vp_row_max) {
      es->v_scroll_offset++;
    } else if (*vp_cur_row <= VIEWPORT_THRESHOLD - 1) {
      es->v_scroll_offset--;
    }
    if (es->v_scroll_offset < 0) {
      es->v_scroll_offset = 0;
    }
  }
  *vp_cur_row = es->cursor_row - es->v_scroll_offset;
  return;
}

void vp_h_scroll(EditorState *es, int *vp_cur_col, int vp_col_max) {
  *vp_cur_col = es->cursor_col - es->h_scroll_offset + LINE_NUM_SIZE;
  if (editor_row_len(es, es->cursor_row) >= vp_col_max - LINE_NUM_SIZE) {
    if (*vp_cur_col >= vp_col_max) {
      es->h_scroll_offset++;
    } else if (*vp_cur_col <= LINE_NUM_SIZE) {
      es->h_scroll_offset--;
    }
    if (es->h_scroll_offset < 0 || editor_row_len(es, es->cursor_row) < vp_col_max - LINE_NUM_SIZE) {
      es->h_scroll_offset = 0;
    }
  } else {
    es->h_scroll_offset = 0;
  }
  *vp_cur_col = es->cursor_col - es->h_scroll_offset + LINE_NUM_SIZE;
  return;
}

void draw_line_nums(EditorState *es, struct ncplane *p, int max_rows) {
  int n_row_nums = max_rows;
  if (es->buffer->row_count < max_rows) {
    n_row_nums = es->buffer->row_count;
  }
  int ln = es->v_scroll_offset;
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
  for (int i = es->v_scroll_offset; i < (n_drawable_rows + es->v_scroll_offset); i++) {
    ncplane_cursor_move_yx(p, y++, 0);
    int len = editor_row_len(es, i) + 1;
    char *logical_text = malloc(len);
    editor_get_row_text(es, i, logical_text);
    logical_text[len - 1] = '\0';
    int shift_left_to = es->h_scroll_offset < len ? es->h_scroll_offset : len - 1;
    memmove(&logical_text[0], &logical_text[shift_left_to], len - 1);
    ncplane_putstr(p, logical_text);
    free(logical_text);
  }
}

void indent(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack, CommandStage *staged_cmd) {
  for (int i = 0; i < TAB_SIZE; i++) {
    history_update_and_check_staged_command(undo_stack, redo_stack, staged_cmd, es->cursor_row, es->cursor_col, INSERT, ' ');
    editor_insert_char(es, ' ');
  }
}

