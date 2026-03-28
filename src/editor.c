// FILE *f = fopen("log", "a"); fprintf(f, "HERE..."); fclose(f);
#include "../include/editor.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _init_staged_command(CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type);
void _stack_push(CommandStack *stack, InputCommand *new_cmd);
void _stack_pop(CommandStack *stack);

void editor_save_to_file(EditorState *es) {
  buffer_save(es->buffer, es->filename);
  return;
}

void editor_create_row(EditorState *es, int preceeding_row) {
  buffer_create_row(es->buffer, preceeding_row, es->cursor_col);
  es->cursor_row = preceeding_row + 1;
  es->cursor_col = 0;
  return;
}

void editor_delete_row(EditorState *es) {
  if (es->cursor_row > 0) {
    int len = editor_row_len(es, es->cursor_row);
    buffer_delete_row(es->buffer, es->cursor_row);
    es->cursor_row--;
    es->cursor_col = editor_row_len(es, es->cursor_row) - len;
  }
  return;
}

void editor_insert_char(EditorState *es, char c) {
  buffer_insert_char(es->buffer, es->cursor_row, es->cursor_col, c);
  es->cursor_col++;
  return;
}

void editor_delete_char(EditorState *es) {
  es->cursor_col++;
  buffer_backspace_char(es->buffer, es->cursor_row, es->cursor_col);
  return;
}

void editor_backspace_char(EditorState *es) {
  if (es->cursor_col > 0) {
    buffer_backspace_char(es->buffer, es->cursor_row, es->cursor_col);
    es->cursor_col = es->buffer->rows[es->cursor_row]->gap_start;
  }
  return;
}

int editor_row_len(EditorState *es, int row) {
  return buffer_row_logical_len(es->buffer->rows[row]);
}

void editor_get_row_text(EditorState *es, int row, char *logical_text) {
  buffer_get_row_text(es->buffer, row, logical_text);
  return;
}

// HISTORY

void history_stack_init(CommandStack *stack) {
  stack->len = 0;
  stack->top = -1;
  return;
}

void _stack_push(CommandStack *stack, InputCommand *new_cmd) {
  stack->top = (stack->top + 1) % STACK_SIZE;
  if (stack->len < STACK_SIZE) stack->len++;

  int index = stack->top;
  stack->stack[index].type = new_cmd->type;
  stack->stack[index].data_len = new_cmd->data_len;
  stack->stack[index].row = new_cmd->row;
  stack->stack[index].col_start = new_cmd->col_start;
  stack->stack[index].col_end = new_cmd->col_end;
  if (stack->stack[index].data == NULL) {
    stack->stack[index].data = malloc(new_cmd->data_len);
  } else {
    free(stack->stack[index].data);
    stack->stack[index].data = malloc(new_cmd->data_len);
  }
  for (int i = 0; i < new_cmd->data_len; i++) {
    stack->stack[index].data[i] = new_cmd->data[i];
  }
  return;
}

void _stack_pop(CommandStack *stack) {
  if (stack->len == 0) {
    return;
  }
  stack->top = (stack->top - 1) % STACK_SIZE;
  if (stack->top < 0) {
    stack->top = stack->len;
  }
  stack->len--;
  return;
}

void _init_staged_command(CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type) {
  staged_cmd->command.type = cmd_type;
  staged_cmd->command.data_len = 0;
  staged_cmd->command.row = cur_row;
  staged_cmd->command.col_start = cur_col;
  staged_cmd->command.col_end = cur_col;
  if (staged_cmd->command.data) free(staged_cmd->command.data);
  staged_cmd->command.data = malloc(sizeof(char) * 64);
  staged_cmd->last_action = time(NULL);
  return;
}

void history_add_command(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *new_cmd) {
  memset(redo_stack->stack, 0, (STACK_SIZE * sizeof(InputCommand *)));
  _stack_push(undo_stack, new_cmd);
  new_cmd->type = 0;
  new_cmd->data_len = 0;
  new_cmd->data_cap = 0;
  new_cmd->row = 0;
  new_cmd->col_start = 0;
  new_cmd->col_end = 0;
  free(new_cmd->data);
  new_cmd->data = NULL;
  return;
}

void history_update_and_check_staged_command(CommandStack *undo_stack, CommandStack *redo_stack,
                                             CommandStage *staged_cmd, int cur_row, int cur_col,
                                             CMDType cmd_type, char new_char) {
  // INIT staged command if blank
  bool is_init = false;
  if (staged_cmd->command.data_len == 0) {
    _init_staged_command(staged_cmd, cur_row, cur_col, cmd_type);
    is_init = true;
  }
  // Update staged command if meets all reqs
  int next_col = staged_cmd->command.type == INSERT ? staged_cmd->command.col_end + 1 : staged_cmd->command.col_end - 1;
  if ((
    staged_cmd->command.data[0] != '\n' &&
    cur_row == staged_cmd->command.row &&
    cur_col == next_col &&
    cmd_type == staged_cmd->command.type &&
    time(NULL) < (staged_cmd->last_action + 5
    )) ||
    is_init
  ) {
    if (staged_cmd->command.data_cap < staged_cmd->command.data_len + 1) {
      staged_cmd->command.data_cap += 16;
      staged_cmd->command.data = realloc(staged_cmd->command.data, sizeof(char) * staged_cmd->command.data_cap);
    }
    staged_cmd->command.data[staged_cmd->command.data_len] = new_char;
    staged_cmd->command.data_len++;
    staged_cmd->command.col_end = cur_col;
    staged_cmd->last_action = time(NULL);
  } else {
    history_add_command(undo_stack, redo_stack, &staged_cmd->command);
    _init_staged_command(staged_cmd, cur_row, cur_col, cmd_type);
    if (staged_cmd->command.data_cap < staged_cmd->command.data_len + 1) {
      staged_cmd->command.data_cap += 16;
      staged_cmd->command.data = realloc(staged_cmd->command.data, sizeof(char) * staged_cmd->command.data_cap);
    }
    staged_cmd->command.data[staged_cmd->command.data_len] = new_char;
    staged_cmd->command.data_len++;
    if (new_char == '\n') staged_cmd->command.col_end = -1;
  }
}

void history_undo(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack) {
  if (undo_stack->stack[undo_stack->top].data_len < 1) return;

  InputCommand history_data = undo_stack->stack[undo_stack->top];
  if (history_data.type == INSERT) {
    es->cursor_row = history_data.row;
    es->cursor_col = history_data.col_end + 1;
    if (history_data.data[0] == '\n') {
      editor_delete_row(es);
    } else {
      int size = history_data.col_end - history_data.col_start + 1;
      for (; size > 0; size--) {
        editor_backspace_char(es);
      }
    }
  } else {
    es->cursor_row = history_data.row;
    es->cursor_col = history_data.col_end - 1;
    if (history_data.data[0] == '\n') {
      editor_create_row(es, es->cursor_row);
    } else {
      for (int i = history_data.data_len - 1; i > -1; i--) {
        editor_insert_char(es, history_data.data[i]);
      }
    }
  }

  // Flip CMDType Before adding to redo_stack.
  undo_stack->stack[undo_stack->top].type = undo_stack->stack[undo_stack->top].type == INSERT ? DELETE : INSERT;
  _stack_push(redo_stack, &undo_stack->stack[undo_stack->top]);
  _stack_pop(undo_stack);
  return;
}

void history_redo(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack) {
  if (redo_stack->stack[redo_stack->top].data_len < 1) return;

  InputCommand history_data = redo_stack->stack[redo_stack->top];
  if (history_data.type == INSERT) {
    es->cursor_row = history_data.row;
    es->cursor_col = history_data.col_start;
    if (history_data.data[0] == '\n') {
      int size = history_data.col_start - history_data.col_end + 1;
      for (; size > 0; size--) {
        editor_backspace_char(es);
      }
      editor_delete_row(es);
    } else {
      int size = history_data.col_start - history_data.col_end + 1;
      for (; size > 0; size--) {
        editor_backspace_char(es);
      }
    }
  } else {
    es->cursor_row = history_data.row;
    es->cursor_col = history_data.col_start;
    if (history_data.data[0] == '\n') {
      es->cursor_row = history_data.row - 1;
      editor_create_row(es, es->cursor_row);
    } else {
      for (int i = 0; i < history_data.data_len; i++) {
        editor_insert_char(es, history_data.data[i]);
      }
    }
  }

  redo_stack->stack[redo_stack->top].type = redo_stack->stack[redo_stack->top].type == INSERT ? DELETE : INSERT;
  _stack_push(undo_stack, &redo_stack->stack[redo_stack->top]);
  _stack_pop(redo_stack);
}

