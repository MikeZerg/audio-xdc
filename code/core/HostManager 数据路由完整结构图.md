* ## 多主机流程图（签到）
    1. QML 配置
    └─> meetingManager.setMeetingHosts([1, 2, 3])

    2. QML 创建签到
    └─> meetingManager.create_CheckInEvent(600)

    3. MeetingManager 构建指令
    ├─> 生成 CID = 1001
    ├─> 构建 payload (6字节)
    └─> sendToMultipleHosts(0x0702, payload)

    4. MeetingManager 批量发送（循环调用 HostManager）
    ├─> m_hostManager->sendProtocolFrame(0x01, 0x80, 0x02, 0x0702, payload)
    │   └─> ConnectionFactory → 串口发送
    ├─> m_hostManager->sendProtocolFrame(0x02, 0x80, 0x02, 0x0702, payload)
    │   └─> ConnectionFactory → 串口发送
    └─> m_hostManager->sendProtocolFrame(0x03, 0x80, 0x02, 0x0702, payload)
        └─> ConnectionFactory → 串口发送

    5. 硬件响应
    ├─> 主机 0x01 返回: 55 05 EF 01 80 04 0702 0001 AA (单元1)
    ├─> 主机 0x02 返回: 55 05 EF 02 80 04 0702 0004 AA (单元4)
    └─> 主机 0x03 返回: 55 05 EF 03 80 04 0702 0007 AA (单元7)

    6. HostManager 接收并路由
    ├─> onFactoryDataReceived() 解析帧
    ├─> 检测到 command = 0x0702 (会议命令)
    └─> emit meetingCommandResponse(0x01, 0x0702, [0x00, 0x01])
    └─> emit meetingCommandResponse(0x02, 0x0702, [0x00, 0x04])
    └─> emit meetingCommandResponse(0x03, 0x0702, [0x00, 0x07])

    7. MeetingManager 接收响应
    ├─> onMeetingCommandResponse(0x01, 0x0702, payload)
    │   └─> parseCheckInResponse() → 保存 {host=0x01, unit=1}
    ├─> onMeetingCommandResponse(0x02, 0x0702, payload)
    │   └─> parseCheckInResponse() → 保存 {host=0x02, unit=4}
    └─> onMeetingCommandResponse(0x03, 0x0702, payload)
        └─> parseCheckInResponse() → 保存 {host=0x03, unit=7}

    8. MeetingManager 更新统计
    └─> emit signal_checkInResultsChanged()
        └─> QML 显示: "已签到: 3 / 10"



* ## HostManager 数据路由完整结构图（修正版）
    onFactoryDataReceived(const QByteArray \&data)
    │
    ├─ 1. 数据缓冲与帧提取
    │   ├─ m\_receivedData.append(data)
    │   └─ ProtocolProcessor::extractFramesFromBuffer()
    │       └─ 循环处理每个完整帧
    │           │
    │           └─ ProtocolProcessor::parseFrame(frame, parsedData)
    │               │
    │               ├─ ✅ 解析成功
    │               │   ├─ updateHostLastSeen(parsedData.srcAddr)  // 更新最后通信时间
    │               │   │
    │               │   └─ 【路由分支判断】
    │               │       │
    │               │       ├─ 分支1: command == 0x0201 (探测响应) ✅ 已实现
    │               │       │   ├─ 解析设备类型 (从 payload\[0])
    │               │       │   ├─ upsertHostInfo(addr, Connected, deviceType)  // 创建/更新主机信息
    │               │       │   ├─ emit detectedHostsResponseReceived(addr)     // 探测完成信号
    │               │       │   ├─ emit hostInfoChanged(addr)                   // 主机信息变化信号
    │               │       │   └─ LOG\_INFO("收到探测响应")
    │               │       │
    │               │       ├─ 分支3: command >= 0x0700 \&\& <= 0x07FF (会议命令) ❌ 待实现 ⭐
    │               │       │   ├─ emit meetingCommandResponse(addr, command, payload)  // 转发给 MeetingManager
    │               │       │   └─ LOG\_INFO("会议命令路由到 MeetingManager")
    │               │       │       // TODO: 需要添加此分支
    │               │       │
    │               │       └─ 分支2: 其他命令 (非探测、非会议) 
    │               │           └─ forwardFrameToHostController(parsedData)  // 转发到控制器
    │               │               │
    │               │               ├─ 检查1: 主机地址是否存在？
    │               │               │   └─ getHostInfoPtr(targetAddress)
    │               │               │       ├─ ❌ 不存在 → handleUnknownAddress() + LOG\_WARNING
    │               │               │       └─ ✅ 存在 → 继续
    │               │               │
    │               │               ├─ 检查2: 控制器实例是否存在？
    │               │               │   └─ m\_controllers.value(targetAddress)
    │               │               │       ├─ ❌ 不存在 → handleUnknownAddress() + LOG\_WARNING
    │               │               │       └─ ✅ 存在 → 继续
    │               │               │
    │               │               └─ 【设备类型路由】
    │               │                   │
    │               │                   ├─ 分支2.1: deviceType == xdc236 (0x01) ✅ 已实现
    │               │                   │   ├─ qobject\_cast<XdcController\*>
    │               │                   │   │   ├─ ✅ 转换成功
    │               │                   │   │   │   ├─ xdcCtrl->handleIncomingFrame(data)  // 调用控制器处理
    │               │                   │   │   │   ├─ emit hostManagerDataReceived(addr, data)  // 通用数据信号
    │               │                   │   │   │   └─ LOG\_INFO("数据路由到 XDC 主机")
    │               │                   │   │   └─ ❌ 转换失败 → LOG\_WARNING("控制器类型转换失败")
    │               │                   │
    │               │                   ├─ 分支2.2: deviceType == DS240 (0x02) ⚠️ 预留位置
    │               │                   │   └─ LOG\_WARNING("DS240控制器尚未实现")
    │               │                   │       // TODO: 需要创建 AfhController 并调用
    │               │                   │
    │               │                   ├─ 分支2.3: deviceType == PowerAmplifier (0x03) ❌ 未实现
    │               │                   │   └─ LOG\_WARNING("PowerAmplifier控制器尚未实现")
    │               │                   │       // TODO: 未来扩展功放控制器
    │               │                   │
    │               │                   ├─ 分支2.4: deviceType == WirelessMic (0x04) ❌ 未实现
    │               │                   │   └─ LOG\_WARNING("WirelessMic控制器尚未实现")
    │               │                   │       // TODO: 未来扩展无线麦克风控制器
    │               │                   │
    │               │                   └─ 分支2.5: 未知设备类型 ⚠️ 降级处理
    │               │                       ├─ 尝试转换为 XdcController
    │               │                       │   ├─ ✅ 成功 → 调用 handleIncomingFrame() + emit signal
    │               │                       │   └─ ❌ 失败 → LOG\_WARNING("未知设备类型且无法转换")
    │               │                       └─ LOG\_WARNING("未知设备类型，使用XDC控制器")
    │               │
    │               └─ ❌ 解析失败
    │                   ├─ emit errorOccurred("协议帧解析失败")
    │                   └─ LOG\_WARNING("帧解析失败")
    │
    └─ 2. 缓冲区清理
        └─ if (m\_receivedData.size() > 10KB) → 清空缓冲区



* ## 修正后的路由分支状态总结表

| 分支编号 | 命令码/条件 | 目标模块 | 状态 | 说明 |
|---------|------------|---------|------|------|
| 分支1 | `command == 0x0201` | HostManager 内部 | 已实现 | 探测响应，创建/更新主机信息 |
| 分支3 | `0x0700 <= command <= 0x07FF` | MeetingManager | 待实现 | 会议命令路由（当前需求） |
| 分支2 | 其他命令 | Controller 路由 | 已实现 | 转发到具体设备控制器 |
| └─ 分支2.1 | `deviceType == xdc236` | XdcController | 已实现 | 会议主机，完整实现 |
| └─ 分支2.2 | `deviceType == DS240` | AfhController | 预留位置 | 跳频主机，仅日志提示 |
| └─ 分支2.3 | `deviceType == PowerAmplifier` | (未定义) | 未实现 | 功放，仅日志提示 |
| └─ 分支2.4 | `deviceType == WirelessMic` | (未定义) | 未实现 | 无线麦克风，仅日志提示 |
| └─ 分支2.5 | 未知设备类型 | XdcController (降级) | 降级处理 | 尝试用 XdcController 处理 |

## HostManager 信号路由与数据流说明

本文档详细说明了 `HostManager` 在接收到硬件原始数据后，如何根据协议命令码进行智能分流，并通过信号通知业务层（MeetingManager）、控制层（Xdc/AfhController）及视图层（QML）。

### 1. 信号流转图 (Signal Flow)

```mermaid
graph TD
    A[硬件接收原始数据] --> B{onFactoryDataReceived<br/>协议解析}
    
    B -->|解析失败/CRC错误| C[emit errorOccurred]
    
    B -->|命令 0x0201<br/>探测响应| D[更新 HostInfo 档案]
    D --> E[emit detectedHostsResponseReceived]
    D --> F[emit hostInfoChanged]
    
    B -->|命令 0x07xx<br/>会议指令| G[emit meetingCommandResponse]
    
    B -->|其他业务指令| H[forwardFrameToHostController]
    H --> I[emit hostManagerDataReceived]
    H --> J[Controller 内部处理<br/>如: handleIncomingFrame]
    J --> K[emit hostInfoChanged / 业务状态信号]


## 2. 核心信号定义与职责

### 2.1 数据分发类 (Data Distribution)

| 信号名称 | 触发条件 | 主要接收者 | 职责说明 |
| :--- | :--- | :--- | :--- |
| **`meetingCommandResponse`** | 命令码在 `0x0700-0x07FF` 范围内 | `MeetingManager` | **分支3路由**：将签到、投票等会议专用指令直接投递给业务管理器，实现软硬件解耦。 |
| **`hostManagerDataReceived`** | 非探测、非会议的普通业务指令 | UI日志/监控组件 | **广播通知**：在数据转发给 Controller 处理后触发，用于全局数据追踪或调试记录。 |

### 2.2 主机发现类 (Discovery)

| 信号名称 | 触发条件 | 主要接收者 | 职责说明 |
| :--- | :--- | :--- | :--- |
| **`detectedHostsResponseReceived`** | 收到 `0x0201` 探测响应并完成档案更新 | QML 主机列表 | **发现通知**：告知界面“发现了一个新地址”，驱动列表实时刷新。 |
| **`hostInfoChanged`** | 主机状态、最后通信时间(lastSeen)或属性变动 | QML 主机卡片 | **状态同步**：采用单一信号设计，统一处理主机从“未探测”到“已就绪”的全生命周期状态变更。 |

### 2.3 基础通信类 (Communication)

| 信号名称 | 触发条件 | 主要接收者 | 职责说明 |
| :--- | :--- | :--- | :--- |
| **`errorOccurred`** | 串口断开、校验失败、解析异常 | 全局错误处理器 | **异常上报**：提供详细的错误描述字符串，用于弹窗警告或写入错误日志。 |
| **`activeHostChanged`** | 调用 `activateHost` 切换主控权时 | MeetingManager / UI | **主控权转移**：标记当前系统正在与哪个物理主机交互，指导业务指令的目标地址选择。 |

## 3. 路由逻辑详解

1.  **优先级判断**：`HostManager` 在 `onFactoryDataReceived` 中首先检查命令码范围。
2.  **会议优先**：若识别为会议命令（0x07xx），立即发射 `meetingCommandResponse`，不再进入常规的 Controller 转发流程。这确保了即使该主机没有对应的 `XdcController` 实例，会议业务也能正常运行。
3.  **常规转发**：对于其他指令，通过 `forwardFrameToHostController` 查找对应的控制器实例进行处理，并在处理完成后发射 `hostManagerDataReceived` 作为回执。

## 4. 开发建议

*   **MeetingManager 集成**：务必在初始化时调用 `setHostManager`，以建立 `meetingCommandResponse` 的信号槽连接。
*   **UI 性能优化**：由于 `hostInfoChanged` 触发频率可能较高（如心跳更新），建议在 QML 端使用 `Throttle` 或确保绑定逻辑轻量化。
*   **调试技巧**：开启 `hostDataReceived`（需手动连接底层适配器信号）可以查看最原始的 HEX 数据，辅助排查协议解析前的通讯问题。


