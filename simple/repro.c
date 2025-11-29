#include "../src/tomlc17.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const char *PATH = "/tmp/t.toml";

static void setup() {
  const char *text =
      "[default]\n"
      "\n"
      "[wayland_displays.\"$WAYLAND_DISPLAY\"]\n"
      "seats = [ \"$XDG_SEAT\" ] \n"
      "[[clipboards.Default.mime_type_groups]]\n"
      "group = [ \"TEXT\", \"STRING\", \"UTF8_STRING\", \"text/plain\" ]\n"
      "xxxx xx xx\n";

  (void)text;
  FILE *fp = fopen(PATH, "w");
  fprintf(fp, "%s", text);
  fclose(fp);
}

static void run() {

  toml_result_t root = toml_parse_file_ex(PATH);

  if (!root.ok) {
    fprintf(stderr, "toml_parse_file_ex: %s\n", root.errmsg);
    toml_free(root);
    exit(-1);
  }

  toml_datum_t wayland_displays =
      toml_seek(root.toptab, "main.wayland_displays");
  toml_datum_t clipboards = toml_seek(root.toptab, "main.clipboards");
  (void)

      toml_free(root);
}

int main() {
  setup();
  run();
  return 0;
}
