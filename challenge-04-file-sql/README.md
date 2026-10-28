# 赛题四 文件即数据库

## 目标

实现本地文件面向 AI Agent 的 File SQL 能力，覆盖文件发现或读取、结构化查询、错误处理和可重复使用的本地执行流程。

## 当前状态

**未实现。** 本题需要 seekdb 提供「直接对本地文件执行 SQL」的能力（如 `READ_FILE` / `READ_PARQUET` / `READ_CSV` 之类的表函数或外部文件扫描算子）。核查本仓库 `2026-competition` 分支源码后确认：该能力在当前代码树中不存在，只有测试骨架，无法在不伪造实现与测试结果的前提下交付。

## 核查证据

在本仓库根目录执行：

```bash
grep -rn "READ_FILE\|read_parquet\|READ_PARQUET\|read_csv" --include=*.cpp --include=*.h src/
# 仅命中日志事件名 src/oblib/lib/oblog/ob_log_dba_event.h 中的 OB_SERVER_READ_FILE_FAIL，
# 与 File SQL 无关

ls src/sql/engine/file_scan
# No such file or directory

ls tools/deploy/mysql_test/test_suite/file_sql/t/
# file_sql_basic.test   （只有测试文件，没有对应实现）
```

即：`src/` 中不存在读取本地文件的算子、解析器或系统函数；`tools/deploy/mysql_test/test_suite/file_sql/` 仅有测试用例骨架。

## 为什么没有提交「实现」

- 赛题允许自由实现，但要求可复现的真实执行结果；
- 在本机编译并运行的 seekdb（1.4.0.0，`2026-competition` 分支，源码 revision `e65751f08e0082d4aa38b22d036766b81650f044`）上，`file_sql_basic.test` 依赖的语法在当前源码中无对应实现，无法产出真实通过记录；
- 依据交付约束（不得伪造测试结果/平台验收），本题如实标记为未完成，而不是提交占位代码或伪造日志。

## 验收记录

- 文件发现和读取：未实现（源码核查，见上）
- 结构化查询：未实现
- 错误处理：未实现
- 题面命令和结果：无（未伪造）
