# tomlc17 API Reference

## Overview

```c
#include "tomlc17.h"
```

The typical usage pattern:

1. Call `toml_parse()`, `toml_parse_file()`, or `toml_parse_file_ex()` to parse a document.
2. Check `result.ok`. On failure, `result.errmsg` contains a description.
3. Navigate the document tree using `toml_get()` or `toml_seek()`.
4. Call `toml_free()` when done.

**Important:** `toml_free()` must be called on every result returned by a parse or merge
function, even if `result.ok` is false.

All strings and data returned from the API are owned by the `toml_result_t` and remain
valid until `toml_free()` is called on that result.

---

## Types

### `toml_type_t`

```c
typedef enum {
  TOML_UNKNOWN = 0,
  TOML_STRING,
  TOML_INT64,
  TOML_FP64,
  TOML_BOOLEAN,
  TOML_DATE,
  TOML_TIME,
  TOML_DATETIME,
  TOML_DATETIMETZ,
  TOML_ARRAY,
  TOML_TABLE,
} toml_type_t;
```

The type tag carried by every `toml_datum_t`. A datum with `type == TOML_UNKNOWN`
means "not found" or "invalid".

---

### `toml_datum_t`

A node in the parsed document tree.

```c
struct toml_datum_t {
  toml_type_t type;
  uint32_t    flag;   // internal
  int         lineno; // 1-based source line, 0 when synthesized
  int         colno;  // 1-based source column, 0 when synthesized
  const char *source; // source name (e.g. filename), NULL when not provided
  union {
    const char *s;           // TOML_STRING: shorthand for str.ptr
    struct {
      const char *ptr;       // NUL-terminated string
      int len;               // length excluding the NUL terminator
    } str;                   // TOML_STRING
    int64_t int64;           // TOML_INT64
    double  fp64;            // TOML_FP64
    bool    boolean;         // TOML_BOOLEAN
    struct {
      int16_t year, month, day;
      int16_t hour, minute, second;
      int32_t usec;          // microseconds
      int16_t tz;            // timezone offset in minutes
    } ts;                    // TOML_DATE, TOML_TIME, TOML_DATETIME, TOML_DATETIMETZ
    struct {
      int32_t       size;    // number of elements
      toml_datum_t *elem;    // elem[0..size-1]
    } arr;                   // TOML_ARRAY
    struct {
      int32_t        size;   // number of key/value pairs
      const char   **key;    // key[0..size-1]  (NUL-terminated strings)
      int           *len;    // len[0..size-1]  (key lengths)
      toml_datum_t  *value;  // value[0..size-1]
    } tab;                   // TOML_TABLE
  } u;
};
```

**Fields:**

- `type`: The type of this datum (see `toml_type_t`).
- `flag`: Internal use only. Do not read or modify.
- `lineno`: 1-based line number in the source document where this value appeared. Set to 0 for synthesized values.
- `colno`: 1-based column number in the source document where this value appeared. Set to 0 for synthesized values.
- `source`: Name of the document this value came from (e.g. the filename), or
  `NULL` if no name was supplied. Copied into the result and preserved across
  `toml_merge`, so merged documents retain each entry's origin.
- `u`: Union containing the actual value based on the `type`.

**Accessing values by type:**

| `type`           | field(s) to read                         |
|------------------|------------------------------------------|
| `TOML_STRING`    | `u.s` or `u.str.ptr` / `u.str.len`      |
| `TOML_INT64`     | `u.int64`                                |
| `TOML_FP64`      | `u.fp64`                                 |
| `TOML_BOOLEAN`   | `u.boolean`                              |
| `TOML_DATE`      | `u.ts.year`, `.month`, `.day`            |
| `TOML_TIME`      | `u.ts.hour`, `.minute`, `.second`, `.usec` |
| `TOML_DATETIME`  | all `u.ts` fields except `.tz`           |
| `TOML_DATETIMETZ`| all `u.ts` fields                        |
| `TOML_ARRAY`     | `u.arr.size`, `u.arr.elem[]`             |
| `TOML_TABLE`     | `u.tab.size`, `u.tab.key[]`, `u.tab.value[]` |

---

### `toml_result_t`

Returned by every parse function.

```c
struct toml_result_t {
  bool         ok;          // true on success
  toml_datum_t toptab;      // the top-level table; valid when ok == true
  char         errmsg[200]; // error description; valid when ok == false
  void        *__internal;  // internal use only; do not access
};
```

---

### `toml_option_t`

Global options. See [`toml_set_option()`](#toml_set_option).

```c
struct toml_option_t {
  bool check_utf8;                          // validate UTF-8; default: false
  void *(*mem_realloc)(void *ptr, size_t); // default: realloc()
  void (*mem_free)(void *ptr);             // default: free()
};
```

---

## Parsing

### `toml_parse`

```c
toml_result_t toml_parse(const char *src, int len);
```

Parse a TOML document from a NUL-terminated string. `len` is the length of `src`
excluding the NUL terminator. The result must be released with `toml_free()`.

---

### `toml_parse_file`

```c
toml_result_t toml_parse_file(FILE *fp);
```

Parse a TOML document from an open file handle. The caller is responsible for
calling `fclose(fp)`. The result must be released with `toml_free()`.

---

### `toml_parse_file_ex`

```c
toml_result_t toml_parse_file_ex(const char *fname);
```

Parse a TOML document from a file path. Opens and closes the file internally.
Every parsed datum's `source` is set to `fname`. The result must be released
with `toml_free()`.

---

### `toml_parse_named`

```c
toml_result_t toml_parse_named(const char *src, int len, const char *name);
```

Like `toml_parse`, but tags every parsed datum's `source` with `name` (copied
into the result; pass `NULL` for no name). The name survives `toml_merge`.

---

### `toml_parse_file_named`

```c
toml_result_t toml_parse_file_named(FILE *fp, const char *name);
```

Like `toml_parse_file`, but tags datums with `name`. `toml_parse_file_ex`
automatically uses the file path as the name.

---

## Releasing

### `toml_free`

```c
void toml_free(toml_result_t result);
```

Release all memory associated with a `toml_result_t`. Must be called on every
result returned by a parse or merge function, regardless of whether parsing
succeeded. All pointers obtained from this result become invalid after this call.

---

## Querying

### `toml_get`

```c
toml_datum_t toml_get(toml_datum_t table, const char *key);
```

Look up a single key in a table. Returns a datum with `type == TOML_UNKNOWN`
if the key is not found or if `table` is not a `TOML_TABLE`.

---

### `toml_seek`

```c
toml_datum_t toml_seek(toml_datum_t table, const char *multipart_key);
```

Look up a dot-separated key path starting from `table`. For example,
`"server.host"` is equivalent to calling `toml_get` twice.

Constraints:
- Keys must not contain escape characters.
- The total length of `multipart_key` must not exceed 255 bytes.

Returns a datum with `type == TOML_UNKNOWN` if any component is not found.

---

## Merging and Comparing

### `toml_merge`

```c
toml_result_t toml_merge(const toml_result_t *r1, const toml_result_t *r2);
```

Produce a new result that is `r1` overridden by `r2`. Merge rules:

- Key in r2 not in r1 → added to result.
- Key in r2 with a different type than r1 → overrides r1.
- Key is an array of tables → r2's entries are appended to r1's.
- Key is a table → r2's sub-keys are recursively merged into r1's.
- Otherwise → r2 overrides r1.

All three results (`r1`, `r2`, and the returned result) must each be freed
with `toml_free()` independently.

---

### `toml_equiv`

```c
bool toml_equiv(const toml_result_t *r1, const toml_result_t *r2);
```

Return `true` if the two results represent identical documents. Tables are
compared as unordered maps (keys matched by name); array element order is
significant.

---

## Options

### `toml_default_option`

```c
toml_option_t toml_default_option(void);
```

Return the current default options. Use this to obtain a baseline before
modifying individual fields.

---

### `toml_set_option`

```c
void toml_set_option(toml_option_t opt);
```

Replace the global options. This affects all subsequent parse calls.
Not thread-safe; call once during program initialization.

**Example — custom allocator:**

```c
toml_option_t opt = toml_default_option();
opt.mem_realloc = my_realloc;
opt.mem_free    = my_free;
toml_set_option(opt);
```

---

## Deprecated

### `toml_table_find`

```c
toml_datum_t toml_table_find(toml_datum_t table, const char *key);
```

Alias for `toml_get()`. Use `toml_get()` instead.
