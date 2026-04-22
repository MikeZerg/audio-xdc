##1. 流程分析 (Process Flow)
###A. 通信路由流程 (The Routing Flow)
- 发送指令：MeetingManager 调用 sendToMultipleHosts()。
- 硬件转发：遍历 m_meetingHosts，调用 HostManager::sendProtocolFrame() 将指令发送给各个物理主机。
- 响应接收：物理主机返回帧 -> HostManager::onFactoryDataReceived()。
- 智能路由：HostManager 识别命令码范围（0x0700-0x07FF），触发 meetingCommandResponse 信号。
- 业务处理：MeetingManager::onMeetingCommandResponse() 捕获信号，解析数据并更新 m_currentMeeting。
###B. 事件生命周期流程 (The Event Lifecycle)
以签到为例：
- 创建：调用 createCheckInEvent() -> 生成 cid，设置 isActive = true。
- 进行：单元按下签到键 -> 硬件上报 -> parseCheckInResponse() -> 存入 currentCheckIn.results。
- 重置/结束：调用 resetCheckInEvent(0) -> 触发 archiveCurrentCheckIn()。
- 归档：将当前的 currentCheckIn 拷贝到 checkInHistory 列表中，然后清空当前状态。
##2. 功能分析 (Functional Analysis)
| 功能模块 | 核心能力 | 设计亮点 |
| :--- | :--- | :--- |
| **多主机协同** | 广播指令、汇总响应 | 通过 `QList<uint8_t>` 统一管理目标主机，实现“一键多控”。 |
| **聚合数据模型** | `MeetingRecord` 结构体 | 将会议信息与所有子事件打包，解决了数据碎片化问题，便于日志保存。 |
| **历史追溯** | `History` 列表存储 | 支持在一次会议中进行多轮投票或分段签到，并能随时调出任意一轮的详细名单。 |
| **类型安全** | 统一使用 `uint8_t` | 从协议层到业务层保持数据类型一致，消除了转换开销和潜在错误。 |
| **QML 友好** | `Q_PROPERTY` + `toVariantMap` | 复杂的 C++ 嵌套结构通过 `VariantMap` 轻松映射到 QML 界面，实现了逻辑与视图的解耦。 |


##1. 组成模块 (Composition Modules)
MeetingManager 由以下几个核心逻辑块组成：
- 生命周期管理模块：负责会议的创建（快速/计划）、启动、结束以及状态的流转。
- 事件控制模块：专门处理签到和投票事件的开启、重置、时长更新以及互斥逻辑（防止签到投票冲突）。
- 多主机通信模块：维护一个关联的主机地址列表 (m_meetingHosts)，并提供批量发送指令的能力。
- 数据归档与查询模块：提供当前活跃数据的实时访问接口，以及历史事件的存储与回溯功能。
- 协议解析模块：作为“分支3”的终点，负责将硬件返回的二进制负载拆解为业务数据。

##2. 数据模块 (Data Modules)
我们采用了嵌套聚合结构体的设计，确保了数据的内聚性：
| 数据结构 | 职责描述 | 关键字段 |
| :--- | :--- | :--- |
| **`CheckInEvent`** | 签到事件单元 | `cid`, `isActive`, `results` (UnitID -> Time) |
| **`VotingEvent`** | 投票事件单元 | `vid`, `options` (选项列表), `results` (UnitID -> Option) |
| **`MeetingRecord`** | **核心聚合体** | 包含会议基本信息、`currentCheckIn`、`currentVoting` 以及两个历史记录列表。 |
| **`MeetingManager`** | 业务控制器 | 持有 `m_currentMeeting` 实例，通过 `Q_PROPERTY` 暴露给 QML。 |


##3. 数据流分析 (Data Flow Analysis)
数据在系统中的流动遵循 “下行指令广播，上行响应路由” 的模式：
A. 下行流（软件 -> 硬件）
- 触发：QML 调用 createCheckInEvent()。
- 封装：MeetingManager 生成事件 ID，设置 isActive = true。
- 分发：调用 sendToMultipleHosts(0x0701, payload)。
- 转发：遍历 m_meetingHosts，调用 HostManager::sendProtocolFrame 发送给所有物理主机。

B. 上行流（硬件 -> 软件）
- 接收：HostManager 收到原始字节流并解析出 ParsedFrameData。
- 路由（关键）：识别命令码为 0x07xx，发射 meetingCommandResponse 信号。
- 捕获：MeetingManager::onMeetingCommandResponse 接收到信号。
- 解析与更新：根据命令码调用 parseCheckInResponse，将单元 ID 存入 m_currentMeeting.currentCheckIn.results。
- 同步：发射 signal_meetingUpdated()，驱动 QML 界面刷新进度条或名单。


##4. 操作流程总结 (Operational Flow)
场景一：开启一场快速会议并签到
- 初始化：setMeetingHosts({1, 2}) 绑定两台主机。
- 建会：createQuickMeeting("周会", "讨论项目", 60) -> 状态变为 InProgress。
- 发起签到：createCheckInEvent(5) -> 向主机 1 和 2 发送 0x0701 指令。
- 单元上报：单元按下签到键 -> 主机回复 0x0702 + 单元 ID。
- 实时更新：MeetingManager 解析 ID 并更新 results Map，界面显示“已签到: 1人”。
- 结束签到：resetCheckInEvent(0) -> 触发 archiveCurrentCheckIn()，将本次签到移入 checkInHistory。

场景二：进行多轮投票
- 第一轮投票：createReferendumVoting(3) -> 开启赞成/反对投票。
- 归档：投票结束后，数据自动进入 votingHistory[0]。
- 第二轮投票：createCustomVoting(5, [...]) -> 开启自定义议题投票。
- 历史查询：QML 调用 getVotingHistoryDetail(0) 查看第一轮的详细分布。

##5. 架构特点
- 解耦性：MeetingManager 不需要知道串口怎么开、波特率是多少，它只关心“会议业务”。
- 扩展性：如果需要增加“问卷”功能，只需在 MeetingRecord 中增加一个 QuestionnaireEvent 成员，并在路由中增加对应的解析分支即可。
- 鲁棒性：通过 m_meetingHosts.contains(hostAddress) 校验，确保只有属于当前会议的主机数据才会被处理，避免了多会议室环境下的数据串扰。