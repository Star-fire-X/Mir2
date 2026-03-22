# 传奇类 MMORPG 双端详细设计方案

## 1. 文档定位

**文档名称**
《传奇类 MMORPG 双端详细设计方案》

**基线来源**
基于你提供的《基于服务端 EnTT ECS + OOP 混合架构与客户端 OOP 架构的传奇类 MMORPG 双端架构设计（独立开发者版）》V1.0 进行工程化细化。

**文档目标**
把已经定稿的架构原则，收敛成可落地的模块设计、代码组织、链路顺序、保存策略、测试边界和分阶段交付基线。

**适用范围**
- 第一阶段单服可运行闭环
- 第二阶段形成可玩版本
- 第三阶段在已有稳定版本上扩展内容和做瓶颈优化

## 2. 设计目标与边界

### 2.1 P0 目标

首版必须稳定打通以下权威链路：

1. 登录
2. 选角
3. 进图
4. 移动
5. 攻击/技能
6. 怪物死亡
7. 掉落生成
8. 拾取入包
9. 下线保存
10. 重连后全量重建

### 2.2 首版成功标准

以下标准全部满足，才视为第一阶段通过：

- 单进程服务端可以稳定运行一张基础地图
- 单个 Scene 的 Tick 顺序固定且可追踪
- 玩家进入地图后能看到主角、怪物、掉落三类实体
- 移动、战斗、掉落、背包更新的协议顺序稳定
- 高风险链路不存在可稳定复现的 dup
- 断线重连采用全量重同步后，客户端状态与服务端重新对齐
- 日志、GM、协议调试、配置校验、开发者面板均可用

### 2.3 明确不做

- 多进程拆服
- 跨服玩法
- 复杂脚本平台
- 重型 MVVM / 全局事件总线
- 邮件、公会、交易、拍卖行等高一致性复杂系统
- 复杂移动预测、回滚和局部恢复

## 3. 技术落地建议

### 3.1 服务端建议栈

- 语言：C++20
- ECS：EnTT
- 网络：Asio 或 Boost.Asio
- 日志：spdlog
- 配置：JSON/TOML 任一种统一格式
- 数据库：PostgreSQL 或 MySQL，首版优先选团队最熟悉的一种
- 单元测试：GoogleTest
- 构建：CMake

### 3.2 客户端建议栈

- 语言：C++20
- 架构：传统 OOP 分层
- UI/渲染层：沿用现有引擎或自研框架，不在本文强制指定
- 配置：与服务端共享配置主键，不共享运行态逻辑

### 3.3 共享层建议

首版建议额外引入 `shared/` 层，放以下内容：

- 协议号与消息结构定义
- 错误码
- 枚举定义
- 配置主键与静态类型声明

这样可以降低双端字段漂移风险，但不共享双端业务实现。

## 4. 建议仓库结构

```text
/
  shared/
    protocol/
    types/
    ids/
  server/
    app/
    core/
    net/
    protocol/
    config/
    data/
    db/
    player/
    scene/
    ecs/
    entity/
    aoi/
    ai/
    battle/
    skill/
    buff/
    gm/
    tests/
  client/
    app/
    net/
    protocol/
    config/
    model/
    scene/
    view/
    controller/
    ui/
    debug/
    tests/
  tools/
    config_checker/
    proto_dump/
    replay/
  docs/
    superpowers/
      specs/
      plans/
```

## 5. 服务端详细设计

### 5.1 进程与线程模型

首版固定为：

- 1 个主逻辑线程
- 1 个网络 IO 线程
- 1 个 DB 异步线程
- 可选 1 个日志/后台线程

职责边界：

- 主逻辑线程：Scene、ECS Tick、战斗、AI、AOI、掉落、玩家命令落地
- 网络线程：连接、收包、解包、入主线程命令队列、发送队列
- DB 线程：加载、保存、重试
- 日志线程：可选异步刷盘，不反向影响业务

### 5.2 服务端核心对象关系

核心关系固定为：

- `Session` 表示网络连接
- `Player` 表示在线玩家门面
- `CharacterData` 表示长期数据
- `Scene` 表示地图实例
- `EntityId` 表示进入场景后的运行时实体标识
- `entt::entity` 为 ECS 内部句柄

映射关系：

- `Session -> Player`
- `Player -> CharacterId`
- `Player -> 当前 Scene`
- `Player -> 当前 EntityId`
- `Scene -> EntityId -> entt::entity`

规则：

- 长期数据不直接以 `entt::entity` 保存
- 客户端所有场景对象同步均以 `EntityId` 为主键
- `Player` 不保存一份独立的全量运行态副本

### 5.3 启动顺序

建议按以下顺序启动：

1. 初始化日志与错误码
2. 加载配置并校验
3. 初始化协议注册表
4. 初始化 DB 连接与 Repository
5. 初始化 PlayerManager / SceneManager
6. 初始化网络监听
7. 创建基础 Scene
8. 开启主循环

失败原则：

- 配置不合法禁止启动
- 数据库不可用可以进入受限模式，但禁止登录
- 协议注册失败直接退出

### 5.4 Scene 职责边界

`Scene` 只负责：

- 场景生命周期
- 命令队列
- ECS registry
- System 固定顺序调度
- 实体进场/离场
- Scene 级广播汇总

`Scene` 不负责：

- 账号与角色数据加载
- 背包/任务/邮件实现
- 直接操作数据库
- 网络解包与编码

### 5.5 ECS 组件模型

首版建议落以下组件：

- `CIdentity`：`entity_id`、实体类型
- `CSceneNode`：所属 `scene_id`
- `CPosition`：坐标、朝向
- `CMoveState`：移动状态、速度、路径目标
- `CAttr`：HP/MP/攻击/防御等战斗相关运行态
- `CCombatState`：目标、攻击锁定、受击硬直摘要
- `CSkillRuntime`：冷却、待释放请求、施法状态
- `CBuffContainer`：Buff 实例集合
- `CAoiState`：格子、可见集摘要、脏标记
- `CAiState`：怪物 AI 状态
- `CHate`：仇恨摘要
- `CDrop`：掉落实体运行态、归属、过期时间
- `CPlayerRef`：关联 `PlayerId/CharacterId`
- `CMonsterRef`：关联怪物模板
- `CDeadMark`：死亡标记
- `CCleanupMark`：待清理标记

建模规则：

- Component 仅保存高频运行态
- 复杂规则不写进 Component
- 能由配置解释的，不放入 Component 固化

### 5.6 System 固定顺序

建议主 Tick 顺序固定如下：

1. `CommandSystem`：消费来自网络/GM/内部的命令
2. `BuffSystem`：处理 Buff 到期、刷新、周期效果
3. `AiSystem`：推进怪物状态机
4. `SkillSystem`：处理施法前置校验与运行态
5. `MovementSystem`：推进坐标与移动状态
6. `BattleSystem`：伤害、死亡、收益结算
7. `DropSystem`：掉落生成、超时销毁
8. `AoiSystem`：可见集刷新、进入/离开、状态摘要
9. `ProtocolFlushSystem`：统一下发消息
10. `DirtyCollectSystem`：标记玩家脏数据和保存请求
11. `CleanupSystem`：销毁实体和回收临时资源

关键原则：

- 输入先落地
- 运行态后推进
- 广播最后统一发
- 清理放最后，避免中途打断依赖链

### 5.7 命令模型

所有外部输入必须转换成明确命令对象后进入 Scene：

- `CmdEnterScene`
- `CmdMove`
- `CmdCastSkill`
- `CmdPickDrop`
- `CmdUseItem`
- `CmdDisconnect`
- `CmdReconnect`
- `CmdGmSpawnMonster`

命令入场规则：

- 网络线程只负责解析和投递命令
- 命令合法性分两层检查
  - 协议层：字段格式、长度、账号态
  - 业务层：场景态、冷却、距离、容量、归属

### 5.8 战斗链路

首版战斗流程固定为：

1. 客户端发送普攻/技能请求
2. `Player` 转为 Scene 命令
3. `SkillSystem` 做前置校验
4. `BattleSystem` 计算命中、伤害、死亡
5. `BuffSystem` 处理附加与移除
6. `DropSystem` 生成掉落实体
7. `ProtocolFlushSystem` 输出施法、伤害、死亡、掉落消息
8. `DirtyCollectSystem` 标记经验、资源、背包脏数据

禁止行为：

- 网络层直接结算伤害
- `EntityView` 风格的表现逻辑进入服务端
- 在多个模块中各自修改同一个奖励结果

### 5.9 掉落与背包一致性

拾取流程固定顺序：

1. 检查掉落实体是否存在
2. 检查归属和距离
3. 检查背包容量和堆叠条件
4. 组装入包变更集合
5. 一次性提交内存态
6. 删除掉落实体
7. 生成 `DropDisappear` 和 `InventoryDelta`
8. 标记保存

该链路中禁止：

- 客户端先本地加背包
- 先删掉落后写背包
- 多线程并行处理同一拾取请求

### 5.10 保存与回收

保存策略采用：

- 定时保存
- 关键事件保存
- 下线保存

建议关键事件包括：

- 背包变化
- 装备变化
- 金币/元宝/经验变化
- 任务领奖
- 进入新地图前
- 断线回收前

保存实现：

1. 主线程生成 `PlayerSaveSnapshot`
2. 带版本号投递给 DB 线程
3. DB 线程落库
4. 结果回主线程
5. 成功则推进版本，失败则保留脏标记并重试

### 5.11 断线与重连

首版采用最稳妥方案：

- 断线后立即冻结操作
- `Player` 与 `Session` 解绑
- 若角色仍在 Scene 中，回收必要运行态到长期数据
- 触发保存
- 重连后重新走认证和进图
- 客户端先清空旧 Scene 和旧 Model，再接收新快照

不做的能力：

- 原地无缝恢复
- 断线期间保留操作队列
- 差量重建

## 6. 客户端详细设计

### 6.1 客户端主循环

客户端主线程固定按以下顺序执行：

1. 消费网络消息队列
2. 更新输入
3. 驱动 Controller 产生命令请求
4. 更新 Model
5. 更新 Scene
6. 更新 View
7. 更新 UI
8. 渲染
9. 更新开发者面板

网络线程只负责：

- 收包
- 解码
- 投递主线程消息

### 6.2 模块边界

- `GameApp`：启动、装配、切场景
- `NetworkManager`：连接、重连、消息入队
- `ProtocolDispatcher`：消息分发到 Model/Scene 入口
- `SceneManager`：管理当前 Scene 生命周期
- `Scene`：管理 `EntityId -> EntityView`
- `EntityView`：位置插值、动画、特效、血条、名字条
- `PlayerController`：输入转请求
- `SkillController`：技能输入与释放
- `UIManager`：界面生命周期和刷新
- `ModelRoot`：聚合客户端缓存模型
- `DebugPanel`：只读显示调试信息

### 6.3 客户端模型拆分

首版建议显式拆为：

- `PlayerModel`
- `InventoryModel`
- `SkillBarModel`
- `TargetModel`
- `QuestSummaryModel`
- `SceneStateModel`
- `NetworkStateModel`

规则：

- Model 不直接发包
- Model 不裁决权威逻辑
- UI 只读 Model，不越层改 Scene

### 6.4 Scene 与 View

`Scene` 负责：

- `EntityId -> EntityView` 映射
- 新实体创建
- 实体删除
- 快照更新
- 帧更新

`EntityView` 负责：

- 表现资源装载
- 坐标平滑
- 动作切换
- 血条和名字条
- 飘字和受击表现入口

禁止：

- UI 持有 `EntityView` 指针作为长期真值
- 网络层直接创建 `EntityView`

### 6.5 UI 与 Controller 协作

建议链路：

- 输入和按钮点击由 `Controller` 接收
- `Controller` 调 `NetworkManager::Send`
- 服务端回包经 `ProtocolDispatcher`
- `Model` 更新后通知 `Panel`
- `Panel` 做最小刷新

示例：

- 技能栏点击不直接发包，而是调用 `SkillController`
- 背包格点击不直接改 `InventoryModel` 数量，而是请求服务端

### 6.6 客户端重连

客户端重连流程：

1. 进入断线态
2. 停止接受旧场景交互
3. 清理旧 Scene 中全部实体
4. 清理主要 Model
5. 重新认证
6. 接收全量快照
7. 重建 Scene
8. 恢复 UI

## 7. 协议详细设计

### 7.1 协议原则

- 上行只表达意图
- 下行只表达权威结果
- 消息名按业务域组织
- 消息可记录、可过滤、可回放
- 支持按玩家和 `trace_id` 串链路

### 7.2 建议协议分组

- `Auth*`
- `Character*`
- `Scene*`
- `Move*`
- `Combat*`
- `Skill*`
- `Buff*`
- `Drop*`
- `Inventory*`
- `Quest*`
- `System*`
- `Gm*`

### 7.3 消息设计规范

首版建议每条消息都带或可关联：

- `msg_id`
- `player_id`
- `character_id`
- `scene_id`
- `entity_id`（如适用）
- `client_seq` / `op_seq`
- `server_time_ms`

### 7.4 幂等与去重

高风险请求必须带 `client_seq`：

- 移动请求
- 技能请求
- 拾取请求
- 使用物品请求
- 领奖请求

服务端对最近已处理请求做短期缓存：

- 键：`player_id + op_type + client_seq`
- 值：处理结果摘要

### 7.5 首版关键消息

建议至少定义以下消息：

- `C2S_LoginReq`
- `S2C_LoginRsp`
- `C2S_EnterSceneReq`
- `S2C_EnterSceneSnapshot`
- `S2C_SelfState`
- `S2C_AoiEnter`
- `S2C_AoiLeave`
- `S2C_EntityStateUpdate`
- `C2S_MoveReq`
- `S2C_MoveCorrection`
- `C2S_CastSkillReq`
- `S2C_CastAccepted`
- `S2C_CombatResult`
- `S2C_BuffDelta`
- `S2C_DeathNotice`
- `S2C_DropSpawn`
- `C2S_PickDropReq`
- `S2C_PickDropRsp`
- `S2C_InventoryDelta`
- `S2C_ErrorCode`

### 7.6 快照与差量

首版推荐：

- 进图、重连：全量快照
- 背包、任务、Buff：差量更新
- AOI：进入/离开/状态摘要三类消息

不建议首版做：

- 复杂多层订阅
- 位流压缩
- 预测回滚协议

## 8. 数据与存储设计

### 8.1 逻辑分层

- `data/`：内存中的长期业务数据结构
- `db/`：DAO / Repository / SQL 映射

### 8.2 首版推荐数据域

- 账号
- 角色
- 背包
- 装备
- 技能学习状态
- 任务摘要
- 最近在线状态

### 8.3 推荐最小表模型

- `accounts`
- `characters`
- `character_inventory`
- `character_equipment`
- `character_skills`
- `character_quests`
- `save_jobs`（可选，用于追踪异步保存）

### 8.4 版本与并发控制

首版建议每个玩家长期数据带 `version`：

- 保存时比较版本
- 只允许主线程组装快照
- DB 层不直接读 Scene/ECS

## 9. 工程化与调试设计

### 9.1 日志

服务端日志最少包含：

- `INFO`
- `WARN`
- `ERROR`
- `TRACE_GAME`

客户端日志最少包含：

- `NET`
- `PROTO`
- `SCENE`
- `UI`
- `INPUT`

### 9.2 Trace 字段

关键日志必须串上：

- `trace_id`
- `player_id`
- `scene_id`
- `entity_id`
- `client_seq`

### 9.3 GM

首版 GM 命令必须有：

- 传送
- 刷怪
- 杀怪
- 发物品
- 改金币
- 查玩家状态
- 强制保存

### 9.4 配表校验

启动阶段必须校验：

- 引用是否存在
- 数值范围是否合法
- 技能目标类型与释放距离是否合法
- 怪物掉落表是否合法
- 物品堆叠配置是否合法

### 9.5 客户端开发者面板

必须显示：

- 当前 SceneId
- Ping
- 主角坐标
- 当前目标
- 当前 AOI 实体数量
- 最近 20 条协议摘要
- 最近错误摘要

## 10. 测试与验收设计

### 10.1 服务端最小自动化测试集

- 背包增减
- 掉落拾取幂等
- 技能 MP 消耗
- Buff 到期移除
- 死亡掉落生成
- 保存失败重试
- 重连全量重建

### 10.2 客户端最小测试集

- 协议解码分发
- `EntityId -> EntityView` 创建/更新/删除
- `InventoryModel` 差量更新
- 断线后 UI 与 Scene 清理

### 10.3 联调验收脚本

至少覆盖：

1. 登录进入地图
2. 移动到怪物附近
3. 连续攻击直到死亡
4. 掉落生成
5. 成功拾取
6. 背包显示更新
7. 强制断线
8. 重连后快照一致

## 11. 分阶段交付建议

### 阶段 A：架子落地

- 目录结构
- 构建系统
- 日志
- 配置加载
- 协议注册
- 基础测试框架

### 阶段 B：角色进入世界

- 登录
- 选角
- 进图
- 主角与怪物实体创建
- AOI 首次快照

### 阶段 C：移动与 AOI

- 移动命令
- 服务端位置推进
- AOI 进入/离开
- 客户端插值与校正

### 阶段 D：战斗闭环

- 普攻/技能
- 伤害
- Buff
- 死亡
- 掉落

### 阶段 E：背包与保存

- 拾取
- 背包差量
- 定时保存
- 下线保存
- 失败重试

### 阶段 F：调试与稳态

- GM
- 协议调试
- 开发者面板
- 重连全量重建
- 关键链路自动化测试

## 12. 风险与回退策略

### 12.1 Scene 复杂度膨胀

监控信号：

- `Scene` 文件持续增长
- Scene 同时持有 DB、背包、协议编码细节

回退策略：

- 把战斗、掉落、AOI、保存标记拆回独立模块

### 12.2 Player / Entity 双写

监控信号：

- 同一个 HP/位置同时改 `Player` 和 ECS

回退策略：

- 立即确定唯一真值源
- 另一侧改成只读映射

### 12.3 客户端对象膨胀

监控信号：

- `UIManager` 和 `PlayerController` 同时持有大量跨层引用

回退策略：

- 增加 `SkillController`、`TargetModel`、`SceneStateModel`

### 12.4 为未来拆服过度抽象

监控信号：

- 当前只有一张图，也引入大量远程调用接口和虚基类

回退策略：

- 保留模块边界，删除进程边界和多余抽象

## 13. 最终执行建议

建议按以下顺序推进：

1. 先搭建共享层、日志、配置、协议、基础网络
2. 再完成登录、进图、Scene、Entity、AOI
3. 再补移动、战斗、掉落
4. 最后补背包、保存、重连、GM、调试

首版任何设计选择都必须服从以下优先级：

`闭环可运行 > 联调可定位 > 状态一致 > 易维护 > 架构美观`
