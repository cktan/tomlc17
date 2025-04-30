#!/bin/bash
F=parser
rm -f $F
[ -f ../../src/$F ] && ln -s ../../src/$F || { echo "$F not found in ../../src"; exit 1; }

mkdir -p out

for fname in {1..100} array{1..10} tab{1..10} x{1..10}; do
    IN="in/$fname.toml"
    if [ -f $IN ]; then
        echo test $fname.toml
	OUT="out/$fname.out"
	GOOD="good/$fname.out"
        ./$F $IN &> $OUT
        diff $GOOD $OUT || { echo '--- FAILED ---'; exit 1; }
    fi
done

echo DONE
