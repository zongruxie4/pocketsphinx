#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"

# The shipped en-us model has an inconsistent header (it claims 2051547
# bigrams but stores 2051541), which exercises the count-mismatch path in
# the ARPA writer.
lm=$model/en-us/en-us.lm.bin

# The writer warns about the mismatch, does not crash, and writes the
# actual counts found in the trie.
rm -f $bn.arpa
run_program pocketsphinx_lm_convert \
            -i $lm \
            -o $bn.arpa \
            > $bn.log 2>&1
status=$?
if [ $status != 0 ]; then
    fail "convert (exit $status)"
elif [ $status -ge 128 ]; then
    fail "convert (crashed, status $status)"
elif ! grep -q "does not match" $bn.log; then
    fail "convert (no warning)"
else
    pass "convert"
fi

# The written model must be internally consistent: the \data\ counts must
# equal the number of n-gram lines in each section, and the bigram count
# must reflect the true trie content (2051541).
awk '
    /^ngram [0-9]+=[0-9]+$/ { split($2, a, "="); header[a[1] + 0] = a[2] + 0; next }
    /^\\[0-9]+-grams:$/     { sub(/^\\/, "", $1); sub(/-grams:$/, "", $1); sec = $1 + 0; next }
    /^\\end\\$/             { sec = 0; next }
    (sec > 0 && $0 !~ /^\\/ && NF > 0) { actual[sec]++ }
    END {
        ok = 1
        for (o in header)
            if (header[o] != actual[o]) ok = 0
        printf "bigrams header=%d actual=%d\n", header[2], actual[2]
        if (ok && header[2] == 2051541 && actual[2] == 2051541)
            exit 0
        exit 1
    }
' $bn.arpa
if [ $? = 0 ]; then
    pass "output consistent"
else
    fail "output consistent"
fi

rm -f $bn.arpa
