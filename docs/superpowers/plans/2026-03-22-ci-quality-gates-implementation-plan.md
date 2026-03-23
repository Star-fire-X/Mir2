# CI Quality Gates Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split `clang-tidy` into its own GitHub Actions job, keep `gcovr` coverage unchanged, and add an `ASan + UBSan` sanitizer job with repeatable local verification.

**Architecture:** The workflow keeps existing build/test and `gcovr` coverage flows intact, while CI quality gates are split into three independent responsibilities: `clang-tidy`, non-tidy static analysis, and runtime sanitizers. A small shell regression test locks in the required workflow/script structure before the implementation changes land.

**Tech Stack:** GitHub Actions, Bash, CMake, Ninja, clang-tidy, cppcheck, clang-format, cpplint, AddressSanitizer, UndefinedBehaviorSanitizer

---

### Task 1: Lock In CI Structure Expectations

**Files:**
- Create: `scripts/ci/test_quality_gates_layout.sh`
- Test: `scripts/ci/test_quality_gates_layout.sh`

- [ ] **Step 1: Write the failing CI layout regression test**

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORKFLOW="${ROOT_DIR}/.github/workflows/ci.yml"

test -f "${ROOT_DIR}/scripts/ci/run_clang_tidy.sh"
test -f "${ROOT_DIR}/scripts/ci/run_sanitizers.sh"
grep -q "clang_tidy:" "${WORKFLOW}"
grep -q "sanitizers:" "${WORKFLOW}"
```

- [ ] **Step 2: Run the regression test and verify it fails**

Run: `bash scripts/ci/test_quality_gates_layout.sh`
Expected: FAIL because `run_clang_tidy.sh`, `run_sanitizers.sh`, and the new workflow jobs do not exist yet

- [ ] **Step 3: Commit the failing test only after the implementation passes**

```bash
git add scripts/ci/test_quality_gates_layout.sh
```

### Task 2: Split Clang-Tidy Out of Static Analysis

**Files:**
- Create: `scripts/ci/run_clang_tidy.sh`
- Modify: `scripts/ci/run_static_checks.sh`
- Modify: `.github/workflows/ci.yml`
- Test: `scripts/ci/test_quality_gates_layout.sh`

- [ ] **Step 1: Create the dedicated `clang-tidy` runner**

Implement `scripts/ci/run_clang_tidy.sh` with:
- root/build dir discovery
- CMake configure with `compile_commands.json`
- TU filtering via `rg`
- `clang-tidy -p "${BUILD_DIR}" "${TU_FILES[@]}"`

- [ ] **Step 2: Remove `clang-tidy` from `run_static_checks.sh`**

Keep only:
- `cppcheck`
- `clang-format --dry-run --Werror`
- `cpplint`

- [ ] **Step 3: Update the workflow**

In `.github/workflows/ci.yml`:
- add a new `clang_tidy` job
- keep `static_analysis` but remove `clang-tidy`-specific package dependencies
- wire `clang_tidy` to `bash scripts/ci/run_clang_tidy.sh`

- [ ] **Step 4: Run the regression test and verify it passes**

Run: `bash scripts/ci/test_quality_gates_layout.sh`
Expected: PASS for the `clang_tidy` split expectations

- [ ] **Step 5: Run targeted local verification**

Run:
- `bash scripts/ci/run_clang_tidy.sh`
- `bash scripts/ci/run_static_checks.sh`

Expected:
- `run_clang_tidy.sh` passes independently
- `run_static_checks.sh` passes without invoking `clang-tidy`

### Task 3: Add Sanitizer Execution

**Files:**
- Create: `scripts/ci/run_sanitizers.sh`
- Modify: `.github/workflows/ci.yml`
- Test: `scripts/ci/test_quality_gates_layout.sh`

- [ ] **Step 1: Implement the sanitizer runner**

Create `scripts/ci/run_sanitizers.sh` that:
- configures an isolated build directory
- sets `CMAKE_C_FLAGS` and `CMAKE_CXX_FLAGS` to `-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g`
- exports `ASAN_OPTIONS` and `UBSAN_OPTIONS`
- builds and runs `ctest --output-on-failure`

- [ ] **Step 2: Add the `sanitizers` workflow job**

In `.github/workflows/ci.yml`:
- add a new `sanitizers` job
- install the minimum toolchain for clang sanitizer builds
- invoke `bash scripts/ci/run_sanitizers.sh`

- [ ] **Step 3: Re-run the regression test**

Run: `bash scripts/ci/test_quality_gates_layout.sh`
Expected: PASS for sanitizer script and workflow job expectations

- [ ] **Step 4: Run local sanitizer verification**

Run: `bash scripts/ci/run_sanitizers.sh`
Expected: PASS with tests completing under `ASan + UBSan`

### Task 4: Final Verification and Commit

**Files:**
- Modify: `.github/workflows/ci.yml`
- Modify: `scripts/ci/run_static_checks.sh`
- Create: `scripts/ci/run_clang_tidy.sh`
- Create: `scripts/ci/run_sanitizers.sh`
- Create: `scripts/ci/test_quality_gates_layout.sh`

- [ ] **Step 1: Run the complete local verification set**

Run:
- `bash scripts/ci/test_quality_gates_layout.sh`
- `bash scripts/ci/run_clang_tidy.sh`
- `bash scripts/ci/run_static_checks.sh`
- `bash scripts/ci/run_sanitizers.sh`
- `bash scripts/ci/generate_coverage.sh`

Expected: All commands pass

- [ ] **Step 2: Inspect final diff**

Run: `git diff --check`
Expected: no whitespace or merge-marker issues

- [ ] **Step 3: Commit the implementation**

```bash
git add .github/workflows/ci.yml scripts/ci
git commit -m "ci: split clang-tidy and add sanitizers"
```
