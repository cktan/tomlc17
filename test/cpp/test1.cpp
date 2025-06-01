#include "../../src/tomlcpp.hpp"
#include <cstring>
#include <iostream>

static void failed() {
  printf("FAILED\n");
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed()

using namespace toml;
using namespace std::chrono;
using std::cout;
using std::endl;

static void test_string() {
  printf("test string ...\n");
  const char* doc = "title = \"Moby Dick\"";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("title")).as_str();
  CHECK(value == "Moby Dick");
}

static void test_int() {
  printf("test int ...\n");
  const char* doc = "count = 20";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("count")).as_int();
  CHECK(value == 20);
}
static void test_float() {
  printf("test float ...\n");
  const char* doc = "speed = 20.5";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("speed")).as_real();
  CHECK(value == 20.5);
}
static void test_boolean() {
  printf("test boolean ...\n");
  const char* doc = "always = true";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("always")).as_bool();
  CHECK(value == true);
}
static void test_date() {
  printf("test date ...\n");
  const char* doc = "christmas = 2025-12-25";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("christmas")).as_date();
  CHECK(value == December/25/2025);
}
static void test_time() {
  printf("test time ...\n");
  const char* doc = "noon = 12:00:00";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("noon")).as_time();
  CHECK(value.hours() == 12h && value.minutes() == 0min && value.seconds()== 0s);
}
static void test_datetime() {
  printf("test datetime ...\n");
  const char* doc = "now = 2025-06-01 11:15:00.123";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("now")).as_datetime();
  auto ymd = year_month_day{floor<days>(value)};
  CHECK(ymd.year() == year{2025} && ymd.month() == month{6} && ymd.day() == day{1});
  auto tod = hh_mm_ss<microseconds>{value - sys_days{ymd}};
  CHECK(tod.hours() == hours(11) && tod.minutes() == minutes(15) && tod.seconds() == seconds(0) && tod.subseconds() == microseconds(123000));
}
static void test_datetimetz() {
  printf("test datetimetz ...\n");
  const char* doc = "now = 2025-06-01 11:15:00.123-05:00";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto valuetz = *(*toptab.get("now")).as_datetimetz();
  auto [value, tzoff] = valuetz;
  auto ymd = year_month_day{floor<days>(value)};
  CHECK(ymd.year() == year{2025} && ymd.month() == month{6} && ymd.day() == day{1});
  auto tod = hh_mm_ss<microseconds>{value - sys_days{ymd}};
  CHECK(tod.hours() == hours(11) && tod.minutes() == minutes(15) && tod.seconds() == seconds(0) && tod.subseconds() == microseconds(123000));

  CHECK(tzoff == -5 * 60);
}
static void test_array() {
  printf("test array ...\n");
  const char* doc = "array = [1,2,3]";
  auto result = toml_parse(doc, strlen(doc));
  CHECK(result.ok);
  Datum toptab{result.toptab};
  auto value = *(*toptab.get("array")).as_intvec();
  CHECK(value == (std::vector<int64_t>{1,2,3}));
}

int main() {
  test_string();
  test_int();
  test_float();
  test_boolean();
  test_date();
  test_time();
  test_datetime();
  test_datetimetz();
  test_array();
  return 0;
}
