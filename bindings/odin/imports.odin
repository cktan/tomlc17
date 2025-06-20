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
