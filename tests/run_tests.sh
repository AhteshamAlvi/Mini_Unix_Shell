#!/usr/bin/env bash
#
# Test suite for d8sh — Mini Unix Shell
# Usage: bash tests/run_tests.sh
#

set -euo pipefail

SHELL_BIN="./d8sh"
PASS=0
FAIL=0
TMPDIR=$(mktemp -d)

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

# ── helpers ──────────────────────────────────────────────────────────

run_shell() {
    # Feed commands via stdin, capture stdout and stderr separately
    printf '%s\n' "$1" | "$SHELL_BIN" 2>"$TMPDIR/stderr"
}

# Strip the prompt ("d8sh> ") from output and remove empty lines
strip_prompt() {
    sed 's/d8sh> //g' | sed '/^$/d'
}

assert_output() {
    local desc="$1" input="$2" expected="$3"
    local actual
    actual=$(run_shell "$input" | strip_prompt)

    if [ "$actual" = "$expected" ]; then
        PASS=$((PASS + 1))
        printf "  PASS: %s\n" "$desc"
    else
        FAIL=$((FAIL + 1))
        printf "  FAIL: %s\n" "$desc"
        printf "    input:    %s\n" "$input"
        printf "    expected: %s\n" "$expected"
        printf "    actual:   %s\n" "$actual"
    fi
}

assert_stderr_contains() {
    local desc="$1" input="$2" pattern="$3"
    run_shell "$input" >/dev/null 2>"$TMPDIR/stderr" || true
    if grep -q "$pattern" "$TMPDIR/stderr"; then
        PASS=$((PASS + 1))
        printf "  PASS: %s\n" "$desc"
    else
        FAIL=$((FAIL + 1))
        printf "  FAIL: %s\n" "$desc"
        printf "    expected stderr to contain: %s\n" "$pattern"
        printf "    stderr was: %s\n" "$(cat "$TMPDIR/stderr")"
    fi
}

# ── tests ────────────────────────────────────────────────────────────

echo "=== Simple Commands ==="
assert_output "echo hello" \
    "echo hello" \
    "hello"

assert_output "echo multiple args" \
    "echo hello world" \
    "hello world"

echo ""
echo "=== Semicolon ==="
assert_output "semicolon runs both commands" \
    "echo aaa; echo bbb" \
    "aaa
bbb"

assert_output "triple semicolon" \
    "echo 1; echo 2; echo 3" \
    "1
2
3"

echo ""
echo "=== AND operator ==="
assert_output "AND success path" \
    "true && echo yes" \
    "yes"

assert_output "AND failure path (no output)" \
    "false && echo no" \
    ""

echo ""
echo "=== OR operator ==="
assert_output "OR failure path" \
    "false || echo fallback" \
    "fallback"

assert_output "OR success path (no fallback)" \
    "true || echo no" \
    ""

echo ""
echo "=== Pipes ==="
assert_output "simple pipe" \
    "echo hello | cat" \
    "hello"

assert_output "pipe with grep" \
    "echo hello world | grep hello" \
    "hello world"

assert_output "multi-pipe" \
    "echo abc | cat | cat" \
    "abc"

echo ""
echo "=== Subshell ==="
assert_output "subshell basic" \
    "(echo sub)" \
    "sub"

assert_output "subshell with semicolon" \
    "(echo a; echo b)" \
    "a
b"

echo ""
echo "=== Output Redirection ==="
assert_output "redirect output to file" \
    "echo redir_test > $TMPDIR/out.txt" \
    ""

# Verify the file content
if [ "$(cat "$TMPDIR/out.txt")" = "redir_test" ]; then
    PASS=$((PASS + 1))
    printf "  PASS: output file has correct content\n"
else
    FAIL=$((FAIL + 1))
    printf "  FAIL: output file has wrong content: %s\n" "$(cat "$TMPDIR/out.txt")"
fi

echo ""
echo "=== Input Redirection ==="
echo "input_data" > "$TMPDIR/in.txt"
assert_output "redirect input from file" \
    "cat < $TMPDIR/in.txt" \
    "input_data"

echo ""
echo "=== Builtin: cd ==="
# Use /usr to avoid macOS symlink resolution issues
assert_output "cd then pwd" \
    "cd /usr && pwd" \
    "/usr"

echo ""
echo "=== Ambiguous Redirects ==="
assert_output "ambiguous output redirect" \
    "echo hi > $TMPDIR/x.txt | cat" \
    "Ambiguous output redirect."

assert_output "ambiguous input redirect" \
    "cat < $TMPDIR/in.txt | cat < $TMPDIR/in.txt" \
    "Ambiguous input redirect."

echo ""
echo "=== Syntax Errors ==="
assert_stderr_contains "unmatched paren" \
    "(echo hello" \
    "syntax error"

assert_stderr_contains "empty pipe rhs" \
    "echo hello |" \
    "syntax error"

echo ""
echo "=== Error Handling ==="
assert_stderr_contains "command not found" \
    "nonexistent_command_xyz" \
    "nonexistent_command_xyz"

echo ""
echo "=== Combined Operators ==="
assert_output "pipe into grep with AND" \
    "echo needle haystack | grep needle && echo found" \
    "needle haystack
found"

assert_output "OR with pipe" \
    "false || echo recovered | cat" \
    "recovered"

# ── summary ──────────────────────────────────────────────────────────

echo ""
echo "================================"
printf "Results: %d passed, %d failed\n" "$PASS" "$FAIL"
echo "================================"

[ "$FAIL" -eq 0 ] && exit 0 || exit 1
