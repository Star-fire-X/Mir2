#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping build and tests."
  exit 0
fi

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure
