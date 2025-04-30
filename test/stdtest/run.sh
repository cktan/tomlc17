set -e
make
go install github.com/toml-lang/toml-test/cmd/toml-test@latest
toml-test $PWD/driver
