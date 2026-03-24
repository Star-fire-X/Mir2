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

grep -Fq '.worktrees' "${STATIC_CHECKS}"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

TOOL_BIN="${TMP_DIR}/bin"
mkdir -p "${TOOL_BIN}"

for tool in cmake cppcheck clang-format cpplint; do
  cat <<'EOF' > "${TOOL_BIN}/${tool}"
#!/usr/bin/env bash
set -euo pipefail
log_name="$(basename "$0")"
printf '%s\n' "$*" >> "${TOOL_LOG_DIR}/${log_name}.log"
EOF
  chmod +x "${TOOL_BIN}/${tool}"
done

run_static_checks_fixture() {
  local repo_name="$1"
  local case_name="$2"
  local test_repo="${TMP_DIR}/${repo_name}/.worktrees/active"
  local tool_log_dir="${TMP_DIR}/tool-logs-${case_name}"

  mkdir -p "${test_repo}/scripts/ci" \
    "${test_repo}/src" \
    "${test_repo}/.worktrees/other/src" \
    "${tool_log_dir}"

  cp "${STATIC_CHECKS}" "${test_repo}/scripts/ci/run_static_checks.sh"
  chmod +x "${test_repo}/scripts/ci/run_static_checks.sh"

  printf 'cmake_minimum_required(VERSION 3.20)\nproject(static_checks_fixture LANGUAGES CXX)\n' \
    > "${test_repo}/CMakeLists.txt"
  printf 'int kept = 0;\n' > "${test_repo}/src/kept.cc"
  printf 'int ignored = 0;\n' > "${test_repo}/.worktrees/other/src/ignored.cc"

  PATH="${TOOL_BIN}:$PATH" TOOL_LOG_DIR="${tool_log_dir}" \
    bash "${test_repo}/scripts/ci/run_static_checks.sh"

  if [[ ! -f "${tool_log_dir}/clang-format.log" ]]; then
    echo "clang-format was not invoked for case ${case_name}"
    exit 1
  fi

  grep -Fq "${test_repo}/src/kept.cc" "${tool_log_dir}/clang-format.log"

  if grep -Fq "${test_repo}/.worktrees/other/src/ignored.cc" \
    "${tool_log_dir}/clang-format.log"; then
    echo "nested worktree sources should be excluded for case ${case_name}"
    exit 1
  fi
}

run_static_checks_fixture "repo" "plain"
run_static_checks_fixture "repo[1]" "glob"
