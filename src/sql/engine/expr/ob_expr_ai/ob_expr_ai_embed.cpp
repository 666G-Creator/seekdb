/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define USING_LOG_PREFIX SQL_ENG
#include "ob_expr_ai_embed.h"
#include "query/engine/expr/ob_expr_lob_utils.h"
#include "query/ai/ob_ai_endpoint_resolver.h"
#include "share/rc/ob_server_runtime.h"

using namespace oceanbase::common;
using namespace oceanbase::sql;

namespace oceanbase 
{
namespace sql 
{
ObExprAIEmbed::ObExprAIEmbed(common::ObIAllocator &alloc)
    : ObFuncExprOperator(alloc, 
                        T_FUN_SYS_AI_EMBED, 
                        N_AI_EMBED, 
                        MORE_THAN_ZERO,
                        NOT_VALID_FOR_GENERATED_COL, 
                        NOT_ROW_DIMENSION) 
{
}

ObExprAIEmbed::~ObExprAIEmbed() 
{
}

int ObExprAIEmbed::calc_result_typeN(ObExprResType &type,
                                     ObExprResType *types_stack,
                                     int64_t param_num,
                                     common::ObExprTypeCtx &type_ctx) const 
{
  UNUSED(type_ctx);
  UNUSED(types_stack);
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(param_num > 3 || param_num < 2)) {
    ObString func_name_(get_name());
    ret = OB_ERR_PARAM_SIZE;
    LOG_USER_ERROR(OB_ERR_PARAM_SIZE, func_name_.length(), func_name_.ptr());
  } else {
    types_stack[MODEL_IDX].set_calc_type(ObVarcharType);
    types_stack[MODEL_IDX].set_calc_collation_type(CS_TYPE_UTF8MB4_BIN);
    types_stack[CONTENT_IDX].set_calc_collation_type(CS_TYPE_UTF8MB4_BIN);
    if (param_num == 3) {
      if (ob_is_integer_type(types_stack[DIM_IDX].get_type())) {
        types_stack[DIM_IDX].set_calc_type(ObIntType);
        types_stack[DIM_IDX].set_precision(10);
        types_stack[DIM_IDX].set_scale(0);
      } else {
        ret = OB_INVALID_ARGUMENT;
        LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, dimension parameter must be an integer, not a decimal or float");
      }
    }
    type.set_varchar();
    type.set_collation_type(CS_TYPE_UTF8MB4_BIN);
    type.set_collation_level(CS_LEVEL_COERCIBLE);
  }
  return ret;
}

int ObExprAIEmbed::eval_ai_embed(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &res) 
{
  INIT_SUCC(ret);
  ObDatum *arg_model_id = nullptr;
  ObDatum *arg_content = nullptr;
  ObDatum *arg_dim = nullptr;
  if (expr.arg_cnt_ == 3 ? OB_FAIL(expr.eval_param_value(ctx, arg_model_id, arg_content, arg_dim))
                         : OB_FAIL(expr.eval_param_value(ctx, arg_model_id, arg_content))) {
  } else if (arg_model_id->is_null() || arg_content->is_null() ||
             (OB_NOT_NULL(arg_dim) && arg_dim->is_null())) {
    res.set_null();
  } else {
    ObEvalCtx::TempAllocGuard tmp_alloc_g(ctx);
    
    MultimodeAlloctor temp_allocator(tmp_alloc_g.get_allocator());
    lib::ObMallocHookAttrGuard malloc_guard(lib::ObMemAttr(N_AI_EMBED));
    ObAIFuncExprInfo *info = nullptr;
    share::ObAiModelEndpointInfo resolved_endpoint;
    const share::ObAiModelEndpointInfo *endpoint_info = &resolved_endpoint;
    query::ObIAiEndpointResolver *endpoint_resolver =
        ::oceanbase::share::server_service<::oceanbase::query::ObIAiEndpointResolver>();
    ObString model_id = arg_model_id->get_string();
    ObString content;
    if (OB_FAIL(ObTextStringHelper::read_real_string_data(
            ctx.exec_ctx_, temp_allocator, *arg_content, expr.args_[CONTENT_IDX]->datum_meta_,
            expr.args_[CONTENT_IDX]->obj_meta_.has_lob_header(), content))) {
      LOG_WARN("failed to read ai_embed content", K(ret));
    }
    if (OB_FAIL(ret)) {
    } else if (model_id.empty() || content.empty()) {
      ret = OB_INVALID_ARGUMENT;
      LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, model id or input is empty");
      res.set_null();
    }
    int64_t dim = 0;
    ObJsonInt *dim_json = nullptr;
    ObJsonObject *config = nullptr;
    if (OB_FAIL(ret)) {
    } else if (OB_NOT_NULL(arg_dim)) {
      dim = arg_dim->get_int();
      if (dim <= 0) {
        ret = OB_INVALID_ARGUMENT;
        LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, dimension parameter must be a positive integer");
        res.set_null();
      } else if (OB_FAIL(ObAIFuncJsonUtils::get_json_object(temp_allocator, config))) {
      } else if (OB_FAIL(ObAIFuncJsonUtils::get_json_int(temp_allocator, dim, dim_json))) {
      } else if (OB_FAIL(config->add("dimensions", dim_json))) {
      }
    }
    if (OB_FAIL(ret)) {
    } else if (OB_FAIL(ObAIFuncUtils::get_ai_func_info(temp_allocator, model_id, info))) {
    } else if (OB_ISNULL(endpoint_resolver)) {
      ret = OB_ERR_UNEXPECTED;
    } else if (OB_FAIL(endpoint_resolver->resolve_by_model_name(
                   model_id, temp_allocator, resolved_endpoint))) {
    } else {
      ObAIFuncModel model(temp_allocator, *info, *endpoint_info);
      ObString result;
      if (OB_FAIL(model.call_dense_embedding(content, config, result))) {
      } else if (OB_FAIL(ObAIFuncUtils::set_string_result(expr, ctx, res, result))) {
      }
    }
  }
  return ret;
}

int ObExprAIEmbed::eval_ai_embed_batch(const ObExpr &expr, ObEvalCtx &ctx,
                                      const ObBitVector &skip, const int64_t batch_size)
{
  int ret = OB_SUCCESS;
  ObDatum *results = nullptr;
  ObDatum *model_datums = nullptr;
  ObDatum *content_datums = nullptr;
  ObDatum *dimension_datums = nullptr;
  ObBitVector &eval_flags = expr.get_evaluated_flags(ctx);
  if (OB_FAIL(expr.args_[MODEL_IDX]->eval_batch(ctx, skip, batch_size))) {
  } else if (OB_FAIL(expr.args_[CONTENT_IDX]->eval_batch(ctx, skip, batch_size))) {
  } else if (expr.arg_cnt_ == 3 && OB_FAIL(expr.args_[DIM_IDX]->eval_batch(ctx, skip, batch_size))) {
  } else if (OB_ISNULL(results = expr.locate_batch_datums(ctx)) ||
             OB_ISNULL(model_datums = expr.args_[MODEL_IDX]->locate_batch_datums(ctx)) ||
             OB_ISNULL(content_datums = expr.args_[CONTENT_IDX]->locate_batch_datums(ctx)) ||
             (expr.arg_cnt_ == 3 && OB_ISNULL(dimension_datums = expr.args_[DIM_IDX]->locate_batch_datums(ctx)))) {
    ret = OB_ERR_UNEXPECTED;
  } else {
    ObEvalCtx::TempAllocGuard tmp_alloc_g(ctx);
    MultimodeAlloctor allocator(tmp_alloc_g.get_allocator());
    lib::ObMallocHookAttrGuard malloc_guard(lib::ObMemAttr(N_AI_EMBED));
    bool *processed = static_cast<bool *>(allocator.alloc(sizeof(bool) * batch_size));
    if (OB_ISNULL(processed)) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
    } else {
      for (int64_t i = 0; i < batch_size; ++i) {
        processed[i] = false;
      }
      for (int64_t i = 0; OB_SUCC(ret) && i < batch_size; ++i) {
        if (skip.at(i) || eval_flags.at(i)) {
          processed[i] = true;
        } else if (model_datums[i].is_null() || content_datums[i].is_null() ||
                   (expr.arg_cnt_ == 3 && dimension_datums[i].is_null())) {
          results[i].set_null();
          eval_flags.set(i);
          processed[i] = true;
        } else {
          ObString model_id = model_datums[i].get_string();
          int64_t dimension = expr.arg_cnt_ == 3 ? dimension_datums[i].get_int() : 0;
          if (model_id.empty()) {
            ret = OB_INVALID_ARGUMENT;
            LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, model id is empty");
          } else if (expr.arg_cnt_ == 3 && dimension <= 0) {
            ret = OB_INVALID_ARGUMENT;
            LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, dimension parameter must be a positive integer");
          } else {
            ObArray<ObString> contents;
            ObArray<int64_t> row_indexes;
            for (int64_t j = i; OB_SUCC(ret) && j < batch_size; ++j) {
              if (processed[j] || skip.at(j) || eval_flags.at(j)) {
                processed[j] = true;
              } else if (model_datums[j].is_null() || content_datums[j].is_null() ||
                         (expr.arg_cnt_ == 3 && dimension_datums[j].is_null())) {
                results[j].set_null();
                eval_flags.set(j);
                processed[j] = true;
              } else {
                const ObString row_model_id = model_datums[j].get_string();
                const int64_t row_dimension = expr.arg_cnt_ == 3 ? dimension_datums[j].get_int() : 0;
                if (row_model_id.compare(model_id) == 0 && row_dimension == dimension) {
                  ObString content;
                  if (OB_FAIL(ObTextStringHelper::read_real_string_data(
                          ctx.exec_ctx_, allocator, content_datums[j], expr.args_[CONTENT_IDX]->datum_meta_,
                          expr.args_[CONTENT_IDX]->obj_meta_.has_lob_header(), content))) {
                  } else if (content.empty()) {
                    ret = OB_INVALID_ARGUMENT;
                    LOG_USER_ERROR(OB_INVALID_ARGUMENT, "ai_embed, input is empty");
                  } else if (OB_FAIL(contents.push_back(content))) {
                  } else if (OB_FAIL(row_indexes.push_back(j))) {
                  } else {
                    processed[j] = true;
                  }
                }
              }
            }

            if (OB_SUCC(ret) && contents.count() > 0) {
              ObAIFuncExprInfo *info = nullptr;
              share::ObAiModelEndpointInfo resolved_endpoint;
              query::ObIAiEndpointResolver *endpoint_resolver =
                  ::oceanbase::share::server_service<::oceanbase::query::ObIAiEndpointResolver>();
              ObJsonObject *config = nullptr;
              ObJsonInt *dimension_node = nullptr;
              if (dimension > 0) {
                if (OB_FAIL(ObAIFuncJsonUtils::get_json_object(allocator, config))) {
                } else if (OB_FAIL(ObAIFuncJsonUtils::get_json_int(allocator, dimension, dimension_node))) {
                } else if (OB_FAIL(config->add("dimensions", dimension_node))) {
                }
              }
              if (OB_FAIL(ret)) {
              } else if (OB_FAIL(ObAIFuncUtils::get_ai_func_info(allocator, model_id, info))) {
              } else if (OB_ISNULL(endpoint_resolver)) {
                ret = OB_ERR_UNEXPECTED;
              } else if (OB_FAIL(endpoint_resolver->resolve_by_model_name(model_id, allocator, resolved_endpoint))) {
              } else if (OB_FAIL(ObAIFuncUtils::check_info_type_dense_embedding(info))) {
              } else {
                ObAIFuncModel model(allocator, *info, resolved_endpoint);
                ObArray<ObString> embeddings;
                if (OB_FAIL(model.call_dense_embedding_vector_v2(contents, config, embeddings))) {
                } else if (embeddings.count() != row_indexes.count()) {
                  ret = OB_ERR_UNEXPECTED;
                } else {
                  for (int64_t k = 0; OB_SUCC(ret) && k < embeddings.count(); ++k) {
                    const int64_t row = row_indexes[k];
                    if (OB_FAIL(ObAIFuncUtils::set_string_result(expr, ctx, results[row], embeddings[k]))) {
                    } else {
                      eval_flags.set(row);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}

int ObExprAIEmbed::cg_expr(ObExprCGCtx &expr_cg_ctx, 
                           const ObRawExpr &raw_expr,
                           ObExpr &rt_expr) const 
{
  int ret = OB_SUCCESS;
  // TODO: support schema version match in plan cache for ai func
  // const ObRawExpr *model_key = raw_expr.get_param_expr(0);
  // if (OB_NOT_NULL(model_key)
  //     && (model_key->is_static_scalar_const_expr() || model_key->is_const_expr())
  //     && model_key->get_expr_type() != T_OP_GET_USER_VAR &&
  //     OB_NOT_NULL(expr_cg_ctx.schema_guard_)) {
  //   ObIAllocator *allocator = expr_cg_ctx.allocator_;
  //   ObExecContext *exec_ctx = expr_cg_ctx.session_->get_cur_exec_ctx();
  //   ObObj const_data;
  //   bool got_data = false;
  //   ObAIFuncExprInfo *info = nullptr;
  //   if (OB_ISNULL(allocator)) {
  //     ret = OB_ERR_UNEXPECTED;
  //     LOG_WARN("allocator is null", K(ret));
  //   } else if (OB_FAIL(ObSQLUtils::calc_const_or_calculable_expr(exec_ctx,
  //                                                         model_key,
  //                                                         const_data,
  //                                                         got_data,
  //                                                         *allocator))) {
  //     LOG_WARN("failed to calc offset expr", K(ret));
  //   } else if (!got_data || const_data.is_null()) {
  //   } else if (OB_FAIL(ObAIFuncUtils::get_ai_func_info(*allocator, const_data.get_string(), *expr_cg_ctx.schema_guard_, info))) {
  //     LOG_WARN("failed to get ai func info", K(ret), K(const_data.get_string()));
  //   } else {
  //     rt_expr.extra_info_ = info;
  //   }
  // }

  if (OB_SUCC(ret)) {
    rt_expr.eval_func_ = ObExprAIEmbed::eval_ai_embed;
    rt_expr.eval_batch_func_ = ObExprAIEmbed::eval_ai_embed_batch;
  }
  return ret;
}

} // namespace sql
} // namespace oceanbase
