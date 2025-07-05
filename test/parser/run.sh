#!/bin/bash
mkdir -p out

echo
echo =========================
echo == parser test
echo =========================

for fname in {1..200} array{1..10} tab{1..10} x{1..10} e{1..10}; do
    IN="in/$fname.toml"
    if [ -f $IN ]; then
        echo test $fname.toml
	OUT="out/$fname.out"
	GOOD="good/$fname.out"
        ./driver $IN &> $OUT || true    # ignore failure
        diff $GOOD $OUT || { echo '--- FAILED ---'; exit 1; }
    fi
done

echo DONE
