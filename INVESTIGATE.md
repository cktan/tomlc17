# tomlc17 Audit

## Project Overview

A TOML 1.1 parser in C17. One implementation file (`tomlc17.c`, ~2950 lines), a public header (`tomlc17.h`), and a C++20 wrapper (`tomlcpp.hpp`). The parser uses a single-pass scanner + recursive-descent parser, a memory pool for strings, and heap-allocated arrays for tables/arrays.

---

## Confirmed Bugs


### 6. `parse_array_table_expr` duplicates descent logic

**`src/tomlc17.c:1412–1460`**

The intermediate-key descent in `parse_array_table_expr` is a hand-rolled copy of `descend_keypart` rather than a call to it. The two code paths handle the same invariants and can diverge silently. Refactoring the array table path to use `descend_keypart` would reduce maintenance surface.

---

### 7. `tab_emplace` has dual-purpose return semantics

**FIXED** — Added comment to `tab_emplace` explaining that it returns existing datums unmodified and that callers must check `datum->type` to detect duplicates vs. fresh slots.

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
