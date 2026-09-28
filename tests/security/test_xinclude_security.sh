#!/bin/sh
#
# Tests for the xinclude security fix (commit 466bf82f).
#
# The fix ensures that when security preferences are active, XInclude
# processing is disabled in xsltLoadDocument(). This prevents xincluded
# documents from bypassing the read-permission checks that are applied
# to the root document.
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

echo "=== xinclude security tests ==="

# -----------------------------------------------------------------------
# Test 1: --xinclude with --nowrite (activates security prefs) should
#          reject document() loads that contain xi:include.
# -----------------------------------------------------------------------
STDERR="$TMPDIR_BASE/test1.err"
OUTPUT=$("$XSLTPROC" --xinclude --nowrite \
    "$TESTDIR/load_document.xsl" "$TESTDIR/input.xml" 2>"$STDERR") || true
if grep -q "XInclude disabled" "$STDERR"; then
    pass "--xinclude + --nowrite: XInclude correctly rejected"
else
    fail "--xinclude + --nowrite: expected XInclude rejection error"
    echo "    stderr was: $(cat "$STDERR")"
fi

# Also verify that the secret content did NOT appear in the output.
if echo "$OUTPUT" | grep -q "should-not-be-readable"; then
    fail "--xinclude + --nowrite: secret content leaked into output (SECURITY)"
else
    pass "--xinclude + --nowrite: secret content not in output"
fi

# -----------------------------------------------------------------------
# Test 2: --xinclude with --writesubtree (activates security prefs)
#          should also reject xi:include in loaded documents.
# -----------------------------------------------------------------------
STDERR="$TMPDIR_BASE/test2.err"
OUTPUT=$("$XSLTPROC" --xinclude --writesubtree "$TMPDIR_BASE" \
    "$TESTDIR/load_document.xsl" "$TESTDIR/input.xml" 2>"$STDERR") || true
if grep -q "XInclude disabled" "$STDERR"; then
    pass "--xinclude + --writesubtree: XInclude correctly rejected"
else
    fail "--xinclude + --writesubtree: expected XInclude rejection error"
    echo "    stderr was: $(cat "$STDERR")"
fi

# -----------------------------------------------------------------------
# Test 3: --xinclude alone should also reject (xsltproc always allocates
#          a security prefs object, so ctxt->sec is always non-NULL).
# -----------------------------------------------------------------------
STDERR="$TMPDIR_BASE/test3.err"
OUTPUT=$("$XSLTPROC" --xinclude \
    "$TESTDIR/load_document.xsl" "$TESTDIR/input.xml" 2>"$STDERR") || true
if grep -q "XInclude disabled" "$STDERR"; then
    pass "--xinclude alone: XInclude rejected (sec always allocated)"
else
    fail "--xinclude alone: expected XInclude rejection (sec is always non-NULL in xsltproc)"
    echo "    stderr was: $(cat "$STDERR")"
fi

# -----------------------------------------------------------------------
# Test 4: Without --xinclude, document() should load normally even with
#          security prefs. The xi:include elements are left as-is.
# -----------------------------------------------------------------------
STDERR="$TMPDIR_BASE/test4.err"
OUTPUT=$("$XSLTPROC" --nowrite \
    "$TESTDIR/load_document.xsl" "$TESTDIR/input.xml" 2>"$STDERR") || true
if grep -q "XInclude disabled" "$STDERR"; then
    fail "without --xinclude: XInclude rejection should not occur"
else
    pass "without --xinclude: document() loads without XInclude error"
fi

# -----------------------------------------------------------------------
# Test 5: Verify that the error message contains the URI for debugging.
# -----------------------------------------------------------------------
STDERR="$TMPDIR_BASE/test5.err"
"$XSLTPROC" --xinclude --nowrite \
    "$TESTDIR/load_document.xsl" "$TESTDIR/input.xml" 2>"$STDERR" || true
if grep -q "xinclude_doc.xml" "$STDERR"; then
    pass "error message includes the document URI"
else
    fail "error message should include the document URI for debugging"
    echo "    stderr was: $(cat "$STDERR")"
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
