# 赛题一 部署一个 seekdb

## 目标

在可验收环境部署并启动 seekdb，确认 SQL 服务可连接，并保留启动、连接和版本证据。

## 当前状态

已完成。本机从源码构建并启动 seekdb 1.4.0.0，SQL 连接与建表/写入/聚合查询均通过，
遥测文件已由实例在启动时真实生成。

## 部署环境

- 操作系统：Windows 10/11 x64
- 源码：`oceanbase/seekdb`，分支 `2026-competition`，commit `e65751f08e0082d4aa38b22d036766b81650f044`
- 编译器：项目内置 LLVM 18 `clang-cl`
- 构建系统：CMake + Ninja（RelWithDebInfo）
- Rust：1.98.1（项目 `rust/rust-toolchain.toml` 固定版本）

## 构建与启动

构建（在 seekdb 仓库根目录）：

```bash
export PATH="$PWD/deps/3rd/tools/rustup/toolchains/1.98.1-x86_64-pc-windows-msvc/bin:$PWD/deps/3rd/tools/cmake/bin:$PWD/deps/3rd/tools/ninja:$PWD/deps/3rd/tools/llvm18/bin:$PWD/deps/3rd/tools/win_flex_bison:$PATH"
export CARGO_HOME="$PWD/deps/3rd/tools/cargo"
export RUSTC="$PWD/deps/3rd/tools/rustup/toolchains/1.98.1-x86_64-pc-windows-msvc/bin/rustc.exe"
export CXXFLAGS="-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH" CFLAGS="-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
cmake -S . -B build_release -DPython3_EXECUTABLE=<python.exe>
"$PWD/deps/3rd/tools/ninja/ninja.exe" -C build_release -j 16
```

产物：`build_release/src/observer/seekdb.exe`。

启动（运行目录单独放置）：

```bash
seekdb.exe --nodaemon \
  --base-dir=<runtime> --port=12881 \
  --parameter memory_budget=2G --parameter cpu_count=4 \
  --parameter datafile_size=1G --parameter datafile_maxsize=4G \
  --parameter log_disk_size=2G --parameter max_syslog_file_count=5 \
  --parameter enable_rpc_service=false --log-level=WARN
```

实例监听 `0.0.0.0:12881`，日志位于 `<runtime>/log/seekdb.log`。

## 验收记录

- 构建：成功，`seekdb.exe` 112,865,280 字节
- 版本：`OceanBase seekdb 1.4.0.0`，`REVISION 1-e65751f08e0082d4aa38b22d036766b81650f044`，`BUILD_BRANCH 2026-competition`（见 `evidence/version.txt`）
- SQL 连接和版本：`SELECT VERSION()` 返回 `5.7.25-OceanBase seekdb-v1.4.0.0`
- SQL 功能冒烟：建库、建表、批量写入、排序查询、`COUNT/MIN` 聚合、清理全部通过（见 `evidence/sql-smoke.log`）
- 遥测标识：`d3a6464c-de14-8eec-ace8-7bbd67780e3f`

遥测 ID 来自实例运行目录 `run/telemetry.json` 的 `content.id`，文件原件见
`evidence/telemetry.json`（`event` 为 `bootstraped`，`port` 为 12881）。

获取方式：

```bash
cat <runtime>/run/telemetry.json
```

## 边界说明

- 本次为本机源码构建的 RelWithDebInfo 版本，不是官方发布的 deb/rpm 安装包。
- 未在本机验证 AI 相关网络功能；本题只要求实例启动和 SQL 可用。
- 遥测 ID 需由参赛者本人填写到大赛提测平台的赛题一入口，本仓库只保存证据。
