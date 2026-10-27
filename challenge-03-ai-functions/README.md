# 赛题三 AI 函数增强

## 目标

完善 AI 函数在真实 SQL 执行中的输入类型、长文本或 LOB、JSON prompt 参数、空值和维度处理，支持 embedding 批量路径，并保持端点配置校验和结果语义。

## 当前状态

已完成。在本地构建的 seekdb 1.4.0.0 上执行官方测试
`ai_function_enhancement.test`，52 条语句全部通过，输出与官方
`ai_function_enhancement.result` 逐字节一致。

## 修复内容

官方用例暴露出 4 个真实缺陷，均已定位到源码并修复：

1. `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_complete.cpp`
   `ai_complete` 的第三个参数为 SQL NULL 时，代码只判断了 datum 指针非空，
   仍把全零 datum 当作 JSON 文本解析，导致 `ER_INVALID_JSON_TEXT_IN_PARAM`
   （用例 `ai_complete('enhance_complete', 'hello', null)`）。
   修复：追加 `!arg_config->is_null()` 判断，NULL 视为未传 config。

2. `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_rerank.cpp`
   `ai_rerank` 的第四个参数为 SQL NULL 时，代码同样只判断 datum 指针，
   把 NULL 当成“提供了 doc_key”，于是要求 documents 元素必须是对象，
   导致 `ER_WRONG_ARGUMENTS`（用例传入字符串数组 + null doc_key）。
   修复：以 `OB_NOT_NULL(arg_doc_key) && !arg_doc_key->is_null()` 计算
   `has_doc_key`，三处判断统一使用该标志；NULL 时走字符串数组路径。

3. `src/sql/engine/expr/ob_expr_ai/ob_ai_func_utils.cpp`
   Ollama provider 的 `get_header` 直接返回空 header 数组，而
   `ObAIFuncClient::init` 要求 headers 非空，请求在发出前就返回
   `OB_INVALID_ARGUMENT`，Ollama completion / embedding 全部失败。
   修复：Ollama 仍不携带鉴权头，但返回 `Content-Type: application/json`。

4. `src/observer/ai_service/ob_ai_service_proxy.cpp`
   `ALTER_AI_MODEL_ENDPOINT` 会把空 access_key 落成 SQL NULL；
   之后按模型名解析端点时，`EXTRACT_VARCHAR_FIELD_MYSQL` 读到 NULL 抛出
   `OB_ERR_NULL_VALUE`（客户端显示 `Null value`），且发生在 HTTP 请求之前。
   官方用例在 `ALTER ... '{"provider":"Ollama"}'` 之后调用 `ai_complete` 正好触发。
   修复：access_key 改为允许 NULL 读取（与 provider / request_model_name 等列一致），
   空 key 对无鉴权 provider 继续合法；非 Ollama provider 仍由 `check_valid()` 校验非空。

对应补丁：`evidence/ai-function-fixes.patch`。

## 验收环境

- 平台：Windows 10/11 x64
- 源码：`oceanbase/seekdb` 分支 `2026-competition`
- 构建：项目内置 LLVM 18 clang-cl + CMake/Ninja（RelWithDebInfo），Rust 1.98.1
- 服务：`seekdb.exe`，端口 12881，`--base-dir` 指向独立运行目录

## 复现步骤

```bash
# 1. 构建（仓库根目录，环境变量见 challenge-01 README）
"$PWD/deps/3rd/tools/ninja/ninja.exe" -C build_release -j 16

# 2. 启动 seekdb
seekdb.exe --nodaemon --base-dir=<runtime> --port=12881 \
  --parameter memory_budget=2G --parameter cpu_count=4 \
  --parameter datafile_size=1G --parameter datafile_maxsize=4G \
  --parameter log_disk_size=2G --parameter max_syslog_file_count=5 \
  --parameter enable_rpc_service=false --log-level=WARN

# 3. 启动本地 AI mock（OpenAI / SiliconFlow / Ollama 三类接口）
node tools/ai_mock.js seekdb/challenge-03-ai-functions/evidence/mock-requests.log

# 4. 执行官方测试并与官方 result 对比
node tools/run_ai_test.js \
  seekdb/challenge-03-ai-functions/evidence/ai_function_enhancement.test \
  seekdb/challenge-03-ai-functions/evidence/ai_function_enhancement.result \
  seekdb/challenge-03-ai-functions/evidence/ai_function_enhancement.run.log
```

## 验收结果

- 构建：增量编译 + 链接成功（`evidence/build.log`）
- 测试：`statements=52 failures=0 result_matches=true`
  （完整输出见 `evidence/ai_function_enhancement.run.log`）
- 覆盖路径：`ai_prompt`（JSON/重复占位符/LOB）、`ai_complete`（含 NULL config、
  config 透传）、`ai_embed`（单行、指定维度、批量 + NULL 行）、`ai_rerank`
  （doc_key 与无 doc_key）、Ollama completion 与 embedding（大小写 provider + ALTER）
- mock 实际收到的请求（`evidence/mock-requests.log`）：
  `chat/completions` 2 次、`embeddings` 4 次、`rerank` 2 次、
  `/api/generate` 1 次、`/api/embed` 1 次

## 证据清单

- `evidence/ai_function_enhancement.test` / `.result`：官方用例与期望输出原件
- `evidence/ai_function_enhancement.run.log`：本地真实执行输出
- `evidence/mock-requests.log`：mock 服务收到的请求（JSONL）
- `evidence/mock-server.out`：mock 服务启动输出
- `evidence/build.log`：修复后增量构建输出
- `evidence/ai-function-fixes.patch`：4 个源文件的完整改动

## 环境适配说明

- 官方用例中的 `http://0.0.0.0:8000` 在 Windows 上不可路由，执行时改写为
  `http://127.0.0.1:8000`；用例文本与期望结果按原件保存，改写只发生在执行期，
  因此 `.result` 对比不受影响。
- 测试使用本地 mock AI 服务返回确定性结果，未连接真实 OpenAI / SiliconFlow / Ollama
  线上服务；验证的是 seekdb 侧的参数处理、请求构造与结果解析。
- `tools/run_ai_test.js` 是最小 mysqltest 兼容执行器（支持 `--error`、
  `--disable_warnings`、`--disable_result_log`、`connection`），最终输出与官方
  `.result` 全文一致。
