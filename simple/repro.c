#include "../src/tomlc17.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void error(const char *msg, const char *msg1) {
  fprintf(stderr, "ERROR: %s%s\n", msg, msg1 ? msg1 : "");
  exit(1);
}

const char *PATH = "/tmp/t.toml";

static void setup() {
  const char *text =
      "# Configuration file\n"
      "\n"
      "[main]\n"
      "wayland_displays = [ \"$WAYLAND_DISPLAY\" ]\n"
      "clipboards = [ \"Default\" ]\n"
      "\n"
      "[default]\n"
      "connection_timeout = 500\n"
      "data_timeout = 500\n"
      "\n"
      "max_entries = 100\n"
      "max_entries_memory = 10\n"
      "\n"
      "[wayland_displays.\"$WAYLAND_DISPLAY\"]\n"
      "connection_timeout = 500\n"
      "data_timeout = 500\n"
      "seats = [ \"$XDG_SEAT\" ] \n"
      "\n"
      "[wayland_displays.\"$WAYLAND_DISPLAY\".\"$XDG_SEAT\"]\n"
      "clipboard = \"Default\"\n"
      "regular = true\n"
      "primary = false\n"
      "\n"
      "[clipboards.Default]\n"
      "max_entries = 10\n"
      "max_entries_memory = 5\n"
      "allowed_mime_types = [ \"text/*\", \"image/*\" ]\n"
      "\n"
      "[[clipboards.Default.mime_type_groups]]\n"
      "mime_type = \"text/plain;charset=utf-8\"\n"
      "group = [ \"TEXT\", \"STRING\", \"UTF8_STRING\", \"text/plain\" ]\n";

  FILE *fp = fopen(PATH, "w");
  fprintf(fp, "%s", text);
  fclose(fp);
}

static void run() {

  toml_result_t root = toml_parse_file_ex(PATH);

  if (!root.ok) {
    error("toml_parse_file_ex failed", 0);
  }

  toml_datum_t wayland_displays =
      toml_seek(root.toptab, "main.wayland_displays");
  toml_datum_t clipboards = toml_seek(root.toptab, "main.clipboards");

  toml_free(root);
}

int main() {
  setup();
  run();
  return 0;
}
