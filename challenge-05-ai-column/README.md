# 赛题五 AI 列

## 目标

实现模型驱动的派生列自动维护，覆盖定义、写入与更新触发、结果持久化、失败语义和后续查询读取。

## 当前状态

**未实现。** 本题需要数据库支持「AI 列」（建表时把某列声明为模型派生列，写入/更新时自动调用模型并把结果持久化）。核查本仓库 `2026-competition` 分支源码后确认：AI 列的 DDL、元数据与执行链路在当前代码树中不存在，只有测试骨架。

## 核查证据

在本仓库根目录执行：

```bash
grep -rln "AI_COLUMN\|ai_column" --include=*.h --include=*.cpp src/ | wc -l
# 0

ls tools/deploy/mysql_test/test_suite/ai_column/t/
# ai_column_ddl.test  ai_column_retry_strict.test  （只有测试用例）
ls tools/deploy/mysql_test/test_suite/ai_column/r/mysql/
# ai_column_ddl.result  ai_column_t1_dml.result  ai_column_worker.result  ...
```

即：`src/` 中 0 处匹配 AI 列相关标识符（DDL/元数据/执行器均不存在），`tools/deploy/mysql_test/test_suite/ai_column/` 仅有测试与预期结果文件。

说明：赛题三（AI 函数，`DBMS_AI_SERVICE`/`AI_COMPLETE`/`AI_EMBED` 等）在本仓库已可用并有验收记录，见 `challenge-03-ai-functions/`；AI 列是独立特性，当前源码未提供。

## 为什么没有提交「实现」

- AI 列需要解析器、建表/写入/更新触发器、后台任务与持久化等完整链路；当前源码没有这些组件，只有测试用例；
- 依据交付约束（不得伪造测试结果/平台验收），本题如实标记为未完成，而不是提交占位代码或伪造执行日志。

## 验收记录

- 列定义：未实现（源码核查，见上）
- 写入和更新触发：未实现
- 结果持久化和查询：未实现
- 失败语义和回归测试：未实现
