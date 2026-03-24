// FILE *f = fopen("log", "a"); fprintf(f, "HERE..."); fclose(f);

#include "../include/undo.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void stack_init(CommandStack *stack) {
  stack->capacity = STACK_SIZE;
  stack->len = 0;
  stack->top = 0;
  return;
}

void manage_staged_command(CommandStack *undo_stack, CommandStack *redo_stack, CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type, char new_char) {
  // INIT staged command if blank
  bool is_init = false;
  if (staged_cmd->command.data_len == 0) {
    init_staged_command(staged_cmd, cur_row, cur_col, cmd_type);
    is_init = true;
  }
  // Update staged command if meets all reqs
  if ((cur_row == staged_cmd->command.row &&
      (cur_col == (staged_cmd->command.col_end + 1) || cur_col == (staged_cmd->command.col_end - 1)) &&
      cmd_type == staged_cmd->command.type &&
      time(NULL) < (staged_cmd->last_action + 5)) ||
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
    add_command(undo_stack, redo_stack, &staged_cmd->command);
    init_staged_command(staged_cmd, cur_row, cur_col, cmd_type);
    if (staged_cmd->command.data_cap < staged_cmd->command.data_len + 1) {
      staged_cmd->command.data_cap += 16;
      staged_cmd->command.data = realloc(staged_cmd->command.data, sizeof(char) * staged_cmd->command.data_cap);
    }
    staged_cmd->command.data[staged_cmd->command.data_len] = new_char;
    staged_cmd->command.data_len++;
    if (new_char == '\n') staged_cmd->command.col_end = -1;
  }
}

void init_staged_command(CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type) {
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

void stack_push(CommandStack *stack, InputCommand *new_cmd) {
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

void stack_pop(CommandStack *stack) {
  if (stack->len == 0) {
    return;
  }
  stack->top = (stack->top - 1) % STACK_SIZE;
  stack->len--;
  return;
}

void add_command(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *new_cmd) {
  memset(redo_stack, 0, sizeof(*redo_stack));
  stack_push(undo_stack, new_cmd);
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

void undo(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *cmd_out) {
  if (undo_stack->stack[undo_stack->top].data_len < 1) return;

  cmd_out->type = undo_stack->stack[undo_stack->top].type;
  cmd_out->data_len = undo_stack->stack[undo_stack->top].data_len;
  cmd_out->row = undo_stack->stack[undo_stack->top].row;
  cmd_out->col_start = undo_stack->stack[undo_stack->top].col_start;
  cmd_out->col_end = undo_stack->stack[undo_stack->top].col_end;

  cmd_out->data = malloc(cmd_out->data_len);
  for (int i = 0; i < cmd_out->data_len; i++) {
    cmd_out->data[i] = undo_stack->stack[undo_stack->top].data[i];
  }

  // Flip CMDType Before adding to redo_stack.
  undo_stack->stack[undo_stack->top].type = undo_stack->stack[undo_stack->top].type == INSERT ? DELETE : INSERT;
  stack_push(redo_stack, &undo_stack->stack[undo_stack->top]);
  stack_pop(undo_stack);
  return;
}

// void redo(CommandStack *undo_stack, CommandStack *redo_stack) {
//
// }

