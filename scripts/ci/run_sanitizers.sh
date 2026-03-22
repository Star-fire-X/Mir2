#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-sanitizers}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
SANITIZER_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g"
GOOGLETEST_SOURCE_DIR="${GOOGLETEST_SOURCE_DIR:-}"

CMAKE_ARGS=()
if [[ -n "${GOOGLETEST_SOURCE_DIR}" ]]; then
  CMAKE_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${GOOGLETEST_SOURCE_DIR}")
fi

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping sanitizers."
  exit 0
fi

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:strict_string_checks=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1}"

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS="${SANITIZER_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${SANITIZER_FLAGS}" \
  "${CMAKE_ARGS[@]}"

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure
