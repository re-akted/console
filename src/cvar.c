#define _POSIX_C_SOURCE 200809L

#include <redakt/cvar.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define R_CVAR_DEFAULT_BUCKETS 256

static void cvar_printf(RCVarRegistry *reg, const char *fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (reg->print_fn) {
    reg->print_fn(reg->print_ctx, "%s", buf);
  } else {
    fputs(buf, stdout);
  }
}

static uint32_t cvar_hash(const char *name) {
  uint32_t h = 5381;
  while (*name) {
    h = ((h << 5) + h) + (uint8_t)*name++;
  }
  return h;
}

#define R_CVAR_DIRTY_INITIAL_CAPACITY 64

void cvar_registry_init(RCVarRegistry *reg) {
  reg->bucket_count = R_CVAR_DEFAULT_BUCKETS;
  reg->count = 0;
  reg->buckets = calloc(reg->bucket_count, sizeof(RCVar *));
  reg->dirty_capacity = R_CVAR_DIRTY_INITIAL_CAPACITY;
  reg->dirty_count = 0;
  reg->dirty_list = malloc(sizeof(RCVar *) * reg->dirty_capacity);
}

static void cvar_mark_dirty(RCVar *cvar) {
  if (cvar->dirty) {
    return;
  }

  cvar->dirty = true;
  RCVarRegistry *reg = cvar->registry;

  if (reg->dirty_count == reg->dirty_capacity) {
    reg->dirty_capacity *= 2;
    reg->dirty_list = realloc(reg->dirty_list, sizeof(RCVar *) * reg->dirty_capacity);
  }

  reg->dirty_list[reg->dirty_count++] = cvar;
}

void cvar_registry_free(RCVarRegistry *reg) {
  for (uint32_t i = 0; i < reg->bucket_count; i++) {
    RCVar *cvar = reg->buckets[i];
    while (cvar) {
      RCVar *next = cvar->next;
      if (cvar->type == R_CVAR_STRING) {
        free(cvar->value.s);
        free(cvar->default_value.s);
      }
      RCVarSubscription *sub = cvar->subscribers;
      while (sub) {
        RCVarSubscription *next_sub = sub->next;
        free(sub);
        sub = next_sub;
      }
      free(cvar->deps);
      free(cvar->arg_types);
      free(cvar);
      cvar = next;
    }
  }
  free(reg->dirty_list);
  free(reg->buckets);
  memset(reg, 0, sizeof(*reg));
}

static RCVar *cvar_alloc(RCVarRegistry *reg, const char *name, const char *desc, RCVarType type) {
  RCVar *cvar = calloc(1, sizeof(struct RCVar));
  cvar->registry = reg;
  cvar->name = name;
  cvar->description = desc;
  cvar->type = type;

  uint32_t idx = cvar_hash(name) % reg->bucket_count;
  cvar->next = reg->buckets[idx];
  reg->buckets[idx] = cvar;
  reg->count++;

  return cvar;
}

RCVar *cvar_register_bool(RCVarRegistry *reg, const char *name, const char *desc, bool value) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_BOOL);
  cvar->value.b = value;
  cvar->default_value.b = value;
  return cvar;
}

RCVar *cvar_register_int(RCVarRegistry *reg, const char *name, const char *desc, int32_t value) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_INT);
  cvar->value.i = value;
  cvar->default_value.i = value;
  return cvar;
}

RCVar *cvar_register_float(RCVarRegistry *reg, const char *name, const char *desc, float value) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_FLOAT);
  cvar->value.f = value;
  cvar->default_value.f = value;
  return cvar;
}

RCVar *cvar_register_string(RCVarRegistry *reg, const char *name, const char *desc, const char *value) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_STRING);
  cvar->value.s = strdup(value);
  cvar->default_value.s = strdup(value);
  return cvar;
}

RCVar *cvar_register_handler(RCVarRegistry *reg, const char *name, const char *desc, RCVarHandler handler) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_HANDLER);
  cvar->value.handler = handler;
  cvar->default_value.handler = handler;
  return cvar;
}

RCVar *cvar_find(RCVarRegistry *reg, const char *name) {
  uint32_t idx = cvar_hash(name) % reg->bucket_count;
  RCVar *cvar = reg->buckets[idx];
  while (cvar) {
    if (strcmp(cvar->name, name) == 0) {
      return cvar;
    }
    cvar = cvar->next;
  }
  return NULL;
}

void cvar_set_bool(RCVar *cvar, bool value) {
  cvar->value.b = value;
  cvar_mark_dirty(cvar);
  if (cvar->on_change) {
    cvar->on_change(cvar);
  }
}

void cvar_set_int(RCVar *cvar, int32_t value) {
  cvar->value.i = value;
  cvar_mark_dirty(cvar);
  if (cvar->on_change) {
    cvar->on_change(cvar);
  }
}

void cvar_set_float(RCVar *cvar, float value) {
  cvar->value.f = value;
  cvar_mark_dirty(cvar);
  if (cvar->on_change) {
    cvar->on_change(cvar);
  }
}

void cvar_set_string(RCVar *cvar, const char *value) {
  free(cvar->value.s);
  cvar->value.s = strdup(value);
  cvar_mark_dirty(cvar);
  if (cvar->on_change) {
    cvar->on_change(cvar);
  }
}

void cvar_reset(RCVar *cvar) {
  switch (cvar->type) {
  case R_CVAR_BOOL:
    cvar_set_bool(cvar, cvar->default_value.b);
    break;
  case R_CVAR_INT:
    cvar_set_int(cvar, cvar->default_value.i);
    break;
  case R_CVAR_FLOAT:
    cvar_set_float(cvar, cvar->default_value.f);
    break;
  case R_CVAR_STRING:
    cvar_set_string(cvar, cvar->default_value.s);
    break;
  case R_CVAR_HANDLER:
  case R_CVAR_COMPUTED:
    break;
  }
}

static void cvar_print(RCVar *cvar) {
  RCVarRegistry *reg = cvar->registry;
  switch (cvar->type) {
  case R_CVAR_BOOL:
    cvar_printf(reg, "%s = %s\n", cvar->name, cvar->value.b ? "true" : "false");
    break;
  case R_CVAR_INT:
    cvar_printf(reg, "%s = %d\n", cvar->name, cvar->value.i);
    break;
  case R_CVAR_FLOAT:
    cvar_printf(reg, "%s = %.4f\n", cvar->name, (double)cvar->value.f);
    break;
  case R_CVAR_STRING:
    cvar_printf(reg, "%s = \"%s\"\n", cvar->name, cvar->value.s);
    break;
  case R_CVAR_HANDLER:
    cvar_printf(reg, "%s [handler]\n", cvar->name);
    break;
  case R_CVAR_COMPUTED: {
    RCVarValue v = cvar_get(cvar);
    cvar_printf(reg, "%s = (computed)\n", cvar->name);
    (void)v;
    break;
  }
  }
}

static RCVarType cvar_infer_type(const char *str) {
  if (strcmp(str, "true") == 0 || strcmp(str, "false") == 0) {
    return R_CVAR_BOOL;
  }

  char *end;
  strtol(str, &end, 10);

  if (*end == '\0' && end != str) {
    return R_CVAR_INT;
  }

  strtof(str, &end);

  if (*end == '\0' && end != str) {
    return R_CVAR_FLOAT;
  }

  return R_CVAR_STRING;
}

static RCVar *cvar_create_from_string(RCVarRegistry *reg, const char *name, const char *str) {
  RCVarType type = cvar_infer_type(str);

  switch (type) {
  case R_CVAR_BOOL:
    return cvar_register_bool(reg, strdup(name), NULL, strcmp(str, "true") == 0);
  case R_CVAR_INT:
    return cvar_register_int(reg, strdup(name), NULL, (int32_t)strtol(str, NULL, 10));
  case R_CVAR_FLOAT:
    return cvar_register_float(reg, strdup(name), NULL, strtof(str, NULL));
  case R_CVAR_STRING:
    return cvar_register_string(reg, strdup(name), NULL, str);
  case R_CVAR_HANDLER:
  case R_CVAR_COMPUTED:
    return NULL;
  }

  return NULL;
}

static void cvar_set_from_string(RCVar *cvar, const char *str) {
  switch (cvar->type) {
  case R_CVAR_BOOL:
    cvar_set_bool(cvar, strcmp(str, "true") == 0 || strcmp(str, "1") == 0);
    break;
  case R_CVAR_INT:
    cvar_set_int(cvar, (int32_t)strtol(str, NULL, 10));
    break;
  case R_CVAR_FLOAT:
    cvar_set_float(cvar, strtof(str, NULL));
    break;
  case R_CVAR_STRING:
    cvar_set_string(cvar, str);
    break;
  case R_CVAR_HANDLER:
  case R_CVAR_COMPUTED:
    break;
  }
}

bool cvar_parse(RCVarRegistry *reg, const char *line) {
  while (*line == ' ') {
    line++;
  }

  if (*line == '\0') {
    return false;
  }

  char name[256];
  const char *rest = line;
  uint32_t len = 0;

  while (*rest && *rest != ' ' && *rest != '=' && len < sizeof(name) - 1) {
    name[len++] = *rest++;
  }
  name[len] = '\0';

  while (*rest == ' ') {
    rest++;
  }

  // skip '=' if present
  if (*rest == '=') {
    rest++;
    while (*rest == ' ') {
      rest++;
    }
  }

  RCVar *cvar = cvar_find(reg, name);

  if (!cvar) {
    if (*rest == '\0') {
      cvar_printf(reg, "unknown cvar: %s\n", name);
      return false;
    }

    cvar = cvar_create_from_string(reg, name, rest);
    if (cvar) {
      cvar_print(cvar);
    }
    return cvar != NULL;
  }

  if (cvar->type == R_CVAR_HANDLER) {
    if (cvar->value.handler) {
      cvar->value.handler(cvar, rest);
    }
    return true;
  }

  if (*rest == '\0') {
    cvar_print(cvar);
    return true;
  }

  cvar_set_from_string(cvar, rest);
  return true;
}

bool cvar_type_from_string(const char *str, RCVarType *out) {
  if (strcmp(str, "bool") == 0) { *out = R_CVAR_BOOL; return true; }
  if (strcmp(str, "int") == 0) { *out = R_CVAR_INT; return true; }
  if (strcmp(str, "float") == 0) { *out = R_CVAR_FLOAT; return true; }
  if (strcmp(str, "string") == 0) { *out = R_CVAR_STRING; return true; }
  return false;
}

const char *cvar_type_name(RCVarType type) {
  switch (type) {
  case R_CVAR_BOOL:    return "bool";
  case R_CVAR_INT:     return "int";
  case R_CVAR_FLOAT:   return "float";
  case R_CVAR_STRING:  return "string";
  case R_CVAR_HANDLER:  return "handler";
  case R_CVAR_COMPUTED: return "computed";
  }
  return "unknown";
}

RCVar *cvar_define(RCVarRegistry *reg, const char *name, RCVarType type) {
  RCVar *existing = cvar_find(reg, name);
  if (existing) {
    return existing;
  }

  switch (type) {
  case R_CVAR_BOOL:    return cvar_register_bool(reg, strdup(name), NULL, false);
  case R_CVAR_INT:     return cvar_register_int(reg, strdup(name), NULL, 0);
  case R_CVAR_FLOAT:   return cvar_register_float(reg, strdup(name), NULL, 0.0f);
  case R_CVAR_STRING:  return cvar_register_string(reg, strdup(name), NULL, "");
  case R_CVAR_HANDLER: return cvar_register_handler(reg, strdup(name), NULL, NULL);
  case R_CVAR_COMPUTED: return NULL;
  }

  return NULL;
}

RCVar *cvar_register_computed(RCVarRegistry *reg, const char *name, const char *desc,
                              RCVarType result_type, RCVar **deps, uint32_t dep_count,
                              RCVarComputeFn compute_fn, void *ctx) {
  RCVar *cvar = cvar_alloc(reg, name, desc, R_CVAR_COMPUTED);
  cvar->compute_fn = compute_fn;
  cvar->compute_ctx = ctx;
  cvar->dep_count = dep_count;
  cvar->dirty = true;

  if (dep_count > 0) {
    cvar->deps = malloc(sizeof(RCVar *) * dep_count);
    memcpy(cvar->deps, deps, sizeof(RCVar *) * dep_count);
  }

  // store result type in default_value.i for reference
  cvar->default_value.i = (int32_t)result_type;

  // compute initial value
  cvar->value = compute_fn(cvar->deps, cvar->dep_count, ctx);
  cvar->dirty = false;

  return cvar;
}

RCVarValue cvar_get(RCVar *cvar) {
  if (cvar->type == R_CVAR_COMPUTED && cvar->dirty && cvar->compute_fn) {
    cvar->value = cvar->compute_fn(cvar->deps, cvar->dep_count, cvar->compute_ctx);
    cvar->dirty = false;
  }
  return cvar->value;
}

void cvar_subscribe(RCVar *cvar, RCVarSubscribeFn fn, void *ctx) {
  RCVarSubscription *sub = malloc(sizeof(RCVarSubscription));
  sub->fn = fn;
  sub->ctx = ctx;
  sub->next = cvar->subscribers;
  cvar->subscribers = sub;
}

void cvar_flush(RCVarRegistry *reg) {
  // first pass: invalidate computed cvars that depend on dirty values
  for (uint32_t i = 0; i < reg->dirty_count; i++) {
    RCVar *dirty = reg->dirty_list[i];

    // walk all cvars looking for computed ones that depend on this dirty cvar
    for (uint32_t b = 0; b < reg->bucket_count; b++) {
      RCVar *c = reg->buckets[b];
      while (c) {
        if (c->type == R_CVAR_COMPUTED) {
          for (uint32_t d = 0; d < c->dep_count; d++) {
            if (c->deps[d] == dirty) {
              cvar_mark_dirty(c);
              break;
            }
          }
        }
        c = c->next;
      }
    }
  }

  // second pass: notify subscribers
  for (uint32_t i = 0; i < reg->dirty_count; i++) {
    RCVar *cvar = reg->dirty_list[i];
    RCVarSubscription *sub = cvar->subscribers;
    while (sub) {
      sub->fn(cvar, sub->ctx);
      sub = sub->next;
    }
    cvar->dirty = false;
  }

  reg->dirty_count = 0;
}

uint32_t cvar_autocomplete(RCVarRegistry *reg, const char *prefix, const char **results, uint32_t max_results) {
  uint32_t count = 0;
  size_t prefix_len = strlen(prefix);

  for (uint32_t i = 0; i < reg->bucket_count && count < max_results; i++) {
    RCVar *cvar = reg->buckets[i];
    while (cvar && count < max_results) {
      if (strncmp(cvar->name, prefix, prefix_len) == 0) {
        results[count++] = cvar->name;
      }
      cvar = cvar->next;
    }
  }

  return count;
}
