# Bare Key Characters in TOML v1.1

In TOML v1.1, the definition of a **bare key** (unquoted key) has been significantly expanded from the strict ASCII-only restriction of TOML v1.0 to support internationalization.

## 1. The TOML v1.0 Definition (Legacy)
In TOML v1.0, a bare key could only contain:
*   ASCII letters (`A-Z`, `a-z`)
*   ASCII digits (`0-9`)
*   Underscores (`_`)
*   Dashes (`-`)

## 2. The TOML v1.1 Expansion
TOML v1.1 allows **non-ASCII Unicode characters** in bare keys. The character ranges are largely aligned with the **XML 1.1 `NameChar`** specification, which covers most letters, digits, and combining marks from scripts worldwide.

A `bare-key-char` in TOML v1.1 is defined as:
*   Any character allowed in v1.0 (`[A-Za-z0-9_-]`).
*   Any non-ASCII Unicode codepoint from the following specific ranges:
    *   `U+00B2`, `U+00B3`, `U+00B9` (Superscripts)
    *   `U+00BC` to `U+00BE` (Fractions)
    *   `U+00C0` to `U+00D6`
    *   `U+00D8` to `U+00F6`
    *   `U+00F8` to `U+037D`
    *   `U+037F` to `U+1FFF`
    *   `U+200C` to `U+200D` (Zero-width joiners)
    *   `U+203F` to `U+2040`
    *   `U+2070` to `U+218F`
    *   `U+2460` to `U+24FF`
    *   `U+2C00` to `U+2FEF`
    *   `U+3001` to `U+D7FF`
    *   `U+F900` to `U+FDCF`
    *   `U+FDF0` to `U+FFFD`
    *   `U+10000` to `U+EFFFF` (Supplementary Planes)

## Summary Table

| Feature | TOML v1.0 | TOML v1.1 |
| :--- | :--- | :--- |
| **Allowed Characters** | `[A-Za-z0-9_-]` | `[A-Za-z0-9_-]` + Wide Unicode ranges |
| **Examples** | `key = "val"` | `café = "coffee"`, `ñoño = "val"`, `日本語 = "val"` |
| **Prohibited** | `café = "val"` (must use `"café"`) | Whitespace, `.`, `[`, `]`, `#`, `=`, etc. |

## Implementation Status in `tomlc17`
The `tomlc17` project recently updated its scanner in commit `df7ecd4` to support these TOML v1.1 ranges using a new helper function `is_unicode_bare_key_char()`. This allows the parser to correctly handle unquoted internationalized keys.
