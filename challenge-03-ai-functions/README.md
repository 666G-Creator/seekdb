# 赛题三 AI 函数增强

## 目标

完善 AI 函数在真实 SQL 执行中的输入类型、长文本或 LOB、JSON prompt 参数、空值和维度处理，支持 embedding 批量路径，并保持端点配置校验和结果语义。

## 当前状态

已提交部分源码改动，尚未完成构建和 mysqltest 验收。

## 实现位置

- `src/share/ai_service/ob_ai_service_struct.cpp`
- `src/sql/engine/expr/ob_expr_ai/ob_ai_func_utils.cpp`
- `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_complete.cpp`
- `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_embed.cpp`
- `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_embed.h`
- `src/sql/engine/expr/ob_expr_ai/ob_expr_ai_prompt.cpp`

## 验收记录

- 编译：待完成
- AI prompt：待完成真实 SQL 验收
- AI embed：待完成真实 SQL 验收
- AI complete：待完成真实 SQL 验收
