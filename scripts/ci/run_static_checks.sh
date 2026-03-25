#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-static}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
GOOGLETEST_SOURCE_DIR="${GOOGLETEST_SOURCE_DIR:-}"

# Support tools installed with: python3 -m pip install --user <tool>
export PATH="$HOME/.local/bin:$PATH"

collect_cpp_files() {
  cd "${ROOT_DIR}"
  find . -type f \
    \( -name "*.h" -o -name "*.hpp" -o -name "*.hh" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" \) \
    -not -path "./.git/*" \
    -not -path "./.worktrees/*" \
    -not -path "./build/*" \
    -not -path "./build-*/*" \
    -not -path "./third_party/*" | sort
}

CMAKE_ARGS=()
if [[ -n "${GOOGLETEST_SOURCE_DIR}" ]]; then
  CMAKE_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${GOOGLETEST_SOURCE_DIR}")
fi

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping static checks."
  exit 0
fi

mapfile -t CPP_FILES < <(collect_cpp_files)

if [[ "${#CPP_FILES[@]}" -eq 0 ]]; then
  echo "::notice::No C++ files found. Skipping static checks."
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

if ! command -v cppcheck >/dev/null 2>&1; then
  echo "cppcheck is required for static analysis."
  exit 1
fi

cppcheck \
  --project="${BUILD_DIR}/compile_commands.json" \
  --error-exitcode=1 \
  --std=c++20 \
  --enable=warning,style,performance,portability \
  --library=googletest \
  --suppress=missingIncludeSystem \
  -i "${BUILD_DIR}/_deps" \
  -i "${BUILD_DIR}/CMakeFiles" \
  --inline-suppr

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required for format checks."
  exit 1
fi

if ! command -v cpplint >/dev/null 2>&1; then
  echo "cpplint is required for style checks."
  exit 1
fi

(
  cd "${ROOT_DIR}"
  clang-format --dry-run --Werror -style=Google "${CPP_FILES[@]}"
  cpplint \
    --quiet \
    --repository="${ROOT_DIR}" \
    --filter=-legal/copyright \
    "${CPP_FILES[@]}"
)
