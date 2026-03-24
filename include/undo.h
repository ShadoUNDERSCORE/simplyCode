#ifndef UNDO_H
#define UNDO_H

#include <time.h>

#define STACK_SIZE 100

typedef enum {
  INSERT,
  DELETE
} CMDType;

typedef struct {
  CMDType type;
  char *data;
  int data_cap;
  int data_len;
  int row;
  int col_start;
  int col_end;
} InputCommand;

typedef struct {
  InputCommand stack[STACK_SIZE];
  int capacity;
  int len;
  int top;
} CommandStack;

typedef struct {
  time_t last_action;
  InputCommand command;
} CommandStage;

void manage_staged_command(CommandStack *undo_stack, CommandStack *redo_stack, CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type, char new_char);
void init_staged_command(CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type);
void add_command(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *new_cmd);
void stack_init(CommandStack *stack);
void stack_push(CommandStack *stack, InputCommand *new_cmd);
void stack_pop(CommandStack *stack);
void undo(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *cmd_out);
// void redo(CommandStack *undo_stack, CommandStack *redo_stack);

#endif
