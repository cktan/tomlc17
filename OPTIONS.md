# tomlc17 Options

## Setting Options: Custom Allocators

By default, `tomlc17` uses the standard C library `realloc()` and `free()` 
functions for memory management. You can override this behavior by setting 
custom allocator functions using `toml_set_option()`.

This is useful when you need to:
- Use a custom memory pool or arena allocator
- Track memory allocations for debugging
- Enforce memory constraints in embedded or resource-limited environments
- Integrate with a specialized memory management system

### Example: Using Custom Allocators

```c
#include "tomlc17.h"
#include <stdlib.h>

// Custom realloc function
void* my_realloc(void *ptr, size_t size) {
  // Your custom allocation logic here
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, size);
}

// Custom free function
void my_free(void *ptr) {
  // Your custom deallocation logic here
  free(ptr);
}

int main() {
  // Set custom allocators
  toml_option_t opt = toml_default_option();
  opt.mem_realloc = my_realloc;
  opt.mem_free = my_free;
  toml_set_option(opt);

  // Now all subsequent parse calls will use your custom allocators
  toml_result_t result = toml_parse_file_ex("config.toml");
  
  if (result.ok) {
    // Use the parsed data
    toml_free(result);
  }

  return 0;
}
```

### Important Notes

- **Initialization:** Call `toml_set_option()` once during program startup, before 
  any parsing operations.
- **Thread Safety:** `toml_set_option()` is not thread-safe. Set options only during 
  initialization in a single-threaded context.
- **Custom Allocator Requirements:** Your custom `mem_realloc` function should behave 
  like standard `realloc()`:
  - When `size > 0`: allocate or resize memory, return pointer or NULL on failure
  - When `size == 0`: free memory and return NULL
  - Your custom `mem_free` function should handle NULL pointers gracefully

For more details, see the [`toml_set_option()` documentation](API.md#toml_set_option) 
in API.md.
