#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORKFLOW="${ROOT_DIR}/.github/workflows/ci.yml"
STATIC_CHECKS="${ROOT_DIR}/scripts/ci/run_static_checks.sh"
CLANG_TIDY_SCRIPT="${ROOT_DIR}/scripts/ci/run_clang_tidy.sh"
SANITIZER_SCRIPT="${ROOT_DIR}/scripts/ci/run_sanitizers.sh"

test -f "${CLANG_TIDY_SCRIPT}"
test -f "${SANITIZER_SCRIPT}"

grep -q '^  clang_tidy:$' "${WORKFLOW}"
grep -q '^  sanitizers:$' "${WORKFLOW}"
grep -q 'run: bash scripts/ci/run_clang_tidy.sh' "${WORKFLOW}"
grep -q 'run: bash scripts/ci/run_sanitizers.sh' "${WORKFLOW}"

grep -q 'clang-tidy' "${CLANG_TIDY_SCRIPT}"
grep -q -- '-fsanitize=address,undefined' "${SANITIZER_SCRIPT}"

if grep -q 'clang-tidy' "${STATIC_CHECKS}"; then
  echo "run_static_checks.sh still invokes clang-tidy"
  exit 1
fi

grep -Fq -- '-not -path "*/.worktrees/*"' "${STATIC_CHECKS}"
