package main

import "../bindings/odin/toml"

import "core:c"
import "core:fmt"
import "core:os"
import "core:slice"

error :: proc(msg: string) {
	fmt.eprintln(msg)
	os.exit(1)
}

main :: proc() {
	result := toml.parse_file_ex("simple.toml")
	defer toml.free(result)

	if !result.ok {
		error(string(result.errmsg[:]))
	}
	server := toml.get(result.toptab, "server")
	host := toml.get(server, "host")
	port := toml.get(server, "port")

	if host.type != .STRING {
		error("missing or invalid 'server.host' property in config")
	}
	fmt.println("server.host =", host.u.s)

	if (port.type != .ARRAY) {
		error("missing or invalid 'server.port' property in config")
	}
	fmt.print("server.port = [")
	elems := slice.from_ptr(port.u.arr.elem, cast(int)port.u.arr.size)
	for elem, i in elems {
		if (elem.type != .INT64) {
			error("server.port element not an integer")
		}
		fmt.print(i != 0 ? ", " : "", elem.u.int64)
	}
	fmt.println("]")
}
