#!/usr/bin/env bash
#
# Run tomlc99's toml_cat, built against the compat layer, over
# tomlc99's stdex/ cases and compare with the expected *.res output.
#
# - Where *.res records an error, only the rejection is checked: the
#   error text comes from tomlc17 and differs from tomlc99's.
# - "-nan" is printed as "nan" on some platforms; normalize it.
# - stdex/string8.toml.res was corrected: tomlc99 wrongly rejects that
#   valid basic string; tomlc17 accepts it.

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR/stdex" || exit 1

fail=0
for i in *.toml; do
  out=$("$DIR/toml_cat" "$i" 2>&1 | sed 's/-nan/nan/')
  if grep -q '^ERROR:' "$i.res"; then
    if grep -q '^ERROR:' <<<"$out"; then
      continue
    fi
    echo "$i [FAILED]: expected an error"
  elif [ "$out" == "$(sed 's/-nan/nan/' "$i.res")" ]; then
    continue
  else
    echo "$i [FAILED]"
    diff <(echo "$out") "$i.res"
  fi
  fail=1
done

if [ $fail == 0 ]; then
  echo "stdex: all passed"
fi
exit $fail
