# tomlc17 Audit

## Project Overview

A TOML 1.1 parser in C17. One implementation file (`tomlc17.c`, ~2950 lines), a public header (`tomlc17.h`), and a C++20 wrapper (`tomlcpp.hpp`). The parser uses a single-pass scanner + recursive-descent parser, a memory pool for strings, and heap-allocated arrays for tables/arrays.

---

## Confirmed Bugs

### 1. Sub-second precision truncated at 6 digits — parse fails for 7+

**`src/tomlc17.c:2169–2175`**

`read_time` accumulates fractional seconds into microseconds (6 digits), but when the loop exits on `micro_factor == 0`, the cursor is left *before* the remaining digits. The main parse loop then fails when it tries to scan `ENDL` and encounters the leftover digits instead.

```c
// current code — exits but doesn't skip extra digits
while (isdigit(*p) && micro_factor) {
    *usec += (*p - '0') * micro_factor;
    micro_factor /= 10;
    p++;
}
// need to add:  while (isdigit(*p)) p++;
```

**Confirmed** — `x = 2023-01-01T12:34:56.123456789Z` → `(line 1) ENDL expected`. The TOML 1.1 spec places no limit on sub-second precision; the standard test suite includes nanosecond timestamps.

---

### 2. Unicode bare keys rejected (TOML 1.1 compliance gap)

**`src/tomlc17.c:2599–2607`**

`scan_literal` only accepts `[a-zA-Z0-9_-]`:

```c
while (p < sp->endp && (isalnum(*p) || *p == '_' || *p == '-')) {
    p++;
}
```

TOML 1.1 allows any non-ASCII Unicode codepoint (U+00B2 and up) in bare keys. `café = "x"` currently fails with `(line 1) expect '='` — the scanner stops at the first byte of `é`.

---

## Other Issues

### 3. Stray scratch files at repo root

**FIXED** — Deleted `t.c`, `t`, `a.out` and updated `.gitignore`.

---

### 4. `toml_parse(NULL, n)` crashes

**FIXED** — Added NULL guard for `src` in `toml_parse`.

---

### 5. Three-way REALLOC in `tab_emplace` is partially unsafe

**`src/tomlc17.c:279–300`**

When growing a table, three separate `REALLOC` calls are made for `key[]`, `len[]`, and `value[]`. If the first succeeds (freeing old memory and saving new pointer into `tab`) and the second fails, the arrays have inconsistent allocated sizes. The error path returns `NULL`, the caller propagates the error, and `datum_free` + `pool_destroy` clean up correctly — so there is no memory corruption or leak. But the pattern is fragile and surprising. A cleaner approach is to check all three before updating any field.

---

### 6. `parse_array_table_expr` duplicates descent logic

**`src/tomlc17.c:1412–1460`**

The intermediate-key descent in `parse_array_table_expr` is a hand-rolled copy of `descend_keypart` rather than a call to it. The two code paths handle the same invariants and can diverge silently. Refactoring the array table path to use `descend_keypart` would reduce maintenance surface.

---

### 7. `tab_emplace` has dual-purpose return semantics

When a key already exists, `tab_emplace` returns a pointer to the existing value (useful for `datum_merge`). When used via `tab_add`, the caller checks `pvalue->type != 0` and errors with "duplicate key". This dual use is handled correctly by all callers today, but the function has no documentation or assertion distinguishing the two uses.

---

## Code Quality Observations

- **Memory model is clean**: pool for strings (freed atomically), heap arrays for table/array structure (freed recursively by `datum_free`). No mixing.
- **`toml_free` on failed results is safe**: the bail paths destroy the pool and leave `__internal = NULL`; `FREE(NULL)` is a no-op.
- **`toml_free` takes by value**: callers' structs are not zeroed after free. Expected for a C API, and documented.
- **Commented-out error checks** at lines 1184 and 1205–1207 note v1.1 relaxations (trailing commas in inline tables, newlines in inline tables). These are intentional and should stay.
- **Pool size (`len + 10`)**: adequate — normalized string sizes are always ≤ raw token sizes (escapes compress), so total pool usage ≤ source length.
- **Test coverage**: 39 parser regression cases, separate scankey/scanvalue suites, merge tests, C++ wrapper tests, and the upstream `toml-test` validator (TOML 1.1 mode). The sub-second bug is not in any of these test files.

---

## Priority Summary

| # | Severity | Issue |
|---|----------|-------|
| 1 | **High** | Sub-second precision >6 digits breaks parse |
| 2 | **Medium** | Unicode bare keys rejected (TOML 1.1 gap) |
| 3 | Low | Stray scratch files in working tree |
| 4 | Low | No null guard on `toml_parse` `src` parameter |
| 5 | Low | Three-REALLOC pattern in `tab_emplace` |
| 6 | Low | Array table descent code duplicated |
