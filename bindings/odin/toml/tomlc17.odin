/* Copyright (c) 2024-2025, CK Tan.
* https://github.com/cktan/tomlc17/blob/main/LICENSE
*/
package toml

import "core:c"

_ :: c

when ODIN_OS == .Windows {
	foreign import lib "windows/toml.lib"
} else when ODIN_OS == .Linux {
	foreign import lib "linux/toml.a"
} else when ODIN_OS == .Darwin {
	when ODIN_ARCH == .arm64 {
		foreign import lib "macos-arm64/toml.a"
	} else {
		foreign import lib "macos/toml.a"
	}
}


// TOML_EXTERN :: extern

type_t :: enum c.int {
	UNKNOWN = 0,
	STRING,
	INT64,
	FP64,
	BOOLEAN,
	DATE,
	TIME,
	DATETIME,
	DATETIMETZ,
	ARRAY,
	TABLE,
}

datum_t :: struct {
	type: type_t,
	flag: u32, // internal
	u:    struct #raw_union {
		s:       cstring, // same as str.ptr; use if there are no NUL in string.
		str:     struct {
			ptr: cstring, // NUL terminated string
			len: c.int,   // length excluding the terminating NUL.
		},
		int64:   i64,     // integer
		fp64:    f64,     // float
		boolean: bool,
		ts:      struct {
			year, month, day:     i16,
			hour, minute, second: i16,
			usec:                 i32,
			tz:                   i16, // in minutes
		},
		arr:     struct {
			size: i32, // count elem
			elem: ^datum_t,
		},
		tab:     struct {
			size:  i32, // count key
			key:   ^^c.char,
			len:   ^c.int,
			value: ^datum_t,
		},
	},
}

result_t :: struct {
	ok:         bool,        // success flag
	toptab:     datum_t,     // valid if ok
	errmsg:     [200]c.char, // valid if not ok
	__internal: rawptr,      // do not use
}

option_t :: struct {
	check_utf8:  bool,                                  // Check all chars are valid utf8; default: false.
	mem_realloc: proc "c" (rawptr, c.size_t) -> rawptr, // default: realloc()
	mem_free:    proc "c" (rawptr),                     // default: free()
}

@(default_calling_convention="c", link_prefix="toml_")
foreign lib {
	/**
	* Parse a toml document. Returns a toml_result which must be freed
	* using toml_free() eventually.
	*
	* IMPORTANT: src[] must be a NUL terminated string! The len parameter
	* does not include the NUL terminator.
	*/
	parse :: proc(src: cstring, len: c.int) -> result_t ---

	/**
	* Parse a toml file. Returns a toml_result which must be freed
	* using toml_free() eventually.
	*/
	parse_file :: proc(file: ^c.FILE) -> result_t ---

	/**
	* Parse a toml file. Returns a toml_result which must be freed
	* using toml_free() eventually.
	*/
	parse_file_ex :: proc(fname: cstring) -> result_t ---

	/**
	* Release the result.
	*/
	free :: proc(result: result_t) ---

	/**
	* Find a key in a toml_table. Return the value of the key if found,
	* or a TOML_UNKNOWN otherwise.
	*/
	get :: proc(table: datum_t, key: cstring) -> datum_t ---

	/**
	* Locate a value starting from a toml_table. Return the value of the key if
	* found, or a TOML_UNKNOWN otherwise.
	*
	* Note: the multipart-key is separated by DOT, and must not have any escape
	* chars.
	*/
	seek :: proc(table: datum_t, multipart_key: cstring) -> datum_t ---

	/**
	* OBSOLETE: use toml_get() instead.
	* Find a key in a toml_table. Return the value of the key if found,
	* or a TOML_UNKNOWN otherwise. (
	*/
	table_find :: proc(table: datum_t, key: cstring) -> datum_t ---

	/**
	*  Override values in r1 using r2. Return a new result. All results
	*  (i.e., r1, r2 and the returned result) must be freed using toml_free()
	*  after use.
	*
	*  LOGIC:
	*   ret = copy of r1
	*   for each item x in r2:
	*     if x is not in ret:
	*          override
	*     elif x in ret is NOT of the same type:
	*         override
	*     elif x is an array of tables:
	*         append r2.x to ret.x
	*     elif x is a table:
	*         merge r2.x to ret.x
	*     else:
	*         override
	*/
	merge :: proc(r1: ^result_t, r2: ^result_t) -> result_t ---

	/**
	*  Check if two results are the same. Dictinary and array orders are
	*  sensitive.
	*/
	equiv :: proc(r1: ^result_t, r2: ^result_t) -> bool ---

	/**
	* Get the default options. IF NECESSARY, use this to initialize
	* toml_option_t and override values before calling
	* toml_set_option().
	*/
	default_option :: proc() -> option_t ---

	/**
	* Set toml options globally. Do this ONLY IF you are not satisfied with the
	* defaults.
	*/
	set_option :: proc(opt: option_t) ---
}
