# GitHub 协作与同步

## 公开的仓库配置

| 项目 | 值 |
| --- | --- |
| GitHub 仓库 | https://github.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal |
| Git 远端名 | origin |
| Git URL | https://github.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal.git |
| 默认分支 | main |
| 当前稳定分支 | main |
| 最近收尾分支 | feature/f407-p11-freertos-fault-management |
| 最近主分支提交 | 459d08c |
| 提交身份 | Jiang Zexuan <314187437+jiang-zexuan@users.noreply.github.com> |

以上信息可以写入仓库，便于新 agent 接手；访问令牌、密码、SSH 私钥和系统凭据不能写入仓库。

## 新 agent 接手步骤

在 PanView 根目录执行：

1. 阅读 AGENTS.md、README.md 与项目化学习规划书。
2. 执行 git status --branch --short，确认当前分支和工作树状态。
3. 执行 git remote -v，确认 origin 指向本项目私有仓库。
4. 工作树干净时执行 git fetch origin --prune，了解远端是否有新提交。
5. 只在当前任务对应的 feature、experiment 或 fix 分支上修改。

同一台已配置机器上，GitHub CLI 的登录状态与 Git 凭据由系统凭据库管理。新 agent 只可检查 gh auth status，不得导出、复制或记录令牌。

## 每次修改后的同步步骤

1. 检查 git diff，确认变更只覆盖当前任务。
2. 运行 git diff --check，排除行尾空格与冲突标记。
3. 按单一目的创建中文 Conventional Commit。
4. 推送当前分支：git push。
5. 再次执行 git status --branch --short，确认本地分支已跟踪远端且工作树干净。

## 并发与冲突

- 多个 agent 共享同一工作目录时，先检查工作树是否已有未提交变更；存在时不得覆盖、重置或清理它们。
- 推送被拒绝时，先 git fetch origin --prune 并阅读差异；只有确认无冲突时才使用普通合并或变基。不能使用 force push。
- 需要并行探索时，各 agent 使用不同的 experiment 或 feature 分支，避免同时改同一文件。
- 外部协作或新的机器应先克隆私有仓库；若未获得访问权限，只报告认证或权限问题，不尝试绕过。

## 当前状态

本文档记录的是项目协作基线。分支状态、远端提交和登录状态会变化，实际操作必须以 Git 命令的即时输出为准。
