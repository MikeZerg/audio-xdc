# XdcController 头文件功能分类汇总

## 一、成员变量（数据存储）

| 变量名 | 类型 | 作用 |
|-------|------|------|
| `m_hostAddress` | `uint8_t` | 存储主机地址 |
| `m_hostManager` | `HostManager*` | 主机管理器指针 |
| `m_protocolProcessor` | `ProtocolProcessor*` | 协议处理器 |
| `m_pendingResponses` | `int` | 已接收响应计数 |
| `m_expectedResponses` | `int` | 期望响应总数 |
| `m_stateValid` | `bool` | 状态有效性标记 |
| `m_hardwareInfo` | `QVariantMap` | 硬件信息存储 |
| `m_volumeSettings` | `QVariantMap` | 音量设置存储 |
| `m_speechSettings` | `QVariantMap` | 发言设置存储 |
| `m_cameraSettings` | `QVariantMap` | 摄像设置存储 |
| `m_systemDateTime` | `QDateTime` | 系统时间存储 |

---

## 二、Getter 函数（供 QML 绑定读取）- view 前缀

| 函数名 | 返回类型 | 作用 | 对应 Q_PROPERTY |
|-------|---------|------|----------------|
| `viewHardwareInfo()` | `QVariantMap` | 获取硬件信息供 QML 绑定 | `viewHardwareInfo` |
| `viewVolumeSettings()` | `QVariantMap` | 获取音量设置供 QML 绑定 | `viewVolumeSettings` |
| `viewSpeechSettings()` | `QVariantMap` | 获取发言设置供 QML 绑定 | `viewSpeechSettings` |
| `viewCameraSettings()` | `QVariantMap` | 获取摄像设置供 QML 绑定 | `viewCameraSettings` |
| `viewSystemDateTime()` | `QDateTime` | 获取系统时间供 QML 绑定 | `viewSystemDateTime` |
| `isStateValid()` | `bool` | 获取状态有效性 | `stateValid` |
| `address()` | `uint8_t` | 获取主机地址 | `address` |

---

## 三、请求函数（发送读取指令）- get 前缀

| 函数名 | 作用 | 发送的命令码 |
|-------|------|-------------|
| `getHardwareInfo()` | 批量获取硬件信息（6个属性） | 0x0202, 0x0203 |
| `getWirelessVolume()` | 获取无线音量 | 0x0205 |
| `getWiredVolume()` | 获取有线音量 | 0x0206 |
| `getAntennaVolume()` | 获取天线盒音量 | 0x0207 |
| `getSpeechMode()` | 获取发言模式 | 0x0208 |
| `getMaxSpeechCount()` | 获取最大发言数量 | 0x0209 |
| `getCurrentWirelessSpeakerId()` | 获取当前无线发言ID | 0x020E |
| `getCurrentWiredSpeakerId()` | 获取当前有线发言ID | 0x020F |
| `getWirelessAudioPath()` | 获取无线音频路径 | 0x020D |
| `getCameraProtocol()` | 获取摄像跟踪协议 | 0x020A |
| `getCameraAddress()` | 获取摄像跟踪地址 | 0x020B |
| `getCameraBaudrate()` | 获取摄像跟踪波特率 | 0x020C |
| `getSystemDateTime()` | 获取系统时间 | 0x0204 |

---

## 四、设置函数（发送设置指令）- set 前缀

| 函数名 | 参数 | 作用 | 发送的命令码 |
|-------|------|------|-------------|
| `setWirelessVolume()` | `uint8_t index` | 设置无线音量 | 0x0105 |
| `setWiredVolume()` | `uint8_t index` | 设置有线音量 | 0x0106 |
| `setAntennaVolume()` | `uint8_t index` | 设置天线盒音量 | 0x0107 |
| `setSpeechMode()` | `uint8_t mode` | 设置发言模式 | 0x0108 |
| `setMaxSpeechCount()` | `uint8_t count` | 设置最大发言数量 | 0x0109 |
| `setWirelessAudioPath()` | `uint8_t path` | 设置无线音频路径 | 0x010D |
| `setCameraProtocol()` | `uint8_t protocol` | 设置摄像跟踪协议 | 0x010A |
| `setCameraAddress()` | `uint8_t address` | 设置摄像跟踪地址 | 0x010B |
| `setCameraBaudrate()` | `uint8_t baudrate` | 设置摄像跟踪波特率 | 0x010C |
| `setSystemDateTime()` | `uint32_t utcTime` | 设置系统时间 | 0x0104 |

---

## 五、响应处理函数（handle 前缀）

| 函数名 | 处理的命令码 | 作用 |
|-------|-------------|------|
| `handleVersionResponse()` | 0x0202 | 处理版本号响应 |
| `handleBuildDateResponse()` | 0x0203 | 处理编译日期响应 |
| `handleWirelessVolumeResponse()` | 0x0205 | 处理无线音量响应 |
| `handleWiredVolumeResponse()` | 0x0206 | 处理有线音量响应 |
| `handleAntennaVolumeResponse()` | 0x0207 | 处理天线盒音量响应 |
| `handleSpeechModeResponse()` | 0x0208 | 处理发言模式响应 |
| `handleMaxSpeechCountResponse()` | 0x0209 | 处理最大发言数量响应 |
| `handleCurrentWirelessSpeakerIdResponse()` | 0x020E | 处理当前无线发言ID响应 |
| `handleCurrentWiredSpeakerIdResponse()` | 0x020F | 处理当前有线发言ID响应 |
| `handleWirelessAudioPathResponse()` | 0x020D | 处理无线音频路径响应 |
| `handleCameraProtocolResponse()` | 0x020A | 处理摄像跟踪协议响应 |
| `handleCameraAddressResponse()` | 0x020B | 处理摄像跟踪地址响应 |
| `handleCameraBaudrateResponse()` | 0x020C | 处理摄像跟踪波特率响应 |
| `handleSystemDateTimeResponse()` | 0x0204 | 处理系统时间响应 |
| `handleSetResponse()` | 0x01xx | 通用设置响应处理 |

---

## 六、信号（Signals）

| 信号名 | 参数 | 作用 | 触发时机 |
|-------|------|------|---------|
| `hardwareInfoChanged()` | 无 | 硬件信息变化 | 更新 `m_hardwareInfo` 后 |
| `volumeSettingsChanged()` | 无 | 音量设置变化 | 更新 `m_volumeSettings` 后 |
| `speechSettingsChanged()` | 无 | 发言设置变化 | 更新 `m_speechSettings` 后 |
| `cameraSettingsChanged()` | 无 | 摄像设置变化 | 更新 `m_cameraSettings` 后 |
| `systemDateTimeUpdated()` | `address, dateTime` | 系统时间更新 | 收到时间响应后 |
| `currentSpeakerIdUpdated()` | `address, type, id` | 当前发言ID更新 | 收到发言ID响应后 |
| `dataChanged()` | `address, type, name, value` | 统一数据变化 | 任何数据变化时 |
| `dataBatchUpdated()` | `address, type, data` | 批量数据更新 | 批量更新时 |
| `stateSnapshot()` | `address, snapshot` | 状态快照 | 属性加载完成时 |
| `operationResult()` | `address, operation, success, message` | 操作结果 | 设置命令响应后 |
| `commandFailed()` | `command, error` | 命令失败 | 命令执行失败时 |
| `aboutToDeactivate()` | `address` | 即将停用 | 切换前 |
| `activated()` | `address` | 已激活 | 切换后 |

---

## 七、Q_PROPERTY（供 QML 绑定）

| Q_PROPERTY | 类型 | READ 函数 | NOTIFY 信号 | 作用 |
|-----------|------|----------|-------------|------|
| `stateValid` | `bool` | `isStateValid()` | `dataChanged` | 状态有效性 |
| `address` | `uint8_t` | `address()` | `CONSTANT` | 主机地址 |
| `viewHardwareInfo` | `QVariantMap` | `viewHardwareInfo()` | `hardwareInfoChanged` | 硬件信息 |
| `viewVolumeSettings` | `QVariantMap` | `viewVolumeSettings()` | `volumeSettingsChanged` | 音量设置 |
| `viewSpeechSettings` | `QVariantMap` | `viewSpeechSettings()` | `speechSettingsChanged` | 发言设置 |
| `viewCameraSettings` | `QVariantMap` | `viewCameraSettings()` | `cameraSettingsChanged` | 摄像设置 |
| `viewSystemDateTime` | `QDateTime` | `viewSystemDateTime()` | `systemDateTimeUpdated` | 系统时间 |

---

## 八、辅助函数

| 函数名 | 作用 |
|-------|------|
| `formatAddressHex()` | 格式化地址为十六进制字符串 |
| `setupProtocolProcessor()` | 设置协议处理器 |
| `parseAndDispatch()` | 解析并分发响应数据 |
| `notifyDataChange()` | 发射统一数据变化信号 |
| `notifyBatchDataUpdate()` | 发射批量数据更新信号 |
| `notifyStateSnapshot()` | 发射状态快照信号 |
| `checkAllPropertiesLoaded()` | 检查所有属性是否加载完成 |
| `sendToHost()` | 发送原始数据到主机 |
| `sendCommand()` | 构建并发送单个指令 |
| `sendCommandQueue()` | 批量发送指令队列 |
| `createGetQueue()` | 生成批量查询指令集 |
| `createSetQueue()` | 生成批量设置指令集 |
| `exportState()` | 导出状态 |
| `importState()` | 导入状态 |
| `handleIncomingFrame()` | 处理接收到的指令帧 |

---

## 九、QML 使用示例

```qml
// 读取数据（view 前缀，自动绑定）
Text { text: controller.viewHardwareInfo.kernelVersion }
Text { text: controller.viewVolumeSettings.wirelessVolume }

// 发送读取指令（get 前缀）
Button { onClicked: controller.getHardwareInfo() }
Button { onClicked: controller.getWirelessVolume() }

// 发送设置指令（set 前缀）
Slider { onValueChanged: controller.setWirelessVolume(value) }