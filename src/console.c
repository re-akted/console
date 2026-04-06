#define _POSIX_C_SOURCE 200809L

#include <redakt/console.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

void console_init(RConsole *con, RCVarRegistry *registry) {
  memset(con, 0, sizeof(*con));
  con->registry = registry;
}

void console_printf(RConsole *con, const char *fmt, ...) {
  char *line = con->output[(con->output_head + con->output_count) % R_CONSOLE_OUTPUT_MAX];

  va_list args;
  va_start(args, fmt);
  vsnprintf(line, R_CONSOLE_OUTPUT_LINE_MAX, fmt, args);
  va_end(args);

  // strip trailing newline — renderers add their own line breaks
  size_t len = strlen(line);
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[--len] = '\0';
  }

  if (con->output_count < R_CONSOLE_OUTPUT_MAX) {
    con->output_count++;
  } else {
    con->output_head = (con->output_head + 1) % R_CONSOLE_OUTPUT_MAX;
  }

  con->version++;
}

const char *console_output_line(RConsole *con, uint32_t index) {
  if (index >= con->output_count) {
    return NULL;
  }
  return con->output[(con->output_head + index) % R_CONSOLE_OUTPUT_MAX];
}

uint32_t console_output_count(RConsole *con) {
  return con->output_count;
}

RCVar *console_current_cvar(RConsole *con) {
  if (con->length == 0) {
    return NULL;
  }

  char name[256];
  uint32_t len = 0;
  while (len < con->length && con->line[len] != ' ' && con->line[len] != '=' && len < sizeof(name) - 1) {
    name[len] = con->line[len];
    len++;
  }
  name[len] = '\0';

  return cvar_find(con->registry, name);
}

uint32_t console_arg_index(RConsole *con) {
  uint32_t idx = 0;
  bool in_space = false;

  for (uint32_t i = 0; i < con->length; i++) {
    if (con->line[i] == ' ' || con->line[i] == '=') {
      if (!in_space) {
        idx++;
        in_space = true;
      }
    } else {
      in_space = false;
    }
  }

  return idx;
}

bool console_past_name(RConsole *con) {
  for (uint32_t i = 0; i < con->length; i++) {
    if (con->line[i] == ' ' || con->line[i] == '=') {
      return true;
    }
  }
  return false;
}

static void console_history_push(RConsole *con) {
  if (con->length == 0) {
    return;
  }

  if (con->history_count > 0 &&
      strcmp(con->history[(con->history_count - 1) % R_CONSOLE_HISTORY_MAX], con->line) == 0) {
    return;
  }

  memcpy(con->history[con->history_count % R_CONSOLE_HISTORY_MAX], con->line, con->length + 1);
  con->history_count++;
}

static void console_history_up(RConsole *con) {
  if (con->history_count == 0) {
    return;
  }

  if (!con->browsing_history) {
    memcpy(con->saved_line, con->line, con->length + 1);
    con->saved_length = con->length;
    con->history_index = con->history_count;
    con->browsing_history = true;
  }

  if (con->history_index == 0) {
    return;
  }

  con->history_index--;
  uint32_t idx = con->history_index % R_CONSOLE_HISTORY_MAX;
  size_t len = strlen(con->history[idx]);
  memcpy(con->line, con->history[idx], len + 1);
  con->length = (uint32_t)len;
  con->cursor = con->length;
  con->version++;
}

static void console_history_down(RConsole *con) {
  if (!con->browsing_history) {
    return;
  }

  con->history_index++;

  if (con->history_index >= con->history_count) {
    memcpy(con->line, con->saved_line, con->saved_length + 1);
    con->length = con->saved_length;
    con->cursor = con->length;
    con->browsing_history = false;
  } else {
    uint32_t idx = con->history_index % R_CONSOLE_HISTORY_MAX;
    size_t len = strlen(con->history[idx]);
    memcpy(con->line, con->history[idx], len + 1);
    con->length = (uint32_t)len;
    con->cursor = con->length;
  }

  con->version++;
}

static void console_submit(RConsole *con) {
  if (con->length > 0) {
    console_printf(con, "] %s", con->line);
    console_history_push(con);
    cvar_parse(con->registry, con->line);
  }

  con->line[0] = '\0';
  con->cursor = 0;
  con->length = 0;
  con->browsing_history = false;
  con->version++;
}

static void console_tab_complete(RConsole *con) {
  if (con->length == 0) {
    return;
  }

  char prefix[R_CONSOLE_LINE_MAX];
  uint32_t plen = 0;
  while (plen < con->length && con->line[plen] != ' ') {
    prefix[plen] = con->line[plen];
    plen++;
  }
  prefix[plen] = '\0';

  const char *results[R_CONSOLE_AUTOCOMPLETE_MAX];
  uint32_t count = cvar_autocomplete(con->registry, prefix, results, R_CONSOLE_AUTOCOMPLETE_MAX);

  if (count == 0) {
    return;
  }

  if (count == 1) {
    size_t len = strlen(results[0]);
    if (len < R_CONSOLE_LINE_MAX - 2) {
      memcpy(con->line, results[0], len);
      con->line[len] = ' ';
      con->line[len + 1] = '\0';
      con->cursor = (uint32_t)(len + 1);
      con->length = con->cursor;
    }
    con->version++;
    return;
  }

  for (uint32_t i = 0; i < count; i++) {
    console_printf(con, "  %s", results[i]);
  }

  size_t common = strlen(results[0]);
  for (uint32_t i = 1; i < count; i++) {
    size_t j = 0;
    while (j < common && results[0][j] == results[i][j]) {
      j++;
    }
    common = j;
  }

  if (common > plen) {
    memcpy(con->line, results[0], common);
    con->line[common] = '\0';
    con->cursor = (uint32_t)common;
    con->length = con->cursor;
  }

  con->version++;
}

static void console_insert(RConsole *con, char c) {
  if (con->length >= R_CONSOLE_LINE_MAX - 1) {
    return;
  }

  memmove(&con->line[con->cursor + 1], &con->line[con->cursor], con->length - con->cursor + 1);
  con->line[con->cursor] = c;
  con->cursor++;
  con->length++;
  con->version++;
}

static void console_backspace(RConsole *con) {
  if (con->cursor == 0) {
    return;
  }

  memmove(&con->line[con->cursor - 1], &con->line[con->cursor], con->length - con->cursor + 1);
  con->cursor--;
  con->length--;
  con->version++;
}

static void console_delete(RConsole *con) {
  if (con->cursor >= con->length) {
    return;
  }

  memmove(&con->line[con->cursor], &con->line[con->cursor + 1], con->length - con->cursor);
  con->length--;
  con->version++;
}

void console_key(RConsole *con, RConsoleKey key, char c) {
  switch (key) {
  case R_KEY_CHAR:          console_insert(con, c); break;
  case R_KEY_ENTER:         console_submit(con); break;
  case R_KEY_TAB:           console_tab_complete(con); break;
  case R_KEY_BACKSPACE:     console_backspace(con); break;
  case R_KEY_DELETE:        console_delete(con); break;
  case R_KEY_UP:            console_history_up(con); break;
  case R_KEY_DOWN:          console_history_down(con); break;
  case R_KEY_LEFT:
    if (con->cursor > 0) { con->cursor--; con->version++; }
    break;
  case R_KEY_RIGHT:
    if (con->cursor < con->length) { con->cursor++; con->version++; }
    break;
  case R_KEY_HOME:
    con->cursor = 0; con->version++;
    break;
  case R_KEY_END:
    con->cursor = con->length; con->version++;
    break;
  case R_KEY_KILL_LINE:
    con->line[con->cursor] = '\0';
    con->length = con->cursor;
    con->version++;
    break;
  case R_KEY_KILL_TO_START:
    memmove(con->line, &con->line[con->cursor], con->length - con->cursor + 1);
    con->length -= con->cursor;
    con->cursor = 0;
    con->version++;
    break;
  }
}

void console_input_char(RConsole *con, char c) {
  if (c == '\n' || c == '\r') {
    console_key(con, R_KEY_ENTER, 0);
  } else if (c == '\t') {
    console_key(con, R_KEY_TAB, 0);
  } else if (c == 127 || c == '\b') {
    console_key(con, R_KEY_BACKSPACE, 0);
  } else if (c == 1) {
    console_key(con, R_KEY_HOME, 0);
  } else if (c == 5) {
    console_key(con, R_KEY_END, 0);
  } else if (c == 4) {
    console_key(con, R_KEY_DELETE, 0);
  } else if (c == 11) {
    console_key(con, R_KEY_KILL_LINE, 0);
  } else if (c == 21) {
    console_key(con, R_KEY_KILL_TO_START, 0);
  } else if (c >= 32) {
    console_key(con, R_KEY_CHAR, c);
  }
}
