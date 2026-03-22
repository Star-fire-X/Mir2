#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-tidy}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
GOOGLETEST_SOURCE_DIR="${GOOGLETEST_SOURCE_DIR:-}"

CMAKE_ARGS=()
if [[ -n "${GOOGLETEST_SOURCE_DIR}" ]]; then
  CMAKE_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${GOOGLETEST_SOURCE_DIR}")
fi

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping clang-tidy."
  exit 0
fi

mapfile -t CPP_FILES < <(
  find "${ROOT_DIR}" -type f \
    \( -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" \) \
    -not -path "*/.git/*" \
    -not -path "*/build/*" \
    -not -path "*/build-*/*" \
    -not -path "*/third_party/*" | sort
)

if [[ "${#CPP_FILES[@]}" -eq 0 ]]; then
  echo "::notice::No translation units found. Skipping clang-tidy."
  exit 0
fi

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  "${CMAKE_ARGS[@]}"

cmake --build "${BUILD_DIR}" --parallel

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy is required for clang-tidy checks."
  exit 1
fi

clang-tidy -p "${BUILD_DIR}" "${CPP_FILES[@]}"
