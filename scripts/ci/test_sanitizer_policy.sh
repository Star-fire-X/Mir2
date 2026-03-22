#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="${ROOT_DIR}/scripts/ci/run_sanitizers.sh"

grep -q -- '-fno-sanitize-recover=all' "${SCRIPT}"
grep -q 'detect_leaks=1' "${SCRIPT}"
grep -q 'halt_on_error=1' "${SCRIPT}"
grep -q 'log_path=' "${SCRIPT}"
grep -q 'print_stacktrace=1' "${SCRIPT}"
