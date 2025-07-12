#pragma once
#if !defined(__cplusplus) || __cplusplus < 202002L
#error "This code requires C++20 or later"
#endif

#include "tomlc17.h"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace toml {


class Datum : public toml_datum_t {
public:
  using datetime = std::chrono::sys_time<std::chrono::microseconds>;
  using time = std::chrono::hh_mm_ss<std::chrono::microseconds>;
  using date = std::chrono::year_month_day;

  Datum(toml_datum_t dat = toml_datum_t{}) : toml_datum_t(dat) {}
  bool is_table() const { return type == TOML_TABLE; }
  bool is_array() const { return type == TOML_ARRAY; }

  // Retrieve a string.
  std::optional<std::string_view> as_str() const {
    if (type != TOML_STRING)
      return std::nullopt;
    else
      return std::string_view{u.str.ptr, (size_t)u.str.len};
  }

  // Retrieve an int.
  std::optional<std::int64_t> as_int() const {
    if (type != TOML_INT64)
      return std::nullopt;
    else
      return u.int64;
  }

  // Retrieve a float.
  std::optional<double> as_real() const {
    if (type != TOML_FP64)
      return std::nullopt;
    else
      return u.fp64;
  }

  // Retrieve a boolean.
  std::optional<bool> as_bool() const {
    if (type != TOML_BOOLEAN)
      return std::nullopt;
    else
      return !!u.boolean;
  }

  // Retrieve a date.
  std::optional<date> as_date() const {
    using namespace std::chrono;
    if (type != TOML_DATE)
      return std::nullopt;
    else
      return date{year(u.ts.year), month(u.ts.month), day(u.ts.day)};
  }

  // Retrieve a time.
  std::optional<time> as_time() const {
    using namespace std::chrono;
    if (type != TOML_TIME)
      return std::nullopt;
    else
      return time{hours(u.ts.hour) + minutes(u.ts.minute) +
                  seconds(u.ts.second) + microseconds(u.ts.usec)};
  }

  // Retrieve a datetime.
  std::optional<datetime> as_datetime() const {
    using namespace std::chrono;
    if (type != TOML_DATETIME) {
      return std::nullopt;
    }
    year_month_day ymd{year{u.ts.year}, month{(unsigned)u.ts.month},
                       day{(unsigned)u.ts.day}};
    auto tod = hours{u.ts.hour} + minutes{u.ts.minute} + seconds{u.ts.second} +
               microseconds{u.ts.usec};
    return datetime{sys_days{ymd} + tod};
  }

  // Retrieve a datetime and minute offset from UTC.
  std::optional<std::pair<datetime, int>> as_datetimetz() const {
    using namespace std::chrono;

    if (type != TOML_DATETIMETZ) {
      return std::nullopt;
    }
    year_month_day ymd{year{u.ts.year}, month{(unsigned)u.ts.month},
                       day{(unsigned)u.ts.day}};
    auto tod = hours{u.ts.hour} + minutes{u.ts.minute} + seconds{u.ts.second} +
               microseconds{u.ts.usec};
    return std::pair{datetime{sys_days{ymd} + tod}, u.ts.tz};
  }

  // Retrieve an array of Datum from this datum.
  std::optional<std::vector<Datum>> as_vector() const {
    if (type != TOML_ARRAY) {
      return std::nullopt;
    }
    std::vector<Datum> ret;
    ret.assign(u.arr.elem, u.arr.elem + u.arr.size);
    return ret;
  }

  // Retrieve an array of strings from this datum.
  std::optional<std::vector<std::string_view>> as_strvec() const {
    try {
      auto vec = as_vector().value();
      std::vector<std::string_view> ret;
      ret.resize(vec.size());
      for (size_t i = 0; i < vec.size(); i++) {
        ret[i] = vec[i].as_str().value();
      }
      return ret;
    } catch (const std::bad_optional_access &ex) {
      return std::nullopt;
    }
  }

  // Retrieve an array of ints from this datum
  std::optional<std::vector<int64_t>> as_intvec() const {
    try {
      auto vec = as_vector().value();
      std::vector<int64_t> ret;
      ret.resize(vec.size());
      for (size_t i = 0; i < vec.size(); i++) {
        ret[i] = *vec[i].as_int();
      }
      return ret;
    } catch (const std::bad_optional_access &ex) {
      return std::nullopt;
    }
  }

  // For tables. Retrieve the value of a composite key from this datum
  // (which is a table).
  std::optional<Datum> get(std::initializer_list<std::string_view> keys) const {
    Datum tab = *this;
    Datum value;
    for (auto key : keys) {
      value = {};
      if (tab.type != TOML_TABLE) {
        return std::nullopt;
      }
      for (int i = 0; i < tab.u.tab.size; i++) {
        if (key ==
            std::string_view{tab.u.tab.key[i], (size_t)tab.u.tab.len[i]}) {
          value = tab.u.tab.value[i];
          break;
        }
      }
      tab = value;
    }
    return value;
  }

  // Retrieve the value of a key
  std::optional<Datum> get(std::string_view key) const { return get({key}); }

  // Retrieve the value of a multi-part key
  std::optional<Datum> seek(const char *multipart_key) const {
    auto ret = toml_seek(*this, multipart_key);
    if (!ret.type) {
      return std::nullopt;
    }
    return ret;
  }

}; // class Datum

class Result : private toml_result_t {
public:
  Result(const toml_result_t& result) : toml_result_t(result) {}
  ~Result() noexcept { toml_free(*this); }

  // Disallow copying
  Result(const Result&) = delete;
  Result& operator=(const Result&) = delete;
  
  // Allow moving
  Result(Result&& other) noexcept : toml_result_t() {
    swap(*this, other);
  }
  Result& operator=(Result&& other) noexcept {
    swap(*this, other);
    return *this;
  }

  // Access methods
  bool ok() const { return toml_result_t::ok; }
  Datum toptab() const { return toml_result_t::toptab; }
  const char* errmsg() const { return toml_result_t::errmsg; }

  // Shortcuts
  std::optional<Datum> get(std::initializer_list<std::string_view> keys) const {
    return toptab().get(keys);
  }
  std::optional<Datum> seek(const char *multipart_key) const {
    return toptab().seek(multipart_key);
  }

private:
  friend void swap(Result& a, Result& b) noexcept {
    std::swap(static_cast<toml_result_t&>(a), static_cast<toml_result_t&>(b));
  }
};

static inline Result parse_file(FILE* fp) {
  return Result{toml_parse_file(fp)};
}

static inline Result parse_file_ex(const char* fname) {
  return Result{toml_parse_file_ex(fname)};
}

static inline Result parse(const std::string& s) {
  return Result{toml_parse(s.data(), s.size())};
}



}; // namespace toml
