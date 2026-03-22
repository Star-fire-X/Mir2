#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-static}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping static checks."
  exit 0
fi

mapfile -t CPP_FILES < <(
  find "${ROOT_DIR}" -type f \
    \( -name "*.h" -o -name "*.hpp" -o -name "*.hh" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" \) \
    -not -path "*/.git/*" \
    -not -path "*/build/*" \
    -not -path "*/build-*/*" \
    -not -path "*/third_party/*" | sort
)

if [[ "${#CPP_FILES[@]}" -eq 0 ]]; then
  echo "::notice::No C++ files found. Skipping static checks."
  exit 0
fi

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON

cmake --build "${BUILD_DIR}" --parallel

mapfile -t TU_FILES < <(printf "%s\n" "${CPP_FILES[@]}" | rg -N "\.(cc|cpp|cxx)$")

if [[ "${#TU_FILES[@]}" -gt 0 ]]; then
  if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is required for static analysis."
    exit 1
  fi
  clang-tidy -p "${BUILD_DIR}" "${TU_FILES[@]}"
fi

if ! command -v cppcheck >/dev/null 2>&1; then
  echo "cppcheck is required for static analysis."
  exit 1
fi

cppcheck \
  --project="${BUILD_DIR}/compile_commands.json" \
  --error-exitcode=1 \
  --std=c++20 \
  --enable=warning,style,performance,portability \
  --suppress=missingIncludeSystem \
  --inline-suppr

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required for format checks."
  exit 1
fi

clang-format --dry-run --Werror -style=Google "${CPP_FILES[@]}"

if ! command -v cpplint >/dev/null 2>&1; then
  echo "cpplint is required for style checks."
  exit 1
fi

cpplint \
  --quiet \
  --filter=-legal/copyright \
  "${CPP_FILES[@]}"
