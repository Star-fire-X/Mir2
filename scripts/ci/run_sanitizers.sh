#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-sanitizers}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
LOG_DIR="${LOG_DIR:-${ROOT_DIR}/artifacts/sanitizers}"
SANITIZER_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -O1 -g"
GOOGLETEST_SOURCE_DIR="${GOOGLETEST_SOURCE_DIR:-}"

CMAKE_ARGS=()
if [[ -n "${GOOGLETEST_SOURCE_DIR}" ]]; then
  CMAKE_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${GOOGLETEST_SOURCE_DIR}")
fi
CMAKE_ARGS+=("-DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST")

normalize_log_dir() {
  if [[ "${LOG_DIR}" != /* ]]; then
    LOG_DIR="${PWD}/${LOG_DIR}"
  fi
}

find_test_probe_binary() {
  find "${BUILD_DIR}" -type f -perm -111 \
    -not -path '*/_deps/*' \
    -not -path '*/CMakeFiles/*' \
    \( -name '*test' -o -name '*_test' -o -name '*tests' -o -name '*_tests' \) \
    | sort | head -n 1
}

probe_logs_indicate_ptrace() {
  shopt -s nullglob
  local probe_logs=(
    "${LOG_DIR}/probe.stderr"
    "${LOG_DIR}"/probe-asan*
    "${LOG_DIR}"/probe-ubsan*
    "${LOG_DIR}"/probe-lsan*
  )

  local log_file
  for log_file in "${probe_logs[@]}"; do
    if [[ -f "${log_file}" ]] &&
       grep -q 'LeakSanitizer does not work under ptrace' "${log_file}" 2>/dev/null; then
      return 0
    fi

    if [[ -f "${log_file}" ]] &&
       grep -q 'LeakSanitizer has encountered a fatal error' "${log_file}" 2>/dev/null &&
       grep -qi 'ptrace' "${log_file}" 2>/dev/null; then
      return 0
    fi
  done

  return 1
}

probe_leak_detection_compatibility() {
  local probe_bin
  probe_bin="$(find_test_probe_binary)"
  if [[ -z "${probe_bin}" ]]; then
    echo "::notice::No test probe binary found. Keeping leak detection enabled."
    return 0
  fi

  local probe_stdout="${LOG_DIR}/probe.stdout"
  local probe_stderr="${LOG_DIR}/probe.stderr"
  rm -f "${probe_stdout}" "${probe_stderr}" "${LOG_DIR}"/probe-asan* "${LOG_DIR}"/probe-ubsan* "${LOG_DIR}"/probe-lsan*

  if ASAN_OPTIONS="detect_leaks=1:strict_string_checks=1:halt_on_error=1:log_path=${LOG_DIR}/probe-asan" \
     UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:log_path=${LOG_DIR}/probe-ubsan" \
     LSAN_OPTIONS="exitcode=23:log_path=${LOG_DIR}/probe-lsan" \
     "${probe_bin}" --gtest_list_tests >"${probe_stdout}" 2>"${probe_stderr}"; then
    rm -f "${probe_stdout}" "${probe_stderr}" "${LOG_DIR}"/probe-asan* "${LOG_DIR}"/probe-ubsan* "${LOG_DIR}"/probe-lsan*
    return 0
  fi

  if probe_logs_indicate_ptrace; then
    echo "::notice::Ptrace detected on test execution. Disabling LeakSanitizer defaults for this run."
    rm -f "${probe_stdout}" "${probe_stderr}" "${LOG_DIR}"/probe-asan* "${LOG_DIR}"/probe-ubsan* "${LOG_DIR}"/probe-lsan*
    return 1
  fi

  echo "::error::Sanitizer compatibility probe failed unexpectedly."
  if [[ -s "${probe_stdout}" ]]; then
    printf '\n--- %s ---\n' "${probe_stdout}"
    cat "${probe_stdout}"
  fi
  if [[ -s "${probe_stderr}" ]]; then
    printf '\n--- %s ---\n' "${probe_stderr}"
    cat "${probe_stderr}"
  fi
  print_sanitizer_logs
  exit 1
}

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping sanitizers."
  exit 0
fi

normalize_log_dir
mkdir -p "${LOG_DIR}"
rm -f "${LOG_DIR}"/asan* "${LOG_DIR}"/ubsan* "${LOG_DIR}"/lsan*

if command -v llvm-symbolizer >/dev/null 2>&1; then
  export ASAN_SYMBOLIZER_PATH="${ASAN_SYMBOLIZER_PATH:-$(command -v llvm-symbolizer)}"
fi

detect_leaks_default=1

print_sanitizer_logs() {
  shopt -s nullglob
  local logs=(
    "${LOG_DIR}"/asan*
    "${LOG_DIR}"/ubsan*
    "${LOG_DIR}"/lsan*
    "${LOG_DIR}"/probe-asan*
    "${LOG_DIR}"/probe-ubsan*
    "${LOG_DIR}"/probe-lsan*
  )
  printf '\n[Sanitizer log directory] %s\n' "${LOG_DIR}"
  if [[ "${#logs[@]}" -eq 0 ]]; then
    echo "No sanitizer log files were produced."
    return
  fi

  printf '\n[Sanitizer logs]\n'
  for log_file in "${logs[@]}"; do
    if [[ -f "${log_file}" ]]; then
      printf '\n--- %s ---\n' "${log_file}"
      cat "${log_file}"
    fi
  done
}

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS="${SANITIZER_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${SANITIZER_FLAGS}" \
  "${CMAKE_ARGS[@]}"

if ! cmake --build "${BUILD_DIR}" --parallel; then
  print_sanitizer_logs
  exit 1
fi

if ! probe_leak_detection_compatibility; then
  detect_leaks_default=0
fi

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=${detect_leaks_default}:strict_string_checks=1:halt_on_error=1:log_path=${LOG_DIR}/asan}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1:log_path=${LOG_DIR}/ubsan}"
export LSAN_OPTIONS="${LSAN_OPTIONS:-exitcode=23:log_path=${LOG_DIR}/lsan}"

if ! ctest --test-dir "${BUILD_DIR}" --output-on-failure; then
  print_sanitizer_logs
  exit 1
fi
