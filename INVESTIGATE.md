# tomlc17 Audit

## Project Overview

A TOML 1.1 parser in C17. One implementation file (`tomlc17.c`, ~2950 lines), a public header (`tomlc17.h`), and a C++20 wrapper (`tomlcpp.hpp`). The parser uses a single-pass scanner + recursive-descent parser, a memory pool for strings, and heap-allocated arrays for tables/arrays.

---

## Confirmed Bugs

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

