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

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping sanitizers."
  exit 0
fi

mkdir -p "${LOG_DIR}"
rm -f "${LOG_DIR}"/asan* "${LOG_DIR}"/ubsan* "${LOG_DIR}"/lsan*

if command -v llvm-symbolizer >/dev/null 2>&1; then
  export ASAN_SYMBOLIZER_PATH="${ASAN_SYMBOLIZER_PATH:-$(command -v llvm-symbolizer)}"
fi

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:strict_string_checks=1:halt_on_error=1:log_path=${LOG_DIR}/asan}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1:log_path=${LOG_DIR}/ubsan}"
export LSAN_OPTIONS="${LSAN_OPTIONS:-exitcode=23:log_path=${LOG_DIR}/lsan}"

print_sanitizer_logs() {
  shopt -s nullglob
  local logs=("${LOG_DIR}"/asan* "${LOG_DIR}"/ubsan* "${LOG_DIR}"/lsan*)
  if [[ "${#logs[@]}" -eq 0 ]]; then
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

if ! ctest --test-dir "${BUILD_DIR}" --output-on-failure; then
  print_sanitizer_logs
  exit 1
fi
