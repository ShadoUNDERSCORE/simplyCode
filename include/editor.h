#ifndef EDITOR_H
#define EDITOR_H

#include <time.h>

#include "buffer.h"

#define STACK_SIZE 100

// BUFFER MANAGEMENT
typedef struct {
  TextBuffer *buffer;
  int cursor_row;
  int cursor_col;
  int v_scroll_offset;
  int h_scroll_offset;
  bool dirty;
  char *filename;
  bool main_loop_running;
} EditorState;

// HISTORY MANAGEMENT
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
  int len;
  int top;
} CommandStack;

typedef struct {
  time_t last_action;
  InputCommand command;
} CommandStage;

// BUFFER API
void editor_save_to_file(EditorState *es);
void editor_create_row(EditorState *es, int preceeding_row);
void editor_delete_row(EditorState *es);
void editor_insert_char(EditorState *es, char c);
void editor_delete_char(EditorState *es);
void editor_backspace_char(EditorState *es);
void editor_move_cursor(EditorState *es, int row, int col);
int editor_row_len(EditorState *es, int row);
void editor_get_row_text(EditorState *es, int row, char *logical_text);

// HISTORY MANAGEMENT (Undo/Redo)
void history_stack_init(CommandStack *stack);
void history_update_and_check_staged_command(CommandStack *undo_stack, CommandStack *redo_stack, CommandStage *staged_cmd, int cur_row, int cur_col, CMDType cmd_type, char new_char);
void history_add_command(CommandStack *undo_stack, CommandStack *redo_stack, InputCommand *new_cmd);
void history_undo(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack);
void history_redo(EditorState *es, CommandStack *undo_stack, CommandStack *redo_stack);

#endif

