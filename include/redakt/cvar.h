#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum RCVarType {
  R_CVAR_BOOL,
  R_CVAR_INT,
  R_CVAR_FLOAT,
  R_CVAR_STRING,
  R_CVAR_HANDLER,
  R_CVAR_COMPUTED,
} RCVarType;

typedef struct RCVar RCVar;
typedef struct RCVarRegistry RCVarRegistry;
typedef struct RCVarSubscription RCVarSubscription;

typedef void (*RCVarHandler)(RCVar *cvar, const char *args);
typedef void (*RCVarOnChange)(RCVar *cvar);
typedef void (*RCVarSubscribeFn)(RCVar *cvar, void *ctx);

typedef union RCVarValue {
  bool b;
  int32_t i;
  float f;
  char *s;
  RCVarHandler handler;
} RCVarValue;

typedef RCVarValue (*RCVarComputeFn)(RCVar **deps, uint32_t dep_count, void *ctx);
typedef void (*RCVarPrintFn)(void *ctx, const char *fmt, ...);

struct RCVarSubscription {
  RCVarSubscribeFn fn;
  void *ctx;
  RCVarSubscription *next;
};

struct RCVar {
  RCVarRegistry *registry;
  const char *name;
  const char *display_name;
  const char *description;
  RCVarType type;
  RCVarValue value;
  RCVarValue default_value;
  RCVarOnChange on_change;
  RCVarType *arg_types;
  uint32_t arg_count;

  // reactive graph
  bool dirty;
  RCVarSubscription *subscribers;

  // computed cvars
  RCVar **deps;
  uint32_t dep_count;
  RCVarComputeFn compute_fn;
  void *compute_ctx;

  RCVar *next; // hash chain
};

struct RCVarRegistry {
  void *user_data;
  RCVar **buckets;
  uint32_t bucket_count;
  uint32_t count;
  RCVarPrintFn print_fn;
  void *print_ctx;

  // dirty list for deferred flush
  RCVar **dirty_list;
  uint32_t dirty_count;
  uint32_t dirty_capacity;
};

void cvar_registry_init(RCVarRegistry *reg);
void cvar_registry_free(RCVarRegistry *reg);

RCVar *cvar_register_bool(RCVarRegistry *reg, const char *name, const char *desc, bool value);
RCVar *cvar_register_int(RCVarRegistry *reg, const char *name, const char *desc, int32_t value);
RCVar *cvar_register_float(RCVarRegistry *reg, const char *name, const char *desc, float value);
RCVar *cvar_register_string(RCVarRegistry *reg, const char *name, const char *desc, const char *value);
RCVar *cvar_register_handler(RCVarRegistry *reg, const char *name, const char *desc, RCVarHandler handler);
RCVar *cvar_register_computed(RCVarRegistry *reg, const char *name, const char *desc,
                              RCVarType result_type, RCVar **deps, uint32_t dep_count,
                              RCVarComputeFn compute_fn, void *ctx);

RCVar *cvar_find(RCVarRegistry *reg, const char *name);

void cvar_set_bool(RCVar *cvar, bool value);
void cvar_set_int(RCVar *cvar, int32_t value);
void cvar_set_float(RCVar *cvar, float value);
void cvar_set_string(RCVar *cvar, const char *value);

RCVarValue cvar_get(RCVar *cvar);

void cvar_subscribe(RCVar *cvar, RCVarSubscribeFn fn, void *ctx);
void cvar_flush(RCVarRegistry *reg);

bool cvar_parse(RCVarRegistry *reg, const char *line);
void cvar_reset(RCVar *cvar);

RCVar *cvar_define(RCVarRegistry *reg, const char *name, RCVarType type);
bool cvar_type_from_string(const char *str, RCVarType *out);
const char *cvar_type_name(RCVarType type);

uint32_t cvar_autocomplete(RCVarRegistry *reg, const char *prefix, const char **results, uint32_t max_results);
