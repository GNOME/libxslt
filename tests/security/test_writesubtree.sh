#!/bin/sh
#
# Tests for the writesubtree path validation fix (commit f0ce3703).
#
# The fix ensures that --writesubtree /some/path does not allow writes
# to sibling paths like /some/path-evil that merely share the same prefix.
# After the fix, the character immediately following the subtree prefix
# must be '/', '\0' (exact match), or a space.
#
# Exit status: 0 if all tests pass, 1 if any test fails.

set -e

XSLTPROC="${XSLTPROC:-../../xsltproc/xsltproc}"
TESTDIR="$(cd "$(dirname "$0")" && pwd)"
TMPDIR_BASE="$(mktemp -d)"
FAILURES=0
TESTS=0

cleanup() {
    rm -rf "$TMPDIR_BASE"
}
trap cleanup EXIT

pass() {
    TESTS=$((TESTS + 1))
    echo "  PASS: $1"
}

fail() {
    TESTS=$((TESTS + 1))
    FAILURES=$((FAILURES + 1))
    echo "  FAIL: $1"
}

# Create the directory structure for testing.
# We use subtree=$TMPDIR_BASE/safe and try to write to various paths.
SUBTREE="$TMPDIR_BASE/safe"
SIBLING="$TMPDIR_BASE/safe-evil"
mkdir -p "$SUBTREE" "$SIBLING"

echo "=== writesubtree validation tests ==="

# -----------------------------------------------------------------------
# Test 1: Write within the subtree (subdirectory) should succeed.
# -----------------------------------------------------------------------
OUTPUT="$SUBTREE/output.xml"
rm -f "$OUTPUT"
"$XSLTPROC" --writesubtree "$SUBTREE" \
    --param output "'$OUTPUT'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$OUTPUT" ]; then
    pass "write to subtree subdirectory allowed"
else
    fail "write to subtree subdirectory was blocked (should be allowed)"
fi

# -----------------------------------------------------------------------
# Test 2: Write to sibling path sharing prefix should be REJECTED.
# This is the core vulnerability the fix addresses.
# e.g., subtree=/tmp/safe, path=/tmp/safe-evil/output.xml
# -----------------------------------------------------------------------
OUTPUT="$SIBLING/output.xml"
rm -f "$OUTPUT"
"$XSLTPROC" --writesubtree "$SUBTREE" \
    --param output "'$OUTPUT'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$OUTPUT" ]; then
    fail "write to sibling path was allowed (SECURITY: prefix bypass)"
else
    pass "write to sibling path correctly rejected"
fi

# -----------------------------------------------------------------------
# Test 3: Exact match of subtree path should succeed.
# -----------------------------------------------------------------------
SUBTREE_FILE="$TMPDIR_BASE/safefile"
rm -f "$SUBTREE_FILE"
"$XSLTPROC" --writesubtree "$SUBTREE_FILE" \
    --param output "'$SUBTREE_FILE'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$SUBTREE_FILE" ]; then
    pass "write to exact subtree path allowed"
else
    fail "write to exact subtree path was blocked (should be allowed)"
fi

# -----------------------------------------------------------------------
# Test 4: Subtree with trailing slash should allow writes within it.
# -----------------------------------------------------------------------
OUTPUT="$SUBTREE/subdir_output.xml"
rm -f "$OUTPUT"
"$XSLTPROC" --writesubtree "$SUBTREE/" \
    --param output "'$OUTPUT'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$OUTPUT" ]; then
    pass "write within subtree (trailing slash) allowed"
else
    fail "write within subtree (trailing slash) was blocked (should be allowed)"
fi

# -----------------------------------------------------------------------
# Test 5: Completely unrelated path should be rejected.
# -----------------------------------------------------------------------
UNRELATED="$TMPDIR_BASE/other"
mkdir -p "$UNRELATED"
OUTPUT="$UNRELATED/output.xml"
rm -f "$OUTPUT"
"$XSLTPROC" --writesubtree "$SUBTREE" \
    --param output "'$OUTPUT'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$OUTPUT" ]; then
    fail "write to unrelated path was allowed"
else
    pass "write to unrelated path correctly rejected"
fi

# -----------------------------------------------------------------------
# Test 6: Sibling with single extra character should be rejected.
# e.g., subtree=/tmp/safe, path=/tmp/safex
# -----------------------------------------------------------------------
SIBLING2="$TMPDIR_BASE/safex"
rm -f "$SIBLING2"
"$XSLTPROC" --writesubtree "$SUBTREE" \
    --param output "'$SIBLING2'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$SIBLING2" ]; then
    fail "write to sibling (single char suffix) was allowed (SECURITY: prefix bypass)"
else
    pass "write to sibling (single char suffix) correctly rejected"
fi

# -----------------------------------------------------------------------
# Test 7: Nested subdirectory write should succeed.
# -----------------------------------------------------------------------
NESTED="$SUBTREE/a/b/c"
mkdir -p "$NESTED"
OUTPUT="$NESTED/output.xml"
rm -f "$OUTPUT"
"$XSLTPROC" --writesubtree "$SUBTREE" \
    --param output "'$OUTPUT'" \
    "$TESTDIR/write_output.xsl" "$TESTDIR/input.xml" > /dev/null 2>&1 || true
if [ -f "$OUTPUT" ]; then
    pass "write to nested subdirectory allowed"
else
    fail "write to nested subdirectory was blocked (should be allowed)"
fi

# -----------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------
echo ""
echo "Results: $((TESTS - FAILURES))/$TESTS passed"
if [ "$FAILURES" -gt 0 ]; then
    echo "FAILED"
    exit 1
fi
echo "OK"
exit 0
