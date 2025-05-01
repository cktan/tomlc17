#!/bin/bash
mkdir -p out

echo
echo =========================
echo == scankey test
echo =========================

for fname in {1..100}; do
    IN="in/$fname"
    if [ -f $IN ]; then
        echo test $fname
	OUT="out/$fname.out"
	GOOD="good/$fname.out"
        ./driver $IN &> $OUT
        diff $GOOD $OUT || { echo '--- FAILED ---'; exit 1; }
    fi
done

echo DONE
