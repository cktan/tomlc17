#!/bin/bash

cp ../../src/tomlc17.c toml.c
cp ../../src/tomlc17.h tomlc17.h

# Configuration
CFLAGS='-O3 -std=c17'

mkdir -p toml/macos toml/macos-arm64 toml/linux toml/windows

echo 'Intel Mac'
clang -c -target x86_64-apple-darwin toml.c -fPIC $CFLAGS -o toml-x86_64.o
ar r toml/macos/toml.a toml-x86_64.o

echo 'ARM Mac'
clang -c toml.c -fPIC $CFLAGS -o toml-arm64.o
ar r toml/macos-arm64/toml.a toml-arm64.o

echo 'Linux'
clang -c -target x86_64-unknown-linux-gnu toml.c -fPIC $CFLAGS -o toml-linux.o
ar r toml/linux/toml.a toml-linux.o

echo 'x64 Windows'
clang -o toml/windows/toml.lib -target x86_64-pc-windows-msvc -fuse-ld=llvm-lib -static -O3 toml.c

# Cleanup
rm -f toml-*.o toml.c tomlc17.h

echo "Build complete!"
