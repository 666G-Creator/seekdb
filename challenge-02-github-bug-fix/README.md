# 赛题二 GitHub bug 修复

## 结果

- Fork：<https://github.com/666G-Creator/seekdb>（`fork=true`，父仓库 `oceanbase/seekdb`）
- 修复分支：`fix/1480-uuid-node-fallback`，基于 upstream `master`（`4b28bcbff`）
- Pull Request：<https://github.com/oceanbase/seekdb/pull/1947>
- 对应 Issue：<https://github.com/oceanbase/seekdb/issues/1480>（标签 `2026-competition`）

## 问题

受限 Linux 沙箱中，standalone 启动在初始化 UUIDv1 的节点标识时无法获取 MAC 地址，`ObUUIDNode::init()` 返回 `OB_ERR_UNEXPECTED`（`-4016`），进程在 `server_start 4/14` 退出。

## 根因

`src/sql/engine/expr/ob_expr_uuid.cpp` 的 Linux 分支把 `socket()`、`SIOCGIFCONF`、`SIOCGIFHWADDR` 的任何失败以及“只有 loopback、没有可用 MAC”都当作初始化失败。UUIDv1 允许使用合成节点标识，因此 MAC 不可用不应阻断启动。

## 修复

- MAC 发现改为 best-effort：socket/枚举/单个网卡 ioctl 失败只记录 `LOG_WARN` 和 `errno`，不再置 `ret`。
- 没有可用 MAC 时从 `/dev/urandom` 读取 6 字节，按 RFC 9562 §6.10 设置 multicast 位（`mac_addr_[0] |= 0x01`），完整读入后才置 `is_inited_ = true`。
- 处理 `EINTR` 和短读，不丢弃已读字节；熵源真实失败仍返回 `OB_ERR_UNEXPECTED`。
- 保留原有 MAC 路径以及 Windows/Apple/Android/Emscripten 分支行为。

## 验证

- 将 upstream `master` 的 `ob_expr_uuid.cpp` 替换为补丁版本后，用项目自带 clang-cl 18 + Ninja 编译对应 unity 目标，退出码 0（`evidence/build.log`）。
- 该文件在 Windows 上只编译非 Linux 分支，因此本地构建只能证明补丁没有破坏其它平台分支，不能证明 Linux 回退路径已运行。
- 本机没有 Linux 环境，未在受限沙箱中做启动验证；PR 描述中已明确这一点，并将该场景列为需要在 Linux 主机/CI 上执行的验证项。
- 补丁仅修改 `src/sql/engine/expr/ob_expr_uuid.cpp` 一个文件，commit 见 `evidence/uuid-fix.patch`。

## 证据

- `evidence/pull-request.json`：PR、fork、Issue 状态
- `evidence/uuid-fix.patch`：PR commit 全文
- `evidence/build.log`：Windows 构建记录
