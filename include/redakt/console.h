#pragma once

#include <redakt/cvar.h>

#define R_CONSOLE_LINE_MAX 512
#define R_CONSOLE_HISTORY_MAX 128
#define R_CONSOLE_OUTPUT_MAX 1024
#define R_CONSOLE_OUTPUT_LINE_MAX 256
#define R_CONSOLE_AUTOCOMPLETE_MAX 64

typedef enum RConsoleKey {
  R_KEY_CHAR,
  R_KEY_ENTER,
  R_KEY_TAB,
  R_KEY_BACKSPACE,
  R_KEY_DELETE,
  R_KEY_LEFT,
  R_KEY_RIGHT,
  R_KEY_UP,
  R_KEY_DOWN,
  R_KEY_HOME,
  R_KEY_END,
  R_KEY_KILL_LINE,
  R_KEY_KILL_TO_START,
} RConsoleKey;

typedef struct RConsole {
  RCVarRegistry *registry;

  // line editing
  char line[R_CONSOLE_LINE_MAX];
  uint32_t cursor;
  uint32_t length;

  // history
  char history[R_CONSOLE_HISTORY_MAX][R_CONSOLE_LINE_MAX];
  uint32_t history_count;
  uint32_t history_index;
  char saved_line[R_CONSOLE_LINE_MAX];
  uint32_t saved_length;
  bool browsing_history;

  // output log (ring buffer)
  char output[R_CONSOLE_OUTPUT_MAX][R_CONSOLE_OUTPUT_LINE_MAX];
  uint32_t output_head;
  uint32_t output_count;

  // dirty flag: set when state changes, cleared by renderers after reading
  uint32_t version;
} RConsole;

void console_init(RConsole *con, RCVarRegistry *registry);

// input
void console_key(RConsole *con, RConsoleKey key, char c);
void console_input_char(RConsole *con, char c);

// output
void console_printf(RConsole *con, const char *fmt, ...);
const char *console_output_line(RConsole *con, uint32_t index);
uint32_t console_output_count(RConsole *con);

// query
RCVar *console_current_cvar(RConsole *con);
uint32_t console_arg_index(RConsole *con);
bool console_past_name(RConsole *con);
