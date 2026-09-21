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
- **Custom Allocator Requirements:**
  - `mem_realloc`: Called with `size > 0` to allocate (when `ptr == NULL`) or resize (when `ptr != NULL`) memory. Returns a pointer to the allocated memory, or `NULL` on failure. `tomlc17` never passes `size == 0`.
  - `mem_free`: Called to free memory previously allocated by `mem_realloc`. Must handle `NULL` pointers gracefully.

For more details, see the [`toml_set_option()` documentation](API.md#toml_set_option) 
in API.md.
