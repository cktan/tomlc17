go install github.com/toml-lang/toml-test/cmd/toml-test@latest
rm -f parser
ln -s ../../src/parser .
toml-test $PWD/parser
