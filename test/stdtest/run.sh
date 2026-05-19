
echo
echo =========================
echo == stdtest
echo =========================

#v1.0 ==> go install github.com/toml-lang/toml-test/cmd/toml-test@latest
go install github.com/toml-lang/toml-test/v2/cmd/toml-test@latest
toml-test test -toml 1.1 -decoder $PWD/driver \
    -skip invalid/key/special-character  # μ (U+03BC) is valid per TOML 1.1 ABNF; toml-test v2 doesn't exclude this 1.0-era test for 1.1
