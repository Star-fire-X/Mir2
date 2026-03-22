# CI 质量门禁增强设计方案

## 1. 文档定位

**文档名称**
《CI 质量门禁增强设计方案》

**文档目标**
在不替换现有 `gcovr` 覆盖率方案的前提下，增强 GitHub Actions 工作流的质量门禁，把 `clang-tidy` 从现有静态分析任务中拆出为独立 job，并新增 `ASan + UBSan` 运行时校验。

**适用范围**
- 当前仓库唯一的 GitHub Actions workflow: [`.github/workflows/ci.yml`](/home/wsluser/mir2/.github/workflows/ci.yml)
- 当前 CI 脚本目录: `scripts/ci/`
- 当前 CMake + GoogleTest 工程结构

## 2. 当前状态

当前 CI 已有三个 job：

1. `Build and Test (gcc/clang)`：编译并执行测试
2. `Coverage`：使用 `gcovr` 生成覆盖率产物
3. `Static Analysis`：混合运行 `clang-tidy`、`cppcheck`、`clang-format`、`cpplint`

现有问题：

- `Static Analysis` 同时承担语义分析、静态规则、格式和风格检查，失败原因不够清晰
- `clang-tidy` 与 `cppcheck` 等工具混跑，问题定位成本高
- 当前 CI 缺少运行时内存/未定义行为检查，无法在测试阶段尽早发现堆越界、use-after-free、未定义整数/指针行为等问题

## 3. 设计目标与边界

### 3.1 目标

1. 保留现有 `gcovr` 覆盖率流程，不引入 `lcov`
2. 将 `clang-tidy` 拆分为独立的 `Clang-Tidy` job
3. 保留一个单独的 `Static Analysis` job，只负责 `cppcheck`、`clang-format`、`cpplint`
4. 新增 `Sanitizers` job，启用 `ASan + UBSan`
5. 尽量复用现有 CI 脚本和 CMake 配置，避免对业务代码做无关改造

### 3.2 明确不做

- 不引入 `lcov`
- 不新增 `TSan`
- 不拆分成过细的多个 lint job
- 不引入第三方 SaaS 覆盖率平台
- 不要求本次同步改造所有本地开发脚本

## 4. 方案概览

### 4.1 最终 job 结构

CI workflow 调整后保留 5 个 job：

1. `Build and Test (${{ matrix.compiler }})`
2. `Coverage`
3. `Clang-Tidy`
4. `Static Analysis`
5. `Sanitizers`

### 4.2 职责划分

**Build and Test**
- 继续作为基础编译与回归测试门禁
- 使用 gcc/clang 双编译器 matrix

**Coverage**
- 继续使用 `gcovr`
- 不改现有产物格式与上传逻辑

**Clang-Tidy**
- 只负责生成 `compile_commands.json` 并运行 `clang-tidy`
- 出错时明确表示是语义/风格规则问题

**Static Analysis**
- 仅负责 `cppcheck`、`clang-format`、`cpplint`
- 不再运行 `clang-tidy`

**Sanitizers**
- 使用 `clang/clang++`
- 在测试运行阶段启用 `ASan + UBSan`
- 失败时明确表示是运行时内存或未定义行为问题

## 5. 工作流设计

### 5.1 `Coverage` 保持不变

覆盖率 job 继续沿用现有逻辑：

- `gcc/g++`
- `gcovr`
- `artifacts/coverage/coverage.xml`
- `artifacts/coverage/coverage.html`

除非实现时发现脚本复用关系必须调整，否则该 job 不改行为。

### 5.2 新增 `Clang-Tidy` job

建议配置：

- 运行环境：`ubuntu-latest`
- 编译器：`clang/clang++`
- 安装依赖：
  - `cmake`
  - `ninja-build`
  - `clang`
  - `clang-tidy`
  - `ripgrep`

执行脚本：

- 新增 `scripts/ci/run_clang_tidy.sh`

行为：

1. 配置独立构建目录，例如 `build-tidy`
2. 导出 `compile_commands.json`
3. 收集 translation units
4. 仅对 `.cc/.cpp/.cxx` 源文件执行 `clang-tidy`

### 5.3 调整 `Static Analysis` job

当前 `Static Analysis` job 继续保留，但其脚本要瘦身：

- 删除 `clang-tidy` 执行逻辑
- 保留：
  - `cppcheck`
  - `clang-format --dry-run --Werror`
  - `cpplint`

依赖建议：

- `cmake`
- `ninja-build`
- `clang`
- `clang-format`
- `cppcheck`
- `python3-pip`
- `ripgrep`

### 5.4 新增 `Sanitizers` job

建议配置：

- 运行环境：`ubuntu-latest`
- 编译器：`clang/clang++`
- 独立 build 目录，例如 `build-sanitizers`

编译参数：

```bash
-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g
```

执行脚本：

- 新增 `scripts/ci/run_sanitizers.sh`

行为：

1. 配置 CMake
2. 为 C/C++ flags 注入 sanitizer 参数
3. 编译测试目标
4. 执行 `ctest --output-on-failure`

环境变量建议：

```bash
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1
UBSAN_OPTIONS=print_stacktrace=1
```

## 6. 脚本设计

### 6.1 新增 `scripts/ci/run_clang_tidy.sh`

职责：

- 复用现有 `run_static_checks.sh` 中关于仓库根目录、构建目录、CMake 配置和 TU 过滤的逻辑
- 不运行 `cppcheck`、`clang-format`、`cpplint`

关键步骤：

1. 发现仓库根目录
2. 配置 `build-tidy`
3. 构建生成 `compile_commands.json`
4. 使用 `rg` 或等价过滤方式选出 `.cc/.cpp/.cxx`
5. 调用 `clang-tidy -p <build-dir> ...`

### 6.2 修改 `scripts/ci/run_static_checks.sh`

职责变化：

- 从“混合静态分析总入口”调整为“非 tidy 静态检查入口”

保留逻辑：

- `cppcheck`
- `clang-format`
- `cpplint`

移除逻辑：

- `clang-tidy`

### 6.3 新增 `scripts/ci/run_sanitizers.sh`

职责：

- 生成启用 `ASan + UBSan` 的独立构建目录
- 编译测试并执行 `ctest`

建议实现方式：

```bash
cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G "${GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g"

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure
```

## 7. 失败语义与排障价值

拆分后，PR 上的失败来源会更明确：

- `Build and Test` 失败：代码无法编译或基础测试失败
- `Coverage` 失败：覆盖率构建或报告生成问题
- `Clang-Tidy` 失败：语义级检查或规则问题
- `Static Analysis` 失败：`cppcheck` / `clang-format` / `cpplint` 问题
- `Sanitizers` 失败：运行时内存错误或未定义行为

这比当前“所有分析混在一个 job 里失败”更容易定位，也更符合后续逐步加严规则的演进方向。

## 8. 风险与取舍

### 8.1 CI 时间变长

拆出独立 `Clang-Tidy` 和新增 `Sanitizers` 后，总时长会增加。当前项目体量较小，这个成本可接受；后续如果 CI 时长明显增长，再考虑：

- 增加 path filter
- 限制 `clang-tidy` 目标范围
- 在 sanitizer job 中只跑测试目标

### 8.2 Sanitizer 对第三方依赖更敏感

如果后续引入更多依赖，`ASan/UBSan` 可能暴露第三方或测试框架边缘行为。首版先不做 suppressions，只有在真实出现噪音时再补针对性配置。

### 8.3 脚本重复

新增 `run_clang_tidy.sh` 和 `run_sanitizers.sh` 后，脚本间会存在部分重复。当前阶段优先保证 CI 职责边界清晰，不为了去重而提前抽象复杂公共脚本。

## 9. 实施顺序

建议按以下顺序落地：

1. 新增 `run_clang_tidy.sh`
2. 从 `run_static_checks.sh` 中移除 `clang-tidy`
3. 新增 `run_sanitizers.sh`
4. 修改 workflow，增加 `Clang-Tidy` 与 `Sanitizers`
5. 本地逐个验证脚本
6. 推送 PR，观察 GitHub Actions 实际表现

## 10. 验证方案

实现后本地至少验证以下命令：

```bash
bash scripts/ci/run_clang_tidy.sh
bash scripts/ci/run_static_checks.sh
bash scripts/ci/run_sanitizers.sh
bash scripts/ci/generate_coverage.sh
```

GitHub Actions 层面需要确认：

1. 新 workflow 能正常触发
2. `Clang-Tidy` job 独立显示
3. `Static Analysis` 不再执行 `clang-tidy`
4. `Sanitizers` job 能成功构建并运行测试
5. `Coverage` 仍继续生成 `gcovr` 产物
