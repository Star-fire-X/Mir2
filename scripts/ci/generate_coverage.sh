#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-coverage}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
REPORT_DIR="${REPORT_DIR:-${ROOT_DIR}/artifacts/coverage}"

# Support tools installed with: python3 -m pip install --user <tool>
export PATH="$HOME/.local/bin:$PATH"

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "::notice::CMakeLists.txt not found at repository root. Skipping coverage."
  exit 0
fi

if ! command -v gcovr >/dev/null 2>&1; then
  echo "gcovr is required for coverage generation."
  exit 1
fi

mkdir -p "${REPORT_DIR}"

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS="--coverage -O0 -g" \
  -DCMAKE_CXX_FLAGS="--coverage -O0 -g"

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure

gcovr \
  --root "${ROOT_DIR}" \
  --object-directory "${BUILD_DIR}" \
  --exclude ".*/third_party/.*" \
  --gcov-exclude-directories ".*/CMakeFiles/3\\..*/CompilerId.*" \
  --exclude-directories ".*/_deps/.*" \
  --filter "server/.*" \
  --filter "client/.*" \
  --print-summary \
  --xml-pretty \
  --output "${REPORT_DIR}/coverage.xml" \
  --html-details "${REPORT_DIR}/coverage.html"
