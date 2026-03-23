# AGENTS.md

本文件约束在本仓库内工作的所有代理、脚本与自动化流程。

## Code Style

- 所有 C++ 代码遵循 Google C++ Style。
- 统一使用 `clang-format -style=Google` 作为格式基线。
- 统一使用仓库现有的 `.cc` / `.h` 命名约定，不新增 `.cpp` / `.hpp` 风格漂移。
- 新增或修改 C++ 代码时，优先保持：
  - 2 空格缩进
  - Google 风格的 include 顺序
  - 清晰、直接、可维护的命名
  - 小而单一职责的头文件与实现文件

## Worktree / Branch / PR Policy

- 一个 `worktree` 只做一个任务。
- 一个 `worktree` 只绑定一个分支，不复用同一个 `worktree` 处理多个分支。
- 一个分支原则上只提一个 PR，不把多个无关任务堆到同一分支里。
- 无依赖任务一律从 `main` 拉出新分支。
- 只有存在明确依赖关系的任务，才允许使用 stacked PR。
- PR 合并后，立刻清理对应 `worktree` 和已合并分支。

## Execution Expectations

- 开始任务前，先判断该任务是否依赖未合并分支；如果没有，直接基于 `main` 开工。
- 不为了图省事把多个任务塞进同一分支、同一 `worktree` 或同一 PR。
- 任何 stacked PR 都必须明确说明其上游依赖分支与合并顺序。
- 分支或 `worktree` 一旦失去用途，应立即删除，不保留长期悬挂的开发现场。
