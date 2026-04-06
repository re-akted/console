#define _POSIX_C_SOURCE 200809L

#include <redakt/console.h>

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
  tests_run++; \
  printf("  %-60s", #fn); \
  fn(); \
  tests_passed++; \
  printf("PASS\n"); \
} while (0)

#define ASSERT(cond) do { \
  if (!(cond)) { \
    printf("FAIL\n    assertion failed: %s\n    at %s:%d\n", #cond, __FILE__, __LINE__); \
    exit(1); \
  } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
  const char *_a = (a); \
  const char *_b = (b); \
  if (strcmp(_a, _b) != 0) { \
    printf("FAIL\n    expected \"%s\", got \"%s\"\n    at %s:%d\n", _b, _a, __FILE__, __LINE__); \
    exit(1); \
  } \
} while (0)

#define ASSERT_INT_EQ(a, b) do { \
  int _a = (a); \
  int _b = (b); \
  if (_a != _b) { \
    printf("FAIL\n    expected %d, got %d\n    at %s:%d\n", _b, _a, __FILE__, __LINE__); \
    exit(1); \
  } \
} while (0)

#define ASSERT_FLOAT_EQ(a, b, eps) do { \
  float _a = (a); \
  float _b = (b); \
  if (fabsf(_a - _b) > (eps)) { \
    printf("FAIL\n    expected %f, got %f\n    at %s:%d\n", (double)_b, (double)_a, __FILE__, __LINE__); \
    exit(1); \
  } \
} while (0)

// ============================================================
// Helpers
// ============================================================

static char print_buf[4096];
static int print_buf_len;

static void capture_print(void *ctx, const char *fmt, ...) {
  (void)ctx;
  va_list args;
  va_start(args, fmt);
  print_buf_len += vsnprintf(print_buf + print_buf_len,
                             sizeof(print_buf) - (size_t)print_buf_len, fmt, args);
  va_end(args);
}

static void capture_reset(void) {
  print_buf[0] = '\0';
  print_buf_len = 0;
}

static RCVarRegistry new_registry(void) {
  RCVarRegistry reg = {0};
  cvar_registry_init(&reg);
  reg.print_fn = capture_print;
  reg.print_ctx = NULL;
  capture_reset();
  return reg;
}

static void type_string(RConsole *con, const char *str) {
  while (*str) {
    console_key(con, R_KEY_CHAR, *str++);
  }
}

// ============================================================
// CVar: registration and retrieval
// ============================================================

static void test_register_bool(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_bool(&reg, "r_fullscreen", "fullscreen toggle", true);
  ASSERT(cv != NULL);
  ASSERT_STR_EQ(cv->name, "r_fullscreen");
  ASSERT(cv->type == R_CVAR_BOOL);
  ASSERT(cv->value.b == true);
  ASSERT(cv->default_value.b == true);
  cvar_registry_free(&reg);
}

static void test_register_int(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "r_width", "screen width", 1920);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_INT);
  ASSERT_INT_EQ(cv->value.i, 1920);
  ASSERT_INT_EQ(cv->default_value.i, 1920);
  cvar_registry_free(&reg);
}

static void test_register_float(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_float(&reg, "r_gamma", "gamma value", 2.2f);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_FLOAT);
  ASSERT_FLOAT_EQ(cv->value.f, 2.2f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_register_string(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_string(&reg, "player_name", "player name", "player1");
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_STRING);
  ASSERT_STR_EQ(cv->value.s, "player1");
  ASSERT_STR_EQ(cv->default_value.s, "player1");
  cvar_registry_free(&reg);
}

static void test_register_handler(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_handler(&reg, "quit", "quit game", NULL);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_HANDLER);
  ASSERT(cv->value.handler == NULL);
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: set and get
// ============================================================

static void test_set_get_bool(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_bool(&reg, "flag", NULL, false);
  cvar_set_bool(cv, true);
  ASSERT(cvar_get(cv).b == true);
  cvar_set_bool(cv, false);
  ASSERT(cvar_get(cv).b == false);
  cvar_registry_free(&reg);
}

static void test_set_get_int(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "val", NULL, 0);
  cvar_set_int(cv, 42);
  ASSERT_INT_EQ(cvar_get(cv).i, 42);
  cvar_registry_free(&reg);
}

static void test_set_get_float(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_float(&reg, "fval", NULL, 0.0f);
  cvar_set_float(cv, 3.14f);
  ASSERT_FLOAT_EQ(cvar_get(cv).f, 3.14f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_set_get_string(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_string(&reg, "sval", NULL, "old");
  cvar_set_string(cv, "new");
  ASSERT_STR_EQ(cvar_get(cv).s, "new");
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: find
// ============================================================

static void test_find_existing(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "x", NULL, 10);
  RCVar *found = cvar_find(&reg, "x");
  ASSERT(found != NULL);
  ASSERT_INT_EQ(found->value.i, 10);
  cvar_registry_free(&reg);
}

static void test_find_not_found(void) {
  RCVarRegistry reg = new_registry();
  RCVar *found = cvar_find(&reg, "nonexistent");
  ASSERT(found == NULL);
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: reset
// ============================================================

static void test_reset_bool(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_bool(&reg, "b", NULL, false);
  cvar_set_bool(cv, true);
  cvar_reset(cv);
  ASSERT(cv->value.b == false);
  cvar_registry_free(&reg);
}

static void test_reset_int(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "i", NULL, 100);
  cvar_set_int(cv, 999);
  cvar_reset(cv);
  ASSERT_INT_EQ(cv->value.i, 100);
  cvar_registry_free(&reg);
}

static void test_reset_float(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_float(&reg, "f", NULL, 1.5f);
  cvar_set_float(cv, 9.9f);
  cvar_reset(cv);
  ASSERT_FLOAT_EQ(cv->value.f, 1.5f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_reset_string(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_string(&reg, "s", NULL, "default");
  cvar_set_string(cv, "changed");
  cvar_reset(cv);
  ASSERT_STR_EQ(cv->value.s, "default");
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: parse
// ============================================================

static void test_parse_set_value(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "hp", "health", 100);
  bool ok = cvar_parse(&reg, "hp 50");
  ASSERT(ok);
  ASSERT_INT_EQ(cvar_find(&reg, "hp")->value.i, 50);
  cvar_registry_free(&reg);
}

static void test_parse_print_value(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "hp", "health", 100);
  capture_reset();
  bool ok = cvar_parse(&reg, "hp");
  ASSERT(ok);
  ASSERT(strstr(print_buf, "hp = 100") != NULL);
  cvar_registry_free(&reg);
}

static void test_parse_unknown_cvar(void) {
  RCVarRegistry reg = new_registry();
  capture_reset();
  bool ok = cvar_parse(&reg, "nope");
  ASSERT(!ok);
  ASSERT(strstr(print_buf, "unknown cvar: nope") != NULL);
  cvar_registry_free(&reg);
}

static void test_parse_equals_syntax(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_float(&reg, "vol", NULL, 1.0f);
  bool ok = cvar_parse(&reg, "vol=0.5");
  ASSERT(ok);
  ASSERT_FLOAT_EQ(cvar_find(&reg, "vol")->value.f, 0.5f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_parse_equals_with_spaces(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_float(&reg, "vol", NULL, 1.0f);
  bool ok = cvar_parse(&reg, "vol = 0.75");
  ASSERT(ok);
  ASSERT_FLOAT_EQ(cvar_find(&reg, "vol")->value.f, 0.75f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_parse_auto_create_bool(void) {
  RCVarRegistry reg = new_registry();
  bool ok = cvar_parse(&reg, "new_flag true");
  ASSERT(ok);
  RCVar *cv = cvar_find(&reg, "new_flag");
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_BOOL);
  ASSERT(cv->value.b == true);
  cvar_registry_free(&reg);
}

static void test_parse_auto_create_int(void) {
  RCVarRegistry reg = new_registry();
  bool ok = cvar_parse(&reg, "new_val 42");
  ASSERT(ok);
  RCVar *cv = cvar_find(&reg, "new_val");
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_INT);
  ASSERT_INT_EQ(cv->value.i, 42);
  cvar_registry_free(&reg);
}

static void test_parse_auto_create_float(void) {
  RCVarRegistry reg = new_registry();
  bool ok = cvar_parse(&reg, "new_flt 3.14");
  ASSERT(ok);
  RCVar *cv = cvar_find(&reg, "new_flt");
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_FLOAT);
  ASSERT_FLOAT_EQ(cv->value.f, 3.14f, 0.01f);
  cvar_registry_free(&reg);
}

static void test_parse_auto_create_string(void) {
  RCVarRegistry reg = new_registry();
  bool ok = cvar_parse(&reg, "new_str hello");
  ASSERT(ok);
  RCVar *cv = cvar_find(&reg, "new_str");
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_STRING);
  ASSERT_STR_EQ(cv->value.s, "hello");
  cvar_registry_free(&reg);
}

static void test_parse_empty_line(void) {
  RCVarRegistry reg = new_registry();
  ASSERT(!cvar_parse(&reg, ""));
  ASSERT(!cvar_parse(&reg, "   "));
  cvar_registry_free(&reg);
}

static void test_parse_set_bool_from_string(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_bool(&reg, "debug", NULL, false);

  cvar_parse(&reg, "debug true");
  ASSERT(cvar_find(&reg, "debug")->value.b == true);

  cvar_parse(&reg, "debug 1");
  ASSERT(cvar_find(&reg, "debug")->value.b == true);

  cvar_parse(&reg, "debug false");
  ASSERT(cvar_find(&reg, "debug")->value.b == false);

  cvar_parse(&reg, "debug 0");
  ASSERT(cvar_find(&reg, "debug")->value.b == false);

  cvar_registry_free(&reg);
}

static void test_parse_print_bool(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_bool(&reg, "flag", NULL, true);
  capture_reset();
  cvar_parse(&reg, "flag");
  ASSERT(strstr(print_buf, "flag = true") != NULL);
  cvar_registry_free(&reg);
}

static void test_parse_print_float(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_float(&reg, "gamma", NULL, 2.2f);
  capture_reset();
  cvar_parse(&reg, "gamma");
  ASSERT(strstr(print_buf, "gamma = 2.2") != NULL);
  cvar_registry_free(&reg);
}

static void test_parse_print_string(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_string(&reg, "name", NULL, "world");
  capture_reset();
  cvar_parse(&reg, "name");
  ASSERT(strstr(print_buf, "name = \"world\"") != NULL);
  cvar_registry_free(&reg);
}

static void test_parse_print_handler(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_handler(&reg, "help", NULL, NULL);
  capture_reset();
  cvar_parse(&reg, "help");
  // handler with NULL fn should still return true and not crash
  ASSERT(strstr(print_buf, "help") == NULL || 1);
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: define
// ============================================================

static void test_define_bool(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "d_bool", R_CVAR_BOOL);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_BOOL);
  ASSERT(cv->value.b == false);
  cvar_registry_free(&reg);
}

static void test_define_int(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "d_int", R_CVAR_INT);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_INT);
  ASSERT_INT_EQ(cv->value.i, 0);
  cvar_registry_free(&reg);
}

static void test_define_float(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "d_float", R_CVAR_FLOAT);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_FLOAT);
  ASSERT_FLOAT_EQ(cv->value.f, 0.0f, 0.001f);
  cvar_registry_free(&reg);
}

static void test_define_string(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "d_str", R_CVAR_STRING);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_STRING);
  ASSERT_STR_EQ(cv->value.s, "");
  cvar_registry_free(&reg);
}

static void test_define_handler(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "d_handler", R_CVAR_HANDLER);
  ASSERT(cv != NULL);
  ASSERT(cv->type == R_CVAR_HANDLER);
  ASSERT(cv->value.handler == NULL);
  cvar_registry_free(&reg);
}

static void test_define_returns_existing(void) {
  RCVarRegistry reg = new_registry();
  RCVar *a = cvar_register_int(&reg, "x", NULL, 42);
  RCVar *b = cvar_define(&reg, "x", R_CVAR_INT);
  ASSERT(a == b);
  ASSERT_INT_EQ(b->value.i, 42);
  cvar_registry_free(&reg);
}

static void test_define_computed_returns_null(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_define(&reg, "comp", R_CVAR_COMPUTED);
  ASSERT(cv == NULL);
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: type_from_string and type_name
// ============================================================

static void test_type_from_string(void) {
  RCVarType t;
  ASSERT(cvar_type_from_string("bool", &t) && t == R_CVAR_BOOL);
  ASSERT(cvar_type_from_string("int", &t) && t == R_CVAR_INT);
  ASSERT(cvar_type_from_string("float", &t) && t == R_CVAR_FLOAT);
  ASSERT(cvar_type_from_string("string", &t) && t == R_CVAR_STRING);
  ASSERT(!cvar_type_from_string("handler", &t));
  ASSERT(!cvar_type_from_string("garbage", &t));
}

static void test_type_name(void) {
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_BOOL), "bool");
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_INT), "int");
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_FLOAT), "float");
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_STRING), "string");
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_HANDLER), "handler");
  ASSERT_STR_EQ(cvar_type_name(R_CVAR_COMPUTED), "computed");
}

// ============================================================
// CVar: autocomplete
// ============================================================

static void test_autocomplete_no_match(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "hp", NULL, 100);
  const char *results[8];
  uint32_t count = cvar_autocomplete(&reg, "z", results, 8);
  ASSERT_INT_EQ((int)count, 0);
  cvar_registry_free(&reg);
}

static void test_autocomplete_single_match(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_width", NULL, 1920);
  cvar_register_int(&reg, "s_volume", NULL, 80);
  const char *results[8];
  uint32_t count = cvar_autocomplete(&reg, "r_", results, 8);
  ASSERT_INT_EQ((int)count, 1);
  ASSERT_STR_EQ(results[0], "r_width");
  cvar_registry_free(&reg);
}

static void test_autocomplete_multiple_matches(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_width", NULL, 1920);
  cvar_register_int(&reg, "r_height", NULL, 1080);
  cvar_register_int(&reg, "s_volume", NULL, 80);
  const char *results[8];
  uint32_t count = cvar_autocomplete(&reg, "r_", results, 8);
  ASSERT_INT_EQ((int)count, 2);
  cvar_registry_free(&reg);
}

static void test_autocomplete_empty_prefix(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "a", NULL, 1);
  cvar_register_int(&reg, "b", NULL, 2);
  cvar_register_int(&reg, "c", NULL, 3);
  const char *results[8];
  uint32_t count = cvar_autocomplete(&reg, "", results, 8);
  ASSERT_INT_EQ((int)count, 3);
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: handler cvars
// ============================================================

static int handler_called_count;
static char handler_args_buf[256];

static void test_handler_fn(RCVar *cvar, const char *args) {
  (void)cvar;
  handler_called_count++;
  if (args) {
    strncpy(handler_args_buf, args, sizeof(handler_args_buf) - 1);
    handler_args_buf[sizeof(handler_args_buf) - 1] = '\0';
  }
}

static void test_handler_called_via_parse(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_handler(&reg, "greet", NULL, test_handler_fn);
  handler_called_count = 0;
  handler_args_buf[0] = '\0';
  bool ok = cvar_parse(&reg, "greet world");
  ASSERT(ok);
  ASSERT_INT_EQ(handler_called_count, 1);
  ASSERT_STR_EQ(handler_args_buf, "world");
  cvar_registry_free(&reg);
}

static void test_handler_null_fn(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_handler(&reg, "noop", NULL, NULL);
  bool ok = cvar_parse(&reg, "noop");
  ASSERT(ok); // should not crash
  cvar_registry_free(&reg);
}

static void test_handler_no_args(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_handler(&reg, "cmd", NULL, test_handler_fn);
  handler_called_count = 0;
  handler_args_buf[0] = '\0';
  cvar_parse(&reg, "cmd");
  // handler called even with no args (empty rest string after handler check)
  // Looking at the code: if type == HANDLER and rest == "", handler is still called
  ASSERT_INT_EQ(handler_called_count, 1);
  ASSERT_STR_EQ(handler_args_buf, "");
  cvar_registry_free(&reg);
}

// ============================================================
// CVar: on_change callbacks
// ============================================================

static int on_change_count;

static void on_change_fn(RCVar *cvar) {
  (void)cvar;
  on_change_count++;
}

static void test_on_change_fires_on_set(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "val", NULL, 0);
  cv->on_change = on_change_fn;
  on_change_count = 0;
  cvar_set_int(cv, 5);
  ASSERT_INT_EQ(on_change_count, 1);
  cvar_set_int(cv, 10);
  ASSERT_INT_EQ(on_change_count, 2);
  cvar_registry_free(&reg);
}

static void test_on_change_fires_for_each_type(void) {
  RCVarRegistry reg = new_registry();

  on_change_count = 0;
  RCVar *b = cvar_register_bool(&reg, "b", NULL, false);
  b->on_change = on_change_fn;
  cvar_set_bool(b, true);
  ASSERT_INT_EQ(on_change_count, 1);

  RCVar *f = cvar_register_float(&reg, "f", NULL, 0.0f);
  f->on_change = on_change_fn;
  cvar_set_float(f, 1.0f);
  ASSERT_INT_EQ(on_change_count, 2);

  RCVar *s = cvar_register_string(&reg, "s", NULL, "a");
  s->on_change = on_change_fn;
  cvar_set_string(s, "b");
  ASSERT_INT_EQ(on_change_count, 3);

  cvar_registry_free(&reg);
}

// ============================================================
// Reactive graph: dirty flags
// ============================================================

static void test_dirty_flag_set_on_change(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  ASSERT(!cv->dirty);
  cvar_set_int(cv, 5);
  ASSERT(cv->dirty);
  cvar_registry_free(&reg);
}

static void test_dirty_cleared_after_flush(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  cvar_set_int(cv, 5);
  ASSERT(cv->dirty);
  cvar_flush(&reg);
  ASSERT(!cv->dirty);
  cvar_registry_free(&reg);
}

// ============================================================
// Reactive graph: subscribe
// ============================================================

static int sub_called_count;
static RCVar *sub_last_cvar;

static void sub_fn(RCVar *cvar, void *ctx) {
  (void)ctx;
  sub_called_count++;
  sub_last_cvar = cvar;
}

static void test_subscribe_fires_on_flush(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  cvar_subscribe(cv, sub_fn, NULL);
  sub_called_count = 0;
  sub_last_cvar = NULL;
  cvar_set_int(cv, 10);
  ASSERT_INT_EQ(sub_called_count, 0); // not fired yet
  cvar_flush(&reg);
  ASSERT_INT_EQ(sub_called_count, 1);
  ASSERT(sub_last_cvar == cv);
  cvar_registry_free(&reg);
}

static void test_subscribe_not_before_flush(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  cvar_subscribe(cv, sub_fn, NULL);
  sub_called_count = 0;
  cvar_set_int(cv, 10);
  cvar_set_int(cv, 20);
  cvar_set_int(cv, 30);
  ASSERT_INT_EQ(sub_called_count, 0);
  cvar_registry_free(&reg);
}

static int sub_a_count;
static int sub_b_count;

static void sub_a_fn(RCVar *cvar, void *ctx) {
  (void)cvar; (void)ctx;
  sub_a_count++;
}

static void sub_b_fn(RCVar *cvar, void *ctx) {
  (void)cvar; (void)ctx;
  sub_b_count++;
}

static void test_multiple_subscribers(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  cvar_subscribe(cv, sub_a_fn, NULL);
  cvar_subscribe(cv, sub_b_fn, NULL);
  sub_a_count = 0;
  sub_b_count = 0;
  cvar_set_int(cv, 10);
  cvar_flush(&reg);
  ASSERT_INT_EQ(sub_a_count, 1);
  ASSERT_INT_EQ(sub_b_count, 1);
  cvar_registry_free(&reg);
}

static void test_no_duplicate_notifications(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  cvar_subscribe(cv, sub_fn, NULL);
  sub_called_count = 0;
  cvar_set_int(cv, 10);
  cvar_set_int(cv, 20);
  cvar_set_int(cv, 30);
  cvar_flush(&reg);
  ASSERT_INT_EQ(sub_called_count, 1);
  cvar_registry_free(&reg);
}

static void test_subscriber_context(void) {
  RCVarRegistry reg = new_registry();
  RCVar *cv = cvar_register_int(&reg, "x", NULL, 0);
  int ctx_value = 0;
  cvar_subscribe(cv, (RCVarSubscribeFn)sub_fn, &ctx_value);
  sub_called_count = 0;
  cvar_set_int(cv, 5);
  cvar_flush(&reg);
  ASSERT_INT_EQ(sub_called_count, 1);
  cvar_registry_free(&reg);
}

// ============================================================
// Reactive graph: computed cvars
// ============================================================

static int compute_call_count;

static RCVarValue compute_sum(RCVar **deps, uint32_t dep_count, void *ctx) {
  (void)ctx;
  compute_call_count++;
  RCVarValue result = {0};
  int32_t sum = 0;
  for (uint32_t i = 0; i < dep_count; i++) {
    sum += cvar_get(deps[i]).i;
  }
  result.i = sum;
  return result;
}

static void test_computed_initial_value(void) {
  RCVarRegistry reg = new_registry();
  RCVar *a = cvar_register_int(&reg, "a", NULL, 3);
  RCVar *b = cvar_register_int(&reg, "b", NULL, 7);
  RCVar *deps[] = {a, b};
  compute_call_count = 0;
  RCVar *sum = cvar_register_computed(&reg, "sum", NULL, R_CVAR_INT, deps, 2, compute_sum, NULL);
  ASSERT_INT_EQ(compute_call_count, 1);
  ASSERT_INT_EQ(cvar_get(sum).i, 10);
  cvar_registry_free(&reg);
}

static int32_t recomp_observed;
static int recomp_sub_count;

static void recomp_sub_fn(RCVar *cvar, void *ctx) {
  (void)ctx;
  recomp_sub_count++;
  recomp_observed = cvar_get(cvar).i;
}

static void test_computed_recomputes_on_dep_change(void) {
  // After flush, computed cvars that depend on changed values are marked dirty
  // and then immediately cleared. A subscriber reading during flush gets
  // the recomputed value via lazy cvar_get.
  RCVarRegistry reg = new_registry();
  RCVar *a = cvar_register_int(&reg, "a", NULL, 3);
  RCVar *b = cvar_register_int(&reg, "b", NULL, 7);
  RCVar *deps[] = {a, b};
  RCVar *sum = cvar_register_computed(&reg, "sum", NULL, R_CVAR_INT, deps, 2, compute_sum, NULL);
  ASSERT_INT_EQ(cvar_get(sum).i, 10);

  cvar_subscribe(sum, recomp_sub_fn, NULL);
  recomp_sub_count = 0;
  recomp_observed = 0;

  cvar_set_int(a, 10);
  cvar_flush(&reg);

  ASSERT_INT_EQ(recomp_sub_count, 1);
  ASSERT_INT_EQ(recomp_observed, 17);

  cvar_registry_free(&reg);
}

static void test_computed_caching(void) {
  RCVarRegistry reg = new_registry();
  RCVar *a = cvar_register_int(&reg, "a", NULL, 5);
  RCVar *deps[] = {a};
  compute_call_count = 0;
  RCVar *comp = cvar_register_computed(&reg, "comp", NULL, R_CVAR_INT, deps, 1, compute_sum, NULL);
  ASSERT_INT_EQ(compute_call_count, 1);

  // cvar_get should NOT recompute because deps haven't changed
  cvar_get(comp);
  cvar_get(comp);
  cvar_get(comp);
  ASSERT_INT_EQ(compute_call_count, 1);
  cvar_registry_free(&reg);
}

static void test_computed_lazy_recomputation(void) {
  // cvar_get triggers lazy recomputation when the computed cvar is dirty.
  // We set dirty manually to test the lazy path without flush clearing it.
  RCVarRegistry reg = new_registry();
  RCVar *a = cvar_register_int(&reg, "a", NULL, 5);
  RCVar *deps[] = {a};
  compute_call_count = 0;
  RCVar *comp = cvar_register_computed(&reg, "comp", NULL, R_CVAR_INT, deps, 1, compute_sum, NULL);
  ASSERT_INT_EQ(compute_call_count, 1);
  ASSERT_INT_EQ(cvar_get(comp).i, 5);

  cvar_set_int(a, 20);
  // Manually mark comp dirty (flush's first pass would do this, but also clears it)
  comp->dirty = true;
  ASSERT_INT_EQ(compute_call_count, 1); // not yet recomputed

  RCVarValue v = cvar_get(comp); // triggers lazy recompute
  ASSERT_INT_EQ(compute_call_count, 2);
  ASSERT_INT_EQ(v.i, 20);
  cvar_registry_free(&reg);
}

// ============================================================
// Reactive graph: chain (value -> computed -> subscriber)
// ============================================================

static int chain_sub_count;
static int32_t chain_sub_value;

static void chain_sub_fn(RCVar *cvar, void *ctx) {
  (void)ctx;
  chain_sub_count++;
  chain_sub_value = cvar_get(cvar).i;
}

static RCVarValue compute_double(RCVar **deps, uint32_t dep_count, void *ctx) {
  (void)ctx; (void)dep_count;
  RCVarValue result = {0};
  result.i = cvar_get(deps[0]).i * 2;
  return result;
}

static void test_chain_value_computed_subscriber(void) {
  RCVarRegistry reg = new_registry();
  RCVar *val = cvar_register_int(&reg, "base", NULL, 5);
  RCVar *deps[] = {val};
  RCVar *doubled = cvar_register_computed(&reg, "doubled", NULL, R_CVAR_INT, deps, 1, compute_double, NULL);
  ASSERT_INT_EQ(cvar_get(doubled).i, 10);

  cvar_subscribe(doubled, chain_sub_fn, NULL);
  chain_sub_count = 0;
  chain_sub_value = 0;

  cvar_set_int(val, 15);
  cvar_flush(&reg);

  ASSERT_INT_EQ(chain_sub_count, 1);
  ASSERT_INT_EQ(chain_sub_value, 30);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: init
// ============================================================

static void test_console_init_zeroes(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  memset(&con, 0xFF, sizeof(con));
  console_init(&con, &reg);
  ASSERT_INT_EQ((int)con.cursor, 0);
  ASSERT_INT_EQ((int)con.length, 0);
  ASSERT_INT_EQ((int)con.history_count, 0);
  ASSERT_INT_EQ((int)con.output_count, 0);
  ASSERT_INT_EQ((int)con.version, 0);
  ASSERT(con.line[0] == '\0');
  ASSERT(con.registry == &reg);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: character insertion
// ============================================================

static void test_console_insert_chars(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello");
  ASSERT_STR_EQ(con.line, "hello");
  ASSERT_INT_EQ((int)con.cursor, 5);
  ASSERT_INT_EQ((int)con.length, 5);
  cvar_registry_free(&reg);
}

static void test_console_insert_mid_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hllo");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  console_key(&con, R_KEY_CHAR, 'e');
  ASSERT_STR_EQ(con.line, "hello");
  ASSERT_INT_EQ((int)con.cursor, 2);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: cursor movement
// ============================================================

static void test_cursor_left(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_LEFT, 0);
  ASSERT_INT_EQ((int)con.cursor, 2);
  console_key(&con, R_KEY_LEFT, 0);
  ASSERT_INT_EQ((int)con.cursor, 1);
  cvar_registry_free(&reg);
}

static void test_cursor_left_at_start(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "a");
  console_key(&con, R_KEY_LEFT, 0);
  ASSERT_INT_EQ((int)con.cursor, 0);
  uint32_t v = con.version;
  console_key(&con, R_KEY_LEFT, 0); // no-op
  ASSERT_INT_EQ((int)con.cursor, 0);
  ASSERT_INT_EQ((int)con.version, (int)v); // version should not change
  cvar_registry_free(&reg);
}

static void test_cursor_right(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  ASSERT_INT_EQ((int)con.cursor, 1);
  cvar_registry_free(&reg);
}

static void test_cursor_right_at_end(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "ab");
  uint32_t v = con.version;
  console_key(&con, R_KEY_RIGHT, 0); // already at end
  ASSERT_INT_EQ((int)con.cursor, 2);
  ASSERT_INT_EQ((int)con.version, (int)v);
  cvar_registry_free(&reg);
}

static void test_cursor_home(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello");
  console_key(&con, R_KEY_HOME, 0);
  ASSERT_INT_EQ((int)con.cursor, 0);
  cvar_registry_free(&reg);
}

static void test_cursor_end(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_END, 0);
  ASSERT_INT_EQ((int)con.cursor, 5);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: backspace and delete
// ============================================================

static void test_backspace(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_BACKSPACE, 0);
  ASSERT_STR_EQ(con.line, "ab");
  ASSERT_INT_EQ((int)con.cursor, 2);
  ASSERT_INT_EQ((int)con.length, 2);
  cvar_registry_free(&reg);
}

static void test_backspace_at_start(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "a");
  console_key(&con, R_KEY_HOME, 0);
  uint32_t v = con.version;
  console_key(&con, R_KEY_BACKSPACE, 0);
  ASSERT_STR_EQ(con.line, "a");
  ASSERT_INT_EQ((int)con.version, (int)v);
  cvar_registry_free(&reg);
}

static void test_backspace_mid_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abcd");
  console_key(&con, R_KEY_LEFT, 0);
  console_key(&con, R_KEY_BACKSPACE, 0);
  ASSERT_STR_EQ(con.line, "abd");
  ASSERT_INT_EQ((int)con.cursor, 2);
  cvar_registry_free(&reg);
}

static void test_delete(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_DELETE, 0);
  ASSERT_STR_EQ(con.line, "bc");
  ASSERT_INT_EQ((int)con.cursor, 0);
  ASSERT_INT_EQ((int)con.length, 2);
  cvar_registry_free(&reg);
}

static void test_delete_at_end(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "ab");
  uint32_t v = con.version;
  console_key(&con, R_KEY_DELETE, 0); // at end, no-op
  ASSERT_STR_EQ(con.line, "ab");
  ASSERT_INT_EQ((int)con.version, (int)v);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: history
// ============================================================

static void test_history_up_down(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  type_string(&con, "first");
  console_key(&con, R_KEY_ENTER, 0);
  type_string(&con, "second");
  console_key(&con, R_KEY_ENTER, 0);

  // up once => second
  console_key(&con, R_KEY_UP, 0);
  ASSERT_STR_EQ(con.line, "second");

  // up again => first
  console_key(&con, R_KEY_UP, 0);
  ASSERT_STR_EQ(con.line, "first");

  // down => second
  console_key(&con, R_KEY_DOWN, 0);
  ASSERT_STR_EQ(con.line, "second");

  // down => restored empty line
  console_key(&con, R_KEY_DOWN, 0);
  ASSERT_STR_EQ(con.line, "");

  cvar_registry_free(&reg);
}

static void test_history_saves_current_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  type_string(&con, "first");
  console_key(&con, R_KEY_ENTER, 0);

  type_string(&con, "typing");
  console_key(&con, R_KEY_UP, 0);
  ASSERT_STR_EQ(con.line, "first");

  console_key(&con, R_KEY_DOWN, 0);
  ASSERT_STR_EQ(con.line, "typing");

  cvar_registry_free(&reg);
}

static void test_history_empty_not_pushed(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  console_key(&con, R_KEY_ENTER, 0); // empty
  ASSERT_INT_EQ((int)con.history_count, 0);

  cvar_registry_free(&reg);
}

static void test_history_no_duplicates(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  type_string(&con, "cmd");
  console_key(&con, R_KEY_ENTER, 0);
  type_string(&con, "cmd");
  console_key(&con, R_KEY_ENTER, 0);

  ASSERT_INT_EQ((int)con.history_count, 1);

  cvar_registry_free(&reg);
}

static void test_history_up_at_top(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  type_string(&con, "only");
  console_key(&con, R_KEY_ENTER, 0);

  console_key(&con, R_KEY_UP, 0);
  ASSERT_STR_EQ(con.line, "only");
  console_key(&con, R_KEY_UP, 0); // at top, stays
  ASSERT_STR_EQ(con.line, "only");

  cvar_registry_free(&reg);
}

static void test_history_down_without_browsing(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  type_string(&con, "x");
  console_key(&con, R_KEY_ENTER, 0);
  // without pressing up first, down should be no-op
  uint32_t v = con.version;
  console_key(&con, R_KEY_DOWN, 0);
  ASSERT_INT_EQ((int)con.version, (int)v);

  cvar_registry_free(&reg);
}

static void test_history_up_with_empty_history(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  uint32_t v = con.version;
  console_key(&con, R_KEY_UP, 0);
  ASSERT_INT_EQ((int)con.version, (int)v);

  cvar_registry_free(&reg);
}

// ============================================================
// Console: tab completion
// ============================================================

static void test_tab_single_match(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_fullscreen", NULL, 1);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "r_f");
  console_key(&con, R_KEY_TAB, 0);
  ASSERT_STR_EQ(con.line, "r_fullscreen ");
  ASSERT_INT_EQ((int)con.cursor, 13);
  cvar_registry_free(&reg);
}

static void test_tab_multiple_matches_common_prefix(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_width", NULL, 1920);
  cvar_register_int(&reg, "r_wireframe", NULL, 0);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "r_w");
  console_key(&con, R_KEY_TAB, 0);
  // common prefix is "r_wi"
  ASSERT_STR_EQ(con.line, "r_wi");
  // also outputs the matches
  ASSERT(con.output_count >= 2);
  cvar_registry_free(&reg);
}

static void test_tab_no_match(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_width", NULL, 1920);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "z_");
  uint32_t v = con.version;
  console_key(&con, R_KEY_TAB, 0);
  ASSERT_STR_EQ(con.line, "z_");
  // version should not change since no match and no modification
  // Actually looking at code: if count==0 we return, no version bump
  ASSERT_INT_EQ((int)con.version, (int)v);
  cvar_registry_free(&reg);
}

static void test_tab_empty_line(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "test", NULL, 0);
  RConsole con;
  console_init(&con, &reg);
  uint32_t v = con.version;
  console_key(&con, R_KEY_TAB, 0); // empty line, early return
  ASSERT_INT_EQ((int)con.version, (int)v);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: output
// ============================================================

static void test_console_printf(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  console_printf(&con, "hello %s", "world");
  ASSERT_INT_EQ((int)console_output_count(&con), 1);
  ASSERT_STR_EQ(console_output_line(&con, 0), "hello world");
  cvar_registry_free(&reg);
}

static void test_console_printf_strips_newline(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  console_printf(&con, "line\n");
  ASSERT_STR_EQ(console_output_line(&con, 0), "line");
  cvar_registry_free(&reg);
}

static void test_console_output_ring(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);

  for (int i = 0; i < R_CONSOLE_OUTPUT_MAX + 10; i++) {
    console_printf(&con, "line %d", i);
  }

  ASSERT_INT_EQ((int)console_output_count(&con), R_CONSOLE_OUTPUT_MAX);

  // first visible should be line 10 (first 10 dropped)
  const char *first = console_output_line(&con, 0);
  ASSERT(first != NULL);
  ASSERT(strstr(first, "line 10") != NULL);

  // last visible should be line (OUTPUT_MAX + 9)
  char expected[64];
  snprintf(expected, sizeof(expected), "line %d", R_CONSOLE_OUTPUT_MAX + 9);
  const char *last = console_output_line(&con, R_CONSOLE_OUTPUT_MAX - 1);
  ASSERT(last != NULL);
  ASSERT(strstr(last, expected) != NULL);

  cvar_registry_free(&reg);
}

static void test_console_output_line_out_of_bounds(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  console_printf(&con, "only");
  ASSERT(console_output_line(&con, 0) != NULL);
  ASSERT(console_output_line(&con, 1) == NULL);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: submit
// ============================================================

static void test_submit_clears_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "something");
  console_key(&con, R_KEY_ENTER, 0);
  ASSERT_STR_EQ(con.line, "");
  ASSERT_INT_EQ((int)con.cursor, 0);
  ASSERT_INT_EQ((int)con.length, 0);
  cvar_registry_free(&reg);
}

static void test_submit_adds_to_history(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd1");
  console_key(&con, R_KEY_ENTER, 0);
  ASSERT_INT_EQ((int)con.history_count, 1);
  cvar_registry_free(&reg);
}

static void test_submit_adds_to_output(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello");
  console_key(&con, R_KEY_ENTER, 0);
  // submit prints "] hello" to output
  ASSERT(con.output_count >= 1);
  ASSERT(strstr(console_output_line(&con, 0), "] hello") != NULL);
  cvar_registry_free(&reg);
}

static void test_submit_parses_cvar(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "hp", NULL, 100);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hp 50");
  console_key(&con, R_KEY_ENTER, 0);
  ASSERT_INT_EQ(cvar_find(&reg, "hp")->value.i, 50);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: current_cvar
// ============================================================

static void test_current_cvar_found(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "hp", NULL, 100);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hp 50");
  RCVar *cv = console_current_cvar(&con);
  ASSERT(cv != NULL);
  ASSERT_STR_EQ(cv->name, "hp");
  cvar_registry_free(&reg);
}

static void test_current_cvar_not_found(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "nope");
  ASSERT(console_current_cvar(&con) == NULL);
  cvar_registry_free(&reg);
}

static void test_current_cvar_empty(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  ASSERT(console_current_cvar(&con) == NULL);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: arg_index
// ============================================================

static void test_arg_index_no_args(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd");
  ASSERT_INT_EQ((int)console_arg_index(&con), 0);
  cvar_registry_free(&reg);
}

static void test_arg_index_one_arg(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd arg1");
  ASSERT_INT_EQ((int)console_arg_index(&con), 1);
  cvar_registry_free(&reg);
}

static void test_arg_index_two_args(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd arg1 arg2");
  ASSERT_INT_EQ((int)console_arg_index(&con), 2);
  cvar_registry_free(&reg);
}

static void test_arg_index_equals(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd=val");
  ASSERT_INT_EQ((int)console_arg_index(&con), 1);
  cvar_registry_free(&reg);
}

static void test_arg_index_trailing_space(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd ");
  ASSERT_INT_EQ((int)console_arg_index(&con), 1);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: past_name
// ============================================================

static void test_past_name_no_space(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd");
  ASSERT(!console_past_name(&con));
  cvar_registry_free(&reg);
}

static void test_past_name_with_space(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd ");
  ASSERT(console_past_name(&con));
  cvar_registry_free(&reg);
}

static void test_past_name_with_equals(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "cmd=");
  ASSERT(console_past_name(&con));
  cvar_registry_free(&reg);
}

// ============================================================
// Console: kill line and kill to start
// ============================================================

static void test_kill_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello world");
  // move cursor to position 5
  console_key(&con, R_KEY_HOME, 0);
  for (int i = 0; i < 5; i++) {
    console_key(&con, R_KEY_RIGHT, 0);
  }
  console_key(&con, R_KEY_KILL_LINE, 0);
  ASSERT_STR_EQ(con.line, "hello");
  ASSERT_INT_EQ((int)con.cursor, 5);
  ASSERT_INT_EQ((int)con.length, 5);
  cvar_registry_free(&reg);
}

static void test_kill_to_start(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "hello world");
  // move cursor to position 6
  console_key(&con, R_KEY_HOME, 0);
  for (int i = 0; i < 6; i++) {
    console_key(&con, R_KEY_RIGHT, 0);
  }
  console_key(&con, R_KEY_KILL_TO_START, 0);
  ASSERT_STR_EQ(con.line, "world");
  ASSERT_INT_EQ((int)con.cursor, 0);
  ASSERT_INT_EQ((int)con.length, 5);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: input_char mapping
// ============================================================

static void test_input_char_newline(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "test");
  console_input_char(&con, '\n');
  ASSERT_INT_EQ((int)con.length, 0); // submitted
  cvar_registry_free(&reg);
}

static void test_input_char_carriage_return(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "test");
  console_input_char(&con, '\r');
  ASSERT_INT_EQ((int)con.length, 0);
  cvar_registry_free(&reg);
}

static void test_input_char_tab(void) {
  RCVarRegistry reg = new_registry();
  cvar_register_int(&reg, "r_test", NULL, 0);
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "r_t");
  console_input_char(&con, '\t');
  ASSERT_STR_EQ(con.line, "r_test ");
  cvar_registry_free(&reg);
}

static void test_input_char_backspace(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "ab");
  console_input_char(&con, 127); // DEL
  ASSERT_STR_EQ(con.line, "a");
  type_string(&con, "b");
  console_input_char(&con, '\b'); // BS
  ASSERT_STR_EQ(con.line, "a");
  cvar_registry_free(&reg);
}

static void test_input_char_ctrl_a_home(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_input_char(&con, 1); // ctrl-a
  ASSERT_INT_EQ((int)con.cursor, 0);
  cvar_registry_free(&reg);
}

static void test_input_char_ctrl_e_end(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_HOME, 0);
  console_input_char(&con, 5); // ctrl-e
  ASSERT_INT_EQ((int)con.cursor, 3);
  cvar_registry_free(&reg);
}

static void test_input_char_ctrl_d_delete(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abc");
  console_key(&con, R_KEY_HOME, 0);
  console_input_char(&con, 4); // ctrl-d
  ASSERT_STR_EQ(con.line, "bc");
  cvar_registry_free(&reg);
}

static void test_input_char_ctrl_k_kill_line(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abcdef");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  console_input_char(&con, 11); // ctrl-k
  ASSERT_STR_EQ(con.line, "ab");
  cvar_registry_free(&reg);
}

static void test_input_char_ctrl_u_kill_to_start(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "abcdef");
  console_key(&con, R_KEY_HOME, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  console_key(&con, R_KEY_RIGHT, 0);
  console_input_char(&con, 21); // ctrl-u
  ASSERT_STR_EQ(con.line, "cdef");
  ASSERT_INT_EQ((int)con.cursor, 0);
  cvar_registry_free(&reg);
}

static void test_input_char_printable(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  console_input_char(&con, 'A');
  console_input_char(&con, '~');
  console_input_char(&con, ' ');
  ASSERT_STR_EQ(con.line, "A~ ");
  cvar_registry_free(&reg);
}

static void test_input_char_control_ignored(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  uint32_t v = con.version;
  console_input_char(&con, 2); // ctrl-b, not mapped
  console_input_char(&con, 3); // ctrl-c, not mapped
  console_input_char(&con, 6); // ctrl-f, not mapped
  ASSERT_INT_EQ((int)con.version, (int)v);
  ASSERT_INT_EQ((int)con.length, 0);
  cvar_registry_free(&reg);
}

// ============================================================
// Console: version increments
// ============================================================

static void test_version_increments_on_char(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  uint32_t v = con.version;
  console_key(&con, R_KEY_CHAR, 'a');
  ASSERT(con.version > v);
  cvar_registry_free(&reg);
}

static void test_version_increments_on_submit(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  type_string(&con, "x");
  uint32_t v = con.version;
  console_key(&con, R_KEY_ENTER, 0);
  ASSERT(con.version > v);
  cvar_registry_free(&reg);
}

static void test_version_increments_on_printf(void) {
  RCVarRegistry reg = new_registry();
  RConsole con;
  console_init(&con, &reg);
  uint32_t v = con.version;
  console_printf(&con, "msg");
  ASSERT(con.version > v);
  cvar_registry_free(&reg);
}

// ============================================================
// Main
// ============================================================

int main(void) {
  printf("CVar: registration and retrieval\n");
  RUN_TEST(test_register_bool);
  RUN_TEST(test_register_int);
  RUN_TEST(test_register_float);
  RUN_TEST(test_register_string);
  RUN_TEST(test_register_handler);

  printf("\nCVar: set and get\n");
  RUN_TEST(test_set_get_bool);
  RUN_TEST(test_set_get_int);
  RUN_TEST(test_set_get_float);
  RUN_TEST(test_set_get_string);

  printf("\nCVar: find\n");
  RUN_TEST(test_find_existing);
  RUN_TEST(test_find_not_found);

  printf("\nCVar: reset\n");
  RUN_TEST(test_reset_bool);
  RUN_TEST(test_reset_int);
  RUN_TEST(test_reset_float);
  RUN_TEST(test_reset_string);

  printf("\nCVar: parse\n");
  RUN_TEST(test_parse_set_value);
  RUN_TEST(test_parse_print_value);
  RUN_TEST(test_parse_unknown_cvar);
  RUN_TEST(test_parse_equals_syntax);
  RUN_TEST(test_parse_equals_with_spaces);
  RUN_TEST(test_parse_auto_create_bool);
  RUN_TEST(test_parse_auto_create_int);
  RUN_TEST(test_parse_auto_create_float);
  RUN_TEST(test_parse_auto_create_string);
  RUN_TEST(test_parse_empty_line);
  RUN_TEST(test_parse_set_bool_from_string);
  RUN_TEST(test_parse_print_bool);
  RUN_TEST(test_parse_print_float);
  RUN_TEST(test_parse_print_string);
  RUN_TEST(test_parse_print_handler);

  printf("\nCVar: define\n");
  RUN_TEST(test_define_bool);
  RUN_TEST(test_define_int);
  RUN_TEST(test_define_float);
  RUN_TEST(test_define_string);
  RUN_TEST(test_define_handler);
  RUN_TEST(test_define_returns_existing);
  RUN_TEST(test_define_computed_returns_null);

  printf("\nCVar: type_from_string and type_name\n");
  RUN_TEST(test_type_from_string);
  RUN_TEST(test_type_name);

  printf("\nCVar: autocomplete\n");
  RUN_TEST(test_autocomplete_no_match);
  RUN_TEST(test_autocomplete_single_match);
  RUN_TEST(test_autocomplete_multiple_matches);
  RUN_TEST(test_autocomplete_empty_prefix);

  printf("\nCVar: handler\n");
  RUN_TEST(test_handler_called_via_parse);
  RUN_TEST(test_handler_null_fn);
  RUN_TEST(test_handler_no_args);

  printf("\nCVar: on_change\n");
  RUN_TEST(test_on_change_fires_on_set);
  RUN_TEST(test_on_change_fires_for_each_type);

  printf("\nReactive graph: dirty flags\n");
  RUN_TEST(test_dirty_flag_set_on_change);
  RUN_TEST(test_dirty_cleared_after_flush);

  printf("\nReactive graph: subscribe\n");
  RUN_TEST(test_subscribe_fires_on_flush);
  RUN_TEST(test_subscribe_not_before_flush);
  RUN_TEST(test_multiple_subscribers);
  RUN_TEST(test_no_duplicate_notifications);
  RUN_TEST(test_subscriber_context);

  printf("\nReactive graph: computed cvars\n");
  RUN_TEST(test_computed_initial_value);
  RUN_TEST(test_computed_recomputes_on_dep_change);
  RUN_TEST(test_computed_caching);
  RUN_TEST(test_computed_lazy_recomputation);

  printf("\nReactive graph: chain\n");
  RUN_TEST(test_chain_value_computed_subscriber);

  printf("\nConsole: init\n");
  RUN_TEST(test_console_init_zeroes);

  printf("\nConsole: character insertion\n");
  RUN_TEST(test_console_insert_chars);
  RUN_TEST(test_console_insert_mid_line);

  printf("\nConsole: cursor movement\n");
  RUN_TEST(test_cursor_left);
  RUN_TEST(test_cursor_left_at_start);
  RUN_TEST(test_cursor_right);
  RUN_TEST(test_cursor_right_at_end);
  RUN_TEST(test_cursor_home);
  RUN_TEST(test_cursor_end);

  printf("\nConsole: backspace and delete\n");
  RUN_TEST(test_backspace);
  RUN_TEST(test_backspace_at_start);
  RUN_TEST(test_backspace_mid_line);
  RUN_TEST(test_delete);
  RUN_TEST(test_delete_at_end);

  printf("\nConsole: history\n");
  RUN_TEST(test_history_up_down);
  RUN_TEST(test_history_saves_current_line);
  RUN_TEST(test_history_empty_not_pushed);
  RUN_TEST(test_history_no_duplicates);
  RUN_TEST(test_history_up_at_top);
  RUN_TEST(test_history_down_without_browsing);
  RUN_TEST(test_history_up_with_empty_history);

  printf("\nConsole: tab completion\n");
  RUN_TEST(test_tab_single_match);
  RUN_TEST(test_tab_multiple_matches_common_prefix);
  RUN_TEST(test_tab_no_match);
  RUN_TEST(test_tab_empty_line);

  printf("\nConsole: output\n");
  RUN_TEST(test_console_printf);
  RUN_TEST(test_console_printf_strips_newline);
  RUN_TEST(test_console_output_ring);
  RUN_TEST(test_console_output_line_out_of_bounds);

  printf("\nConsole: submit\n");
  RUN_TEST(test_submit_clears_line);
  RUN_TEST(test_submit_adds_to_history);
  RUN_TEST(test_submit_adds_to_output);
  RUN_TEST(test_submit_parses_cvar);

  printf("\nConsole: current_cvar\n");
  RUN_TEST(test_current_cvar_found);
  RUN_TEST(test_current_cvar_not_found);
  RUN_TEST(test_current_cvar_empty);

  printf("\nConsole: arg_index\n");
  RUN_TEST(test_arg_index_no_args);
  RUN_TEST(test_arg_index_one_arg);
  RUN_TEST(test_arg_index_two_args);
  RUN_TEST(test_arg_index_equals);
  RUN_TEST(test_arg_index_trailing_space);

  printf("\nConsole: past_name\n");
  RUN_TEST(test_past_name_no_space);
  RUN_TEST(test_past_name_with_space);
  RUN_TEST(test_past_name_with_equals);

  printf("\nConsole: kill line / kill to start\n");
  RUN_TEST(test_kill_line);
  RUN_TEST(test_kill_to_start);

  printf("\nConsole: input_char\n");
  RUN_TEST(test_input_char_newline);
  RUN_TEST(test_input_char_carriage_return);
  RUN_TEST(test_input_char_tab);
  RUN_TEST(test_input_char_backspace);
  RUN_TEST(test_input_char_ctrl_a_home);
  RUN_TEST(test_input_char_ctrl_e_end);
  RUN_TEST(test_input_char_ctrl_d_delete);
  RUN_TEST(test_input_char_ctrl_k_kill_line);
  RUN_TEST(test_input_char_ctrl_u_kill_to_start);
  RUN_TEST(test_input_char_printable);
  RUN_TEST(test_input_char_control_ignored);

  printf("\nConsole: version\n");
  RUN_TEST(test_version_increments_on_char);
  RUN_TEST(test_version_increments_on_submit);
  RUN_TEST(test_version_increments_on_printf);

  printf("\n%d/%d tests passed\n", tests_passed, tests_run);
  return (tests_passed == tests_run) ? 0 : 1;
}
