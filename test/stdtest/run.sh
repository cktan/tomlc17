
echo
echo =========================
echo == stdtest
echo =========================

#v1.0 ==> go install github.com/toml-lang/toml-test/cmd/toml-test@latest
go install github.com/toml-lang/toml-test/v2/cmd/toml-test@latest
toml-test test -toml 1.1 -decoder $PWD/driver
