/* Copyright (c) 2024-2026, CK Tan.
 * https://github.com/cktan/tomlc17/blob/main/LICENSE
 */

/*
 *  tomlc99 compatibility layer on top of tomlc17.
 *
 *  This file implements the API declared in tomlc99's toml.h (copied
 *  here unchanged), except for the deprecated raw API (toml_raw_in,
 *  toml_raw_at, toml_rto*), which is not provided.
 *
 *  tomlc17 and tomlc99 both define toml_datum_t, toml_parse,
 *  toml_parse_file and toml_free. To keep the two apart, this file
 *  includes tomlc17.c directly with all of tomlc17's public names
 *  renamed to toml17_*, so that the only toml_* names the resulting
 *  library exports are tomlc99's.
 */

#define toml_datum_t toml17_datum_t
#define toml_parse toml17_parse
#define toml_parse_named toml17_parse_named
#define toml_parse_file toml17_parse_file
#define toml_parse_file_named toml17_parse_file_named
#define toml_parse_file_ex toml17_parse_file_ex
#define toml_free toml17_free
#define toml_get toml17_get
#define toml_seek toml17_seek
#define toml_table_find toml17_table_find
#define toml_merge toml17_merge
#define toml_equiv toml17_equiv
#define toml_default_option toml17_default_option
#define toml_set_option toml17_set_option
#define DATUM_ZERO toml17_DATUM_ZERO
#include "../../src/tomlc17.c"
#undef toml_datum_t
#undef toml_parse
#undef toml_parse_file
#undef toml_free
#undef TOML_EXTERN

#include "toml.h"

/* Allocator for everything this layer hands out or keeps on the
 * side. Set by toml_set_memutil(). */
static void *(*ppmalloc)(size_t) = malloc;
static void (*ppfree)(void *) = free;

/*
 *  The tomlc17 tree has no parent links, so a node cannot tell its own
 *  key, and tomlc99 groups a table's entries as key-values, then arrays,
 *  then tables. A small mirror tree, built once after parsing, supplies
 *  both. Each mirror node points at the tomlc17 datum it wraps.
 */
struct toml_table_t {
  const char *key;         /* NULL for root and inline tables in arrays */
  const toml17_datum_t *d; /* the TOML_TABLE datum */
  toml_result_t *res;      /* root only: the owning tomlc17 result */
  int nkval, narr, ntab;
  void **child; /* child[i] wraps d->u.tab.value[i] if table/array */
  int *order;   /* indices into d->u.tab: key-values, arrays, tables */
};

struct toml_array_t {
  const char *key;         /* NULL for arrays nested in arrays */
  const toml17_datum_t *d; /* the TOML_ARRAY datum */
  char kind;               /* 't'able, 'a'rray, 'v'alue, 'm'ixed, 0 if empty */
  char type;               /* see toml_array_type() */
  void **child;            /* child[i] wraps d->u.arr.elem[i] if table/array */
};

static void mirror_free_table(toml_table_t *tab);
static void mirror_free_array(toml_array_t *arr);

static void mirror_free_child(const toml17_datum_t *d, void *child) {
  if (d->type == TOML_TABLE) {
    mirror_free_table(child);
  } else if (d->type == TOML_ARRAY) {
    mirror_free_array(child);
  }
}

static void mirror_free_table(toml_table_t *tab) {
  if (!tab) {
    return;
  }
  for (int i = 0; i < tab->d->u.tab.size; i++) {
    mirror_free_child(&tab->d->u.tab.value[i], tab->child[i]);
  }
  ppfree(tab);
}

static void mirror_free_array(toml_array_t *arr) {
  if (!arr) {
    return;
  }
  for (int i = 0; i < arr->d->u.arr.size; i++) {
    mirror_free_child(&arr->d->u.arr.elem[i], arr->child[i]);
  }
  ppfree(arr);
}

static toml_table_t *mirror_table(const toml17_datum_t *d, const char *key);
static toml_array_t *mirror_array(const toml17_datum_t *d, const char *key);

/* Build the mirror node for d if it is a table or array. Return 0 on
 * success, -1 if out of memory. */
static int mirror_child(const toml17_datum_t *d, const char *key,
                        void **child) {
  *child = NULL;
  if (d->type == TOML_TABLE) {
    *child = mirror_table(d, key);
  } else if (d->type == TOML_ARRAY) {
    *child = mirror_array(d, key);
  } else {
    return 0;
  }
  return *child ? 0 : -1;
}

static toml_table_t *mirror_table(const toml17_datum_t *d, const char *key) {
  int n = d->u.tab.size;
  toml_table_t *tab =
      ppmalloc(sizeof(*tab) + n * sizeof(void *) + n * sizeof(int));
  if (!tab) {
    return NULL;
  }
  memset(tab, 0, sizeof(*tab));
  tab->key = key;
  tab->d = d;
  tab->child = (void **)(tab + 1);
  tab->order = (int *)(tab->child + n);
  for (int i = 0; i < n; i++) {
    tab->child[i] = NULL;
  }

  int k = 0;
  for (int pass = 0; pass < 3; pass++) {
    for (int i = 0; i < n; i++) {
      toml_type_t t = d->u.tab.value[i].type;
      int group = (t == TOML_TABLE) ? 2 : (t == TOML_ARRAY) ? 1 : 0;
      if (group == pass) {
        tab->order[k++] = i;
      }
    }
  }

  for (int i = 0; i < n; i++) {
    toml_type_t t = d->u.tab.value[i].type;
    tab->nkval += (t != TOML_TABLE && t != TOML_ARRAY);
    tab->narr += (t == TOML_ARRAY);
    tab->ntab += (t == TOML_TABLE);
    if (mirror_child(&d->u.tab.value[i], d->u.tab.key[i], &tab->child[i])) {
      mirror_free_table(tab);
      return NULL;
    }
  }
  return tab;
}

/* tomlc99's one-letter code for a scalar's type. */
static char value_type(toml_type_t t) {
  switch (t) {
  case TOML_STRING:
    return 's';
  case TOML_INT64:
    return 'i';
  case TOML_FP64:
    return 'd';
  case TOML_BOOLEAN:
    return 'b';
  case TOML_DATE:
    return 'D';
  case TOML_TIME:
    return 't';
  case TOML_DATETIME:
  case TOML_DATETIMETZ:
    return 'T';
  default:
    return 'u';
  }
}

static toml_array_t *mirror_array(const toml17_datum_t *d, const char *key) {
  int n = d->u.arr.size;
  toml_array_t *arr = ppmalloc(sizeof(*arr) + n * sizeof(void *));
  if (!arr) {
    return NULL;
  }
  memset(arr, 0, sizeof(*arr));
  arr->key = key;
  arr->d = d;
  arr->child = (void **)(arr + 1);
  for (int i = 0; i < n; i++) {
    arr->child[i] = NULL;
  }

  for (int i = 0; i < n; i++) {
    const toml17_datum_t *e = &d->u.arr.elem[i];
    char kind = (e->type == TOML_TABLE)   ? 't'
                : (e->type == TOML_ARRAY) ? 'a'
                                          : 'v';
    arr->kind = (i == 0 || arr->kind == kind) ? kind : 'm';
    if (kind == 'v') {
      char type = value_type(e->type);
      arr->type = (i == 0 || arr->type == type) ? type : 'm';
    }

    // tomlc99 names the tables of a [[header]] array "__anon__", and
    // leaves inline tables and nested arrays unnamed.
    const char *ekey = NULL;
    if (e->type == TOML_TABLE && !(e->flag & TOML_FLAG_INLINED)) {
      ekey = "__anon__";
    }
    if (mirror_child(e, ekey, &arr->child[i])) {
      mirror_free_array(arr);
      return NULL;
    }
  }
  return arr;
}

/* Turn a tomlc17 result into a tomlc99 root table. Consumes r. */
static toml_table_t *make_root(toml_result_t r, char *errbuf, int errbufsz) {
  if (!r.ok) {
    if (errbufsz > 0) {
      snprintf(errbuf, errbufsz, "%s", r.errmsg);
    }
    toml17_free(r);
    return NULL;
  }

  toml_result_t *res = ppmalloc(sizeof(*res));
  toml_table_t *root = NULL;
  if (res) {
    *res = r;
    root = mirror_table(&res->toptab, NULL);
  }
  if (!root) {
    if (errbufsz > 0) {
      snprintf(errbuf, errbufsz, "out of memory");
    }
    ppfree(res);
    toml17_free(r);
    return NULL;
  }
  root->res = res;
  return root;
}

toml_table_t *toml_parse(char *conf, char *errbuf, int errbufsz) {
  if (errbufsz > 0) {
    errbuf[0] = 0;
  }
  size_t len = strlen(conf);
  if (len > INT_MAX) {
    if (errbufsz > 0) {
      snprintf(errbuf, errbufsz, "input is too big");
    }
    return NULL;
  }
  return make_root(toml17_parse(conf, (int)len), errbuf, errbufsz);
}

toml_table_t *toml_parse_file(FILE *fp, char *errbuf, int errbufsz) {
  if (errbufsz > 0) {
    errbuf[0] = 0;
  }
  return make_root(toml17_parse_file(fp), errbuf, errbufsz);
}

void toml_free(toml_table_t *tab) {
  if (!tab) {
    return;
  }
  toml_result_t *res = tab->res;
  mirror_free_table(tab);
  toml17_free(*res);
  ppfree(res);
}

/*-----------------------------------------------------------------
 *  Lookups
 */

/* Index of key in tab, or -1 if absent. */
static int find_key(const toml_table_t *tab, const char *key) {
  const toml17_datum_t *d = tab->d;
  int len = strlen(key);
  for (int i = 0; i < d->u.tab.size; i++) {
    if (d->u.tab.len[i] == len && 0 == memcmp(d->u.tab.key[i], key, len)) {
      return i;
    }
  }
  return -1;
}

/* The scalar value of key in tab, or NULL if absent or not a scalar. */
static const toml17_datum_t *value_in(const toml_table_t *tab,
                                      const char *key) {
  int i = find_key(tab, key);
  if (i < 0) {
    return NULL;
  }
  const toml17_datum_t *v = &tab->d->u.tab.value[i];
  return (v->type == TOML_TABLE || v->type == TOML_ARRAY) ? NULL : v;
}

/* The scalar value at idx in arr, or NULL if out of range or not a
 * scalar. */
static const toml17_datum_t *value_at(const toml_array_t *arr, int idx) {
  if (!(0 <= idx && idx < arr->d->u.arr.size)) {
    return NULL;
  }
  const toml17_datum_t *v = &arr->d->u.arr.elem[idx];
  return (v->type == TOML_TABLE || v->type == TOML_ARRAY) ? NULL : v;
}

/*-----------------------------------------------------------------
 *  Conversions from a tomlc17 datum to a tomlc99 datum.
 *  v may be NULL; the result then has ok == 0.
 */

static toml_datum_t get_string(const toml17_datum_t *v) {
  toml_datum_t ret;
  memset(&ret, 0, sizeof(ret));
  if (v && v->type == TOML_STRING) {
    int len = v->u.str.len;
    ret.u.s = ppmalloc(len + 1);
    if (ret.u.s) {
      memcpy(ret.u.s, v->u.str.ptr, len);
      ret.u.s[len] = 0;
      ret.ok = 1;
    }
  }
  return ret;
}

static toml_datum_t get_bool(const toml17_datum_t *v) {
  toml_datum_t ret;
  memset(&ret, 0, sizeof(ret));
  if (v && v->type == TOML_BOOLEAN) {
    ret.u.b = v->u.boolean;
    ret.ok = 1;
  }
  return ret;
}

static toml_datum_t get_int(const toml17_datum_t *v) {
  toml_datum_t ret;
  memset(&ret, 0, sizeof(ret));
  if (v && v->type == TOML_INT64) {
    ret.u.i = v->u.int64;
    ret.ok = 1;
  }
  return ret;
}

/* Like tomlc99, integers are also accepted as doubles. */
static toml_datum_t get_double(const toml17_datum_t *v) {
  toml_datum_t ret;
  memset(&ret, 0, sizeof(ret));
  if (v && v->type == TOML_FP64) {
    ret.u.d = v->u.fp64;
    ret.ok = 1;
  } else if (v && v->type == TOML_INT64) {
    ret.u.d = (double)v->u.int64;
    ret.ok = 1;
  }
  return ret;
}

static toml_datum_t get_timestamp(const toml17_datum_t *v) {
  toml_datum_t ret;
  memset(&ret, 0, sizeof(ret));
  if (!v) {
    return ret;
  }
  int has_date = (v->type == TOML_DATE || v->type == TOML_DATETIME ||
                  v->type == TOML_DATETIMETZ);
  int has_time = (v->type == TOML_TIME || v->type == TOML_DATETIME ||
                  v->type == TOML_DATETIMETZ);
  if (!has_date && !has_time) {
    return ret;
  }

  toml_timestamp_t *ts = ppmalloc(sizeof(*ts));
  if (!ts) {
    return ret;
  }
  memset(ts, 0, sizeof(*ts));
  if (has_date) {
    ts->__buffer.year = v->u.ts.year;
    ts->__buffer.month = v->u.ts.month;
    ts->__buffer.day = v->u.ts.day;
    ts->year = &ts->__buffer.year;
    ts->month = &ts->__buffer.month;
    ts->day = &ts->__buffer.day;
  }
  if (has_time) {
    ts->__buffer.hour = v->u.ts.hour;
    ts->__buffer.minute = v->u.ts.minute;
    ts->__buffer.second = v->u.ts.second;
    ts->hour = &ts->__buffer.hour;
    ts->minute = &ts->__buffer.minute;
    ts->second = &ts->__buffer.second;
    // tomlc99 sets millisec only when the source has a fraction; the
    // closest tomlc17 can tell is a non-zero fraction.
    if (v->u.ts.usec) {
      ts->__buffer.millisec = v->u.ts.usec / 1000;
      ts->millisec = &ts->__buffer.millisec;
    }
  }
  if (v->type == TOML_DATETIMETZ) {
    // tomlc17 keeps only the offset in minutes, so a zero offset is
    // always reported as "Z".
    int tz = v->u.ts.tz;
    if (tz == 0) {
      snprintf(ts->__buffer.z, sizeof(ts->__buffer.z), "Z");
    } else {
      char sign = tz < 0 ? '-' : '+';
      tz = tz < 0 ? -tz : tz;
      snprintf(ts->__buffer.z, sizeof(ts->__buffer.z), "%c%02d:%02d", sign,
               tz / 60, tz % 60);
    }
    ts->z = ts->__buffer.z;
  }
  ret.u.ts = ts;
  ret.ok = 1;
  return ret;
}

/*-----------------------------------------------------------------
 *  On arrays
 */

int toml_array_nelem(const toml_array_t *arr) { return arr->d->u.arr.size; }

toml_datum_t toml_string_at(const toml_array_t *arr, int idx) {
  return get_string(value_at(arr, idx));
}

toml_datum_t toml_bool_at(const toml_array_t *arr, int idx) {
  return get_bool(value_at(arr, idx));
}

toml_datum_t toml_int_at(const toml_array_t *arr, int idx) {
  return get_int(value_at(arr, idx));
}

toml_datum_t toml_double_at(const toml_array_t *arr, int idx) {
  return get_double(value_at(arr, idx));
}

toml_datum_t toml_timestamp_at(const toml_array_t *arr, int idx) {
  return get_timestamp(value_at(arr, idx));
}

toml_array_t *toml_array_at(const toml_array_t *arr, int idx) {
  if (!(0 <= idx && idx < arr->d->u.arr.size)) {
    return NULL;
  }
  return arr->d->u.arr.elem[idx].type == TOML_ARRAY ? arr->child[idx] : NULL;
}

toml_table_t *toml_table_at(const toml_array_t *arr, int idx) {
  if (!(0 <= idx && idx < arr->d->u.arr.size)) {
    return NULL;
  }
  return arr->d->u.arr.elem[idx].type == TOML_TABLE ? arr->child[idx] : NULL;
}

char toml_array_kind(const toml_array_t *arr) { return arr->kind; }

char toml_array_type(const toml_array_t *arr) {
  return arr->kind == 'v' ? arr->type : 0;
}

const char *toml_array_key(const toml_array_t *arr) {
  return arr ? arr->key : NULL;
}

/*-----------------------------------------------------------------
 *  On tables
 */

const char *toml_key_in(const toml_table_t *tab, int keyidx) {
  if (!(0 <= keyidx && keyidx < tab->d->u.tab.size)) {
    return NULL;
  }
  return tab->d->u.tab.key[tab->order[keyidx]];
}

int toml_key_exists(const toml_table_t *tab, const char *key) {
  return find_key(tab, key) >= 0;
}

toml_datum_t toml_string_in(const toml_table_t *arr, const char *key) {
  return get_string(value_in(arr, key));
}

toml_datum_t toml_bool_in(const toml_table_t *arr, const char *key) {
  return get_bool(value_in(arr, key));
}

toml_datum_t toml_int_in(const toml_table_t *arr, const char *key) {
  return get_int(value_in(arr, key));
}

toml_datum_t toml_double_in(const toml_table_t *arr, const char *key) {
  return get_double(value_in(arr, key));
}

toml_datum_t toml_timestamp_in(const toml_table_t *arr, const char *key) {
  return get_timestamp(value_in(arr, key));
}

toml_array_t *toml_array_in(const toml_table_t *tab, const char *key) {
  int i = find_key(tab, key);
  return (i >= 0 && tab->d->u.tab.value[i].type == TOML_ARRAY) ? tab->child[i]
                                                               : NULL;
}

toml_table_t *toml_table_in(const toml_table_t *tab, const char *key) {
  int i = find_key(tab, key);
  return (i >= 0 && tab->d->u.tab.value[i].type == TOML_TABLE) ? tab->child[i]
                                                               : NULL;
}

int toml_table_nkval(const toml_table_t *tab) { return tab->nkval; }

int toml_table_narr(const toml_table_t *tab) { return tab->narr; }

int toml_table_ntab(const toml_table_t *tab) { return tab->ntab; }

const char *toml_table_key(const toml_table_t *tab) {
  return tab ? tab->key : NULL;
}

/*--------------------------------------------------------------
 *  Misc
 */

/* Unlike tomlc99, only code points up to U+10FFFF (at most 4 bytes)
 * are accepted, and overlong encodings are rejected. */
int toml_utf8_to_ucs(const char *orig, int len, int64_t *ret) {
  uint32_t code;
  int n = utf8_to_ucs(orig, len, &code);
  if (n < 0) {
    return -1;
  }
  *ret = code;
  return n;
}

int toml_ucs_to_utf8(int64_t code, char buf[6]) {
  if (!(0 <= code && code <= 0x10FFFF)) {
    return -1;
  }
  return ucs_to_utf8((uint32_t)code, buf);
}

/*
 *  tomlc17 allocates through realloc/free; tomlc99 users supply
 *  malloc/free. Emulate realloc by recording each block's size in a
 *  header in front of it.
 */
typedef union {
  size_t size;
  max_align_t align;
} memhdr_t;

static void *memutil_realloc(void *ptr, size_t size) {
  memhdr_t *h = ppmalloc(sizeof(memhdr_t) + size);
  if (!h) {
    return NULL;
  }
  h->size = size;
  if (ptr) {
    memhdr_t *old = (memhdr_t *)ptr - 1;
    memcpy(h + 1, ptr, old->size < size ? old->size : size);
    ppfree(old);
  }
  return h + 1;
}

static void memutil_free(void *ptr) {
  if (ptr) {
    ppfree((memhdr_t *)ptr - 1);
  }
}

/* Must be called before any parse, as with tomlc99. */
void toml_set_memutil(void *(*xxmalloc)(size_t), void (*xxfree)(void *)) {
  if (xxmalloc) {
    ppmalloc = xxmalloc;
  }
  if (xxfree) {
    ppfree = xxfree;
  }
  toml_option.mem_realloc = memutil_realloc;
  toml_option.mem_free = memutil_free;
}
