## 一、设计需求总结

1. **XdcController（通信层）**
   
   - 主动查询所有连线单元（发送 `0x0601` 遍历 ID）
   
   - 接收主机返回的各种单元相关响应（`0x0601`, `0x0602`, `0x0606`, `0x0501~0x0505`, `0x020E`, `0x020F` 等）
   
   - 解析响应数据，将结果更新到 `UnitModel`（数据模型）

2. **UnitModel（数据模型层）**
   
   - 存储单元数据（ID、类型、身份、别名、状态、无线属性等）
   
   - 提供 QML 可调用的操控接口：打开/关闭单元、删除单元、设置身份、设置别名、刷新无线状态
   
   - 通过信号（`requestOpenUnit` 等）将操作请求发送给 `XdcController`，不直接处理通信
   
   - 提供数据查询接口（`getUnitInfo`、`getUnitIds` 等）和统计属性（`unitCount`、`wirelessCount` 等）

3. **XdcController 与 UnitModel 的交互**
   
   - 创建 `UnitModel` 实例（在 `XdcController` 构造函数中）
   
   - 连接 `UnitModel` 的请求信号到自己的槽函数，槽函数负责发送对应协议指令
   
   - 处理响应后调用 `UnitModel` 的更新方法（`updateUnitInfo`、`setUnitOpened` 等）

## 二、现有代码完成情况检查

### ✅ 需求1：XdcController 查询所有连线单元并生成 UnitModel

| 功能     | 实现函数                       | 说明                                                           |
| ------ | -------------------------- | ------------------------------------------------------------ |
| 公开查询接口 | `getAllUnits()`            | 设置遍历标志，清空 UnitModel，开始遍历                                     |
| 遍历逻辑   | `requestNextUnitInfo()`    | 循环发送 `0x0601`，每 10 个单元间隔 100ms，完成后发射 `unitListSyncCompleted` |
| 响应解析   | `handleUnitInfoResponse()` | 解析 `0x0601` 响应，调用 `m_unitModel->updateUnitInfo()` 添加/更新单元    |

**结论**：✅ 完整实现。

### ✅ 需求2：UnitModel 中设计单元操控功能

| 操作     | UnitModel 接口                                              | 发出的信号                         | XdcController 槽函数                                      |
| ------ | --------------------------------------------------------- | ----------------------------- | ------------------------------------------------------ |
| 打开单元   | `openUnit(unitId)`                                        | `requestOpenUnit`             | `onUnitModelRequestOpenUnit` → 发送 `0x0504`             |
| 关闭单元   | `closeUnit(unitId)`                                       | `requestCloseUnit`            | `onUnitModelRequestCloseUnit` → 发送 `0x0505`            |
| 删除单元   | `deleteUnit(unitId)`                                      | `requestDeleteUnit`           | `onUnitModelRequestDeleteUnit` → 发送 `0x0503`           |
| 设置身份   | `setUnitIdentity(unitId, identity)`                       | `requestSetIdentity`          | `onUnitModelRequestSetIdentity` → 发送 `0x0501`          |
| 设置别名   | `setUnitAlias(unitId, alias)`                             | `requestSetAlias`             | `onUnitModelRequestSetAlias` → 发送 `0x0502`             |
| 刷新无线状态 | `refreshWirelessState(unitId)`                            | `requestRefreshWirelessState` | `onUnitModelRequestRefreshWirelessState` → 发送 `0x0606` |
| 批量操作   | `openUnits`, `closeUnits`, `closeAllUnits`, `deleteUnits` | 复用上述信号                        | 同上                                                     |

**UnitModel 还提供**：

- 查询接口：`getUnitInfo`, `getUnitIds`, `getWirelessUnitIds` 等

- 统计属性：`unitCount`, `wirelessCount`, `speakingCount` 等

- 数据绑定：继承 `QAbstractListModel`，定义 `roleNames()`，供 QML 直接使用

**结论**：✅ 完整实现。

### ✅ 需求3：XdcController 接收响应、解析、更新 UnitModel

| 响应命令                | 处理函数                                    | 更新 UnitModel 的方法                                                  |
| ------------------- | --------------------------------------- | ----------------------------------------------------------------- |
| `0x0601`            | `handleUnitInfoResponse`                | `updateUnitInfo()`                                                |
| `0x0602`            | `handleUnitAliasResponse`               | `updateUnitAlias()`                                               |
| `0x0606`            | `handleWirelessStateResponse`           | `updateWirelessState()`                                           |
| `0x0504` (打开成功)     | `handleUnitOperationResponse`           | `setUnitOpened(unitId, true)`                                     |
| `0x0505` (关闭成功)     | `handleUnitOperationResponse`           | `setUnitOpened(unitId, false)` + `setUnitSpeaking(unitId, false)` |
| `0x0503` (删除成功)     | `handleUnitOperationResponse`           | `removeUnit(unitId)`                                              |
| `0x020E` / `0x020F` | `handleCurrentSpeakerIdResponse` (统一函数) | `setUnitSpeaking(unitId, true/false)`                             |

**注意**：当前代码中 `handleCurrentWirelessSpeakerIdResponse` 和 `handleCurrentWiredSpeakerIdResponse` 分别处理两种发言 ID，但已经正确更新了 `UnitModel`。用户要求统一使用 `handleCurrentSpeakerIdResponse`，这可以进一步精简代码，但现有实现功能完整。

**结论**：✅ 完整实现。

## 三、整体数据流图

QML 按钮点击
    │
    ▼
UnitModel::openUnit(unitId)
    │
    ├─ 检查单元存在
    └─ emit requestOpenUnit(unitId)
            │
            ▼ (信号连接)
XdcController::onUnitModelRequestOpenUnit(unitId)
    │
    ├─ 构建协议 payload
    └─ sendCommand(0x01, 0x0504, payload)
            │
            ▼ (通过串口/RS485)
主机执行指令
    │
    ▼
主机返回 0x0504 响应帧
    │
    ▼
XdcController::handleUnitOperationResponse()
    │
    ├─ 解析响应，确认成功
    └─ m_unitModel->setUnitOpened(unitId, true)
            │
            ├─ 更新内存数据
            └─ emit unitDataChanged(unitId)
                    │
                    ▼
                QML 界面自动刷新（按钮状态变为“关闭”）



## 四、现有代码的小优化建议（可选）

1. **删除冗余的独立发言 ID 处理函数**
   
   - 当前 `parseAndDispatch` 中仍调用 `handleCurrentWirelessSpeakerIdResponse` 和 `handleCurrentWiredSpeakerIdResponse`
   
   - 可以改为统一调用 `handleCurrentSpeakerIdResponse`，并删除那两个独立函数，减少重复代码

2. **`handleCurrentSpeakerIdResponse` 已在头文件中声明但未实现**
   
   - 需要补充实现（内容与上述两个独立函数类似，只是合并逻辑）
   
   - 或者直接修改 `parseAndDispatch` 继续使用现有两个函数，不引入新函数

3. **添加单元列表同步完成后的自动刷新发言状态**
   
   - 在 `requestNextUnitInfo` 完成后发射 `unitListSyncCompleted`，可以在此信号槽中调用 `refreshSpeakingStatus()`，自动获取发言状态

## 五、总体结论

**所有设计需求已经完整实现**：

- `XdcController` 能够主动查询单元、处理所有单元相关响应、更新 `UnitModel`

- `UnitModel` 提供了完整的数据存储、操控接口和 QML 绑定能力

- 两者通过信号槽解耦，通信逻辑清晰

当前代码可以直接使用。如需进一步精简或优化，可按上述可选建议进行小幅调整。
