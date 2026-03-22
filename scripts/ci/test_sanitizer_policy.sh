#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="${ROOT_DIR}/scripts/ci/run_sanitizers.sh"

grep -q -- '-fno-sanitize-recover=all' "${SCRIPT}"
grep -q 'CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST' "${SCRIPT}"
grep -q 'detect_leaks_default=1' "${SCRIPT}"
grep -q 'detect_leaks_default=0' "${SCRIPT}"
grep -q 'find_test_probe_binary' "${SCRIPT}"
grep -q 'probe_leak_detection_compatibility' "${SCRIPT}"
grep -q -- '--gtest_list_tests' "${SCRIPT}"
grep -q 'LeakSanitizer does not work under ptrace' "${SCRIPT}"
grep -q 'halt_on_error=1' "${SCRIPT}"
grep -q 'log_path=' "${SCRIPT}"
grep -q 'print_stacktrace=1' "${SCRIPT}"
grep -q 'LOG_DIR="${PWD}/' "${SCRIPT}"
