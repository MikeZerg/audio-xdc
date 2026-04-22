// DS240 跳频系统主机控制器
// 负责管理单个已检测到的 DS240 跳频主机的所有功能
// 注意：主机检测（0x0201）由 HostManager 负责，本控制器只处理已确认在线的主机实例。

#ifndef AFHCONTROLLER_H
#define AFHCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QDateTime>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QtMath>
#include "ProtocolProcessor.h"

class HostManager;

/* ==================== 枚举定义 ==================== */

// 工作模式枚举
// 对应协议命令：0x0113 (SET_WORK_MODE) / 0x0213 (GET_WORK_MODE)
enum class AfhWorkMode {
    Standard = 0x00,    // 标准模式：红外对频，无需 RF 注册
    Smart0   = 0x01,    // 手动跳频模式：需 RF 注册，可手动触发跳频
    Smart1   = 0x02     // 自动跳频模式：需 RF 注册，支持自动跳频
};
Q_DECLARE_METATYPE(AfhWorkMode)     // 注册元类型，支持 QVariant 和 QML 绑定

// 频点池类型枚举
// 对应协议命令：0x0114 (SET_FREQ_POOL) / 0x0214 (GET_FREQ_POOL)
enum class AfhFreqPool {
    Preset = 0x00,      // 预置频点池：系统内置 64 个固定频点
    Custom = 0x01       // 客制化频点池：用户自定义频点（8-64个）
};
Q_DECLARE_METATYPE(AfhFreqPool)

// 跳频灵敏度枚举
// 对应协议命令：0x0119 (SET_SENSITIVITY) / 0x0219 (GET_SENSITIVITY)
enum class AfhSensitivity {
    Low  = 0x00,        // 低灵敏度：强干扰才触发跳频
    High = 0x01         // 高灵敏度：弱干扰即触发跳频
};
Q_DECLARE_METATYPE(AfhSensitivity)

// 天线盒类型枚举
// 对应协议命令：0x0112 (SET_AP_CONFIG) / 0x0212 (GET_AP_CONFIG) - 配置类型
//             0x0207 (GET_AP_TYPE) - 获取实际硬件类型
enum class AfhApType {
    Type_500_698 = 0x00,    // 全频段：所有通道 500-698MHz
    Type_500_590 = 0x01,    // 低频段：500-590MHz
    Type_590_698 = 0x02     // 高频段：590-698MHz
};
Q_DECLARE_METATYPE(AfhApType)

// 天线盒在线状态枚举
// 对应协议命令：0x0206 (GET_AP_STATUS)
enum class AfhApStatus {
    Offline = 0x00,     // 离线
    Online  = 0x01      // 在线
};
Q_DECLARE_METATYPE(AfhApStatus)

// 端口锁相状态枚举
// 对应协议命令：0x0604 (GET_PORT_PHASELOCK)
enum class AfhPhaseLock {
    Unlocked = 0x00,    // 未锁频：无信号或信号不稳定
    Locked   = 0x01     // 已锁频：信号稳定，音频正常
};
Q_DECLARE_METATYPE(AfhPhaseLock)


/* ==================== 数据结构定义 ==================== */

// 频点信息结构体
// 存储单个频点的完整信息，用于频点池和扫描数据
struct AfhFreqInfo {
    uint8_t chId = 0;           // 频点号 (0-63)，对应协议中的频点索引
    quint32 freqKhz = 0;        // 频率值 (KHz)，来自 0x0216/0x0218 响应的 4 字节
    qint8 rssi = -127;          // RSSI 信号强度 (dBm)，-127 表示无效/未扫描
    QDateTime updateTime;       // 数据最后更新时间，用于判断数据新鲜度

    inline QString freqMhzStr() const { return QString::number(freqKhz / 1000.0, 'f', 3); } // 格式化为 MHz 字符串（如 "500.500"）
    inline QString label() const { return QString("CH%1").arg(chId + 1, 2, 10, QChar('0')); } // 生成显示标签（如 "CH01"）
    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["chId"] = chId;
        map["freqKhz"] = freqKhz;
        map["freqMhz"] = freqMhzStr();
        map["label"] = label();
        map["rssi"] = rssi;
        map["rssiPercent"] = rssiToPercent(rssi);
        map["signalLevel"] = rssiToLevel(rssi);
        map["updateTime"] = updateTime.toString("hh:mm:ss");
        map["isValid"] = (rssi > -127);
        return map;
    }
    static inline int rssiToPercent(qint8 rssi) { // RSSI 转百分比（-127→0%，-30→100%）
        if (rssi <= -127) return 0;
        if (rssi >= -30) return 100;
        return (rssi + 127) * 100 / 97;
    }
    static inline QString rssiToLevel(qint8 rssi) { // RSSI 转信号等级文字
        if (rssi <= -127) return "无效";
        if (rssi > -55) return "优秀";
        if (rssi > -70) return "良好";
        if (rssi > -80) return "一般";
        return "较差";
    }
};

// 天线盒信息结构体
// 存储单个天线盒的完整信息（静态数据）
struct AfhApInfo {
    uint8_t apId = 0;                               // 天线盒ID (0/1)
    AfhApStatus online = AfhApStatus::Offline;      // 在线状态，来自 0x0206
    AfhApType actualType = AfhApType::Type_500_698; // 实际硬件类型，来自 0x0207
    AfhApType configType = AfhApType::Type_500_698; // 配置类型，来自 0x0212，可由 0x0112 设置
    QString version;                                // 固件版本，来自 0x0208（ASCII字符串）
    QString buildDate;                              // 编译日期，来自 0x0209（ASCII字符串）

    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["apId"] = apId;
        map["online"] = static_cast<int>(online);
        map["onlineText"] = (online == AfhApStatus::Online) ? "在线" : "离线";
        map["actualType"] = static_cast<int>(actualType);
        map["actualTypeText"] = apTypeToString(actualType);
        map["configType"] = static_cast<int>(configType);
        map["configTypeText"] = apTypeToString(configType);
        map["version"] = version;
        map["buildDate"] = buildDate;
        map["typeMatched"] = (actualType == configType);
        return map;
    }
    static inline QString apTypeToString(AfhApType type) { // 天线盒类型转字符串
        switch (type) {
        case AfhApType::Type_500_698: return "500-698MHz";
        case AfhApType::Type_500_590: return "500-590MHz";
        case AfhApType::Type_590_698: return "590-698MHz";
        default: return "Unknown";
        }
    }
};

// 端口实时状态结构体
// 存储单个音频端口的实时运行状态（动态数据，周期性轮询刷新）
struct AfhPortStatus {
    uint8_t portId = 0;                             // 端口号 (0-7)
    AfhPhaseLock phaseLock = AfhPhaseLock::Unlocked; // 锁相状态，来自 0x0604
    qint8 rssi = -127;                              // RF 信号强度 (dBm)，来自 0x0605
    uint8_t rfPercent = 0;                          // RF 信号百分比，由 rssi 计算得出
    quint16 audioRssi = 0;                          // 音频幅度原始值，来自 0x0606（16位无符号）
    uint8_t afPercent = 0;                          // 音频电平百分比，由 audioRssi 计算得出
    uint8_t batteryPercent = 0;                     // 电池电量百分比，从 0x0603 的 userData 解析
    quint32 freqKhz = 0;                            // 当前工作频率 (KHz)，来自 0x0601
    uint8_t chId = 0;                               // 当前频点号，来自 0x0602
    bool online = false;                            // 综合在线状态（锁频即认为在线）
    QDateTime updateTime;                           // 数据最后更新时间

    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["portId"] = portId;
        map["portLabel"] = QString("端口 %1").arg(portId + 1);
        map["phaseLock"] = static_cast<int>(phaseLock);
        map["phaseLockText"] = (phaseLock == AfhPhaseLock::Locked) ? "已锁频" : "未锁频";
        map["online"] = online;
        map["rssi"] = rssi;
        map["rfPercent"] = rfPercent;
        map["audioRssi"] = audioRssi;
        map["afPercent"] = afPercent;
        map["batteryPercent"] = batteryPercent;
        map["freqKhz"] = freqKhz;
        map["freqMhz"] = QString::number(freqKhz / 1000.0, 'f', 3);
        map["chId"] = chId;
        map["chLabel"] = QString("CH%1").arg(chId + 1, 2, 10, QChar('0'));
        map["updateTime"] = updateTime.toString("hh:mm:ss");
        map["phaseLockColor"] = (phaseLock == AfhPhaseLock::Locked) ? "#00FF00" : "#FF0000";
        map["batteryColor"] = (batteryPercent >= 30) ? "#00FF00" :
                                  (batteryPercent >= 10) ? "#FFA500" : "#FF0000";
        return map;
    }
};

// 端口频率范围结构体
// 对应协议命令：0x011B (SET_PORT_RANGE) / 0x021B (GET_PORT_RANGE)
struct AfhPortRange {
    uint8_t portId = 0;                 // 端口号 (0-7)
    quint32 startFreqKhz = 500000;      // 起始频率 (KHz)，来自 0x021B 响应的前 4 字节
    quint32 endFreqKhz = 698000;        // 结束频率 (KHz)，来自 0x021B 响应的后 4 字节

    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["portId"] = portId;
        map["portLabel"] = QString("端口 %1").arg(portId + 1);
        map["startFreqKhz"] = startFreqKhz;
        map["startFreqMhz"] = QString::number(startFreqKhz / 1000.0, 'f', 3);
        map["endFreqKhz"] = endFreqKhz;
        map["endFreqMhz"] = QString::number(endFreqKhz / 1000.0, 'f', 3);
        map["rangeText"] = QString("%1 - %2 MHz")
                               .arg(startFreqKhz / 1000.0, 0, 'f', 3)
                               .arg(endFreqKhz / 1000.0, 0, 'f', 3);
        map["bandwidth"] = (endFreqKhz - startFreqKhz) / 1000.0;
        return map;
    }
};

// 无线报文统计结构体
// 对应协议命令：0x0401 (GET_RF_STATS)
struct AfhRfStats {
    uint8_t apId = 0;               // 天线盒ID (0/1)
    quint32 txPacketCount = 0;      // 发送数据包总数
    quint32 rxErrorCount = 0;       // 错误接收数据包数（校验失败等）
    quint32 rxOtherCount = 0;       // 其它设备发送的数据包数
    quint32 rxSelfCount = 0;        // 本设备成功接收的数据包数
    quint8 lastRssi = 0;            // 最后一次通信的 RSSI 值

    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["apId"] = apId;
        map["txPacketCount"] = QVariant::fromValue(txPacketCount);
        map["rxErrorCount"] = QVariant::fromValue(rxErrorCount);
        map["rxOtherCount"] = QVariant::fromValue(rxOtherCount);
        map["rxSelfCount"] = QVariant::fromValue(rxSelfCount);
        map["lastRssi"] = lastRssi;
        map["successRate"] = successRate();
        map["errorRate"] = errorRate();
        return map;
    }
    inline double successRate() const { // 计算成功率 = rxSelfCount / txPacketCount
        return (txPacketCount > 0) ? (100.0 * rxSelfCount / txPacketCount) : 0.0;
    }
    inline double errorRate() const { // 计算错误率 = rxErrorCount / txPacketCount
        return (txPacketCount > 0) ? (100.0 * rxErrorCount / txPacketCount) : 0.0;
    }
};


/* ==================== 数据容器结构体 ==================== */

// 1. 主机基础信息容器（静态数据，初始化时一次性加载）
// 对应协议命令：0x0202, 0x0203, 0x0204, 0x0205
struct HostBaseInfo {
    uint8_t address = 0;            // 主机地址（从机地址，0x01-0x08）
    QString version;                // 固件版本（0x0202 GET_VERSION）
    QString buildDate;              // 编译日期（0x0203 GET_BUILD_DATE）
    uint8_t portCount = 0;          // 端口数量（0x0204 GET_PORT_COUNT）
    uint8_t apCount = 0;            // 天线盒数量（0x0205 GET_AP_COUNT）

    inline void clear() { version.clear(); buildDate.clear(); portCount = 0; apCount = 0; } // 清空所有数据
    inline bool isValid() const { return address != 0 && portCount > 0; } // 检查数据是否有效
    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["address"] = address;
        map["addressHex"] = QString("0x%1").arg(address, 2, 16, QChar('0')).toUpper();
        map["version"] = version;
        map["buildDate"] = buildDate;
        map["portCount"] = portCount;
        map["apCount"] = apCount;
        return map;
    }
};

// 2. 系统配置容器（可读写配置参数）
// 对应协议命令：0x0110/0x0210, 0x0113/0x0213, 0x0114/0x0214, 0x0119/0x0219
struct SystemConfig {
    AfhWorkMode workMode = AfhWorkMode::Smart0;         // 工作模式（0x0113/0x0213）
    AfhFreqPool freqPool = AfhFreqPool::Preset;         // 频点池类型（0x0114/0x0214）
    AfhSensitivity sensitivity = AfhSensitivity::High;  // 跳频灵敏度（0x0119/0x0219）
    uint8_t volumeIndex = 6;                            // 当前音量索引（0x0110/0x0210）
    uint8_t maxVolume = 9;                              // 最大音量档位（从 0x0210 响应获取）

    inline void clear() { // 清空所有数据，恢复默认值
        workMode = AfhWorkMode::Smart0;
        freqPool = AfhFreqPool::Preset;
        sensitivity = AfhSensitivity::High;
        volumeIndex = 6;
        maxVolume = 9;
    }
    inline QVariantMap toVariantMap() const { // 转换为 QVariantMap 供 QML 使用
        QVariantMap map;
        map["workMode"] = static_cast<int>(workMode);
        map["workModeText"] = (workMode == AfhWorkMode::Standard) ? "标准模式" :
                                  (workMode == AfhWorkMode::Smart0) ? "手动跳频" : "自动跳频";
        map["freqPool"] = static_cast<int>(freqPool);
        map["freqPoolText"] = (freqPool == AfhFreqPool::Preset) ? "预置频点池" : "客制化频点池";
        map["sensitivity"] = static_cast<int>(sensitivity);
        map["sensitivityText"] = (sensitivity == AfhSensitivity::High) ? "高" : "低";
        map["volumeIndex"] = volumeIndex;
        map["maxVolume"] = maxVolume;
        map["volumePercent"] = (maxVolume > 0) ? (volumeIndex * 100 / maxVolume) : 0;
        return map;
    }
};

// 3. 天线盒信息表（静态数据，初始化时加载，热插拔时更新）
// 对应协议命令：0x0206, 0x0207, 0x0208, 0x0209, 0x0112/0x0212
struct ApInfoTable {
    QMap<uint8_t, AfhApInfo> apMap;     // apId -> ApInfo 映射表

    inline void clear() { apMap.clear(); } // 清空所有天线盒信息
    inline void updateApInfo(const AfhApInfo& info) { apMap[info.apId] = info; } // 更新指定天线盒信息
    inline bool isOnline(uint8_t apId) const { // 检查天线盒是否在线（0x0206）
        auto it = apMap.find(apId);
        return (it != apMap.end()) ? (it->online == AfhApStatus::Online) : false;
    }
    inline AfhApType getActualType(uint8_t apId) const { // 获取实际硬件类型（0x0207）
        auto it = apMap.find(apId);
        return (it != apMap.end()) ? it->actualType : AfhApType::Type_500_698;
    }
    inline AfhApType getConfigType(uint8_t apId) const { // 获取配置类型（0x0212）
        auto it = apMap.find(apId);
        return (it != apMap.end()) ? it->configType : AfhApType::Type_500_698;
    }
    inline QString getVersion(uint8_t apId) const { // 获取固件版本（0x0208）
        auto it = apMap.find(apId);
        return (it != apMap.end()) ? it->version : QString();
    }
    inline int getOnlineCount() const { // 获取在线天线盒数量
        int count = 0;
        for (const auto& info : apMap) {
            if (info.online == AfhApStatus::Online) count++;
        }
        return count;
    }
    inline QVariantList toVariantList() const { // 转换为 QVariantList 供 QML 使用
        QVariantList list;
        for (auto it = apMap.constBegin(); it != apMap.constEnd(); ++it) {
            list.append(it->toVariantMap());
        }
        return list;
    }
};

// 4. 频点池容器（静态数据，初始化时加载，用户修改客制化时更新）
// 对应协议命令：0x0215, 0x0216, 0x0117/0x0217, 0x0118/0x0218
struct FreqPoolTable {
    QVector<AfhFreqInfo> presetPool;    // 预置频点池（64个），来自 0x0215 + 0x0216
    QVector<AfhFreqInfo> customPool;    // 客制化频点池（8-64个），来自 0x0217 + 0x0218
    AfhFreqPool activePool = AfhFreqPool::Preset;   // 当前使用的频点池

    inline void clear() { presetPool.clear(); customPool.clear(); activePool = AfhFreqPool::Preset; } // 清空所有频点数据
    inline void initPresetPool(int count) { // 初始化预置频点池（0x0215 响应）
        presetPool.resize(count);
        for (int i = 0; i < count; ++i) {
            presetPool[i].chId = static_cast<uint8_t>(i);
        }
    }
    inline void initCustomPool(int count) { // 初始化客制化频点池（0x0217 响应）
        customPool.resize(count);
        for (int i = 0; i < count; ++i) {
            customPool[i].chId = static_cast<uint8_t>(i);
        }
    }
    inline void updatePresetFreq(uint8_t chId, quint32 freqKhz) { // 更新预置频点值（0x0216 响应）
        if (chId < static_cast<uint8_t>(presetPool.size())) {
            presetPool[chId].freqKhz = freqKhz;
            presetPool[chId].updateTime = QDateTime::currentDateTime();
        }
    }
    inline void updateCustomFreq(uint8_t chId, quint32 freqKhz) { // 更新客制化频点值（0x0218 响应）
        if (chId < static_cast<uint8_t>(customPool.size())) {
            customPool[chId].freqKhz = freqKhz;
            customPool[chId].updateTime = QDateTime::currentDateTime();
        }
    }
    inline const QVector<AfhFreqInfo>& getActivePool() const { // 获取当前激活的频点池
        return (activePool == AfhFreqPool::Preset) ? presetPool : customPool;
    }
    inline int getActivePoolSize() const { return getActivePool().size(); } // 获取当前频点池大小
    inline AfhFreqInfo getFreqInfo(uint8_t chId) const { // 获取指定频点信息
        const QVector<AfhFreqInfo>& pool = getActivePool();
        if (chId < static_cast<uint8_t>(pool.size())) return pool[chId];
        return AfhFreqInfo();
    }
    inline QVariantList getPresetList() const { // 获取预置频点列表供 QML 使用
        QVariantList list;
        for (const auto& info : presetPool) list.append(info.toVariantMap());
        return list;
    }
    inline QVariantList getCustomList() const { // 获取客制化频点列表供 QML 使用
        QVariantList list;
        for (const auto& info : customPool) list.append(info.toVariantMap());
        return list;
    }
};

// 5. 端口配置表（静态配置，初始化时加载，用户修改时更新）
// 对应协议命令：0x011A/0x021A, 0x011B/0x021B
struct PortConfigTable {
    QMap<uint8_t, AfhPortRange> rangeMap;   // portId -> 频率范围（0x011B/0x021B）
    QMap<uint8_t, uint8_t> freqIdMap;       // portId -> chId（0x011A/0x021A）

    inline void clear() { rangeMap.clear(); freqIdMap.clear(); } // 清空所有配置
    inline void initPorts(int portCount) { // 初始化端口配置
        rangeMap.clear();
        freqIdMap.clear();
        for (int i = 0; i < portCount; ++i) {
            AfhPortRange range;
            range.portId = static_cast<uint8_t>(i);
            rangeMap[static_cast<uint8_t>(i)] = range;
            freqIdMap[static_cast<uint8_t>(i)] = static_cast<uint8_t>(i * 8);
        }
    }
    inline void updateRange(uint8_t portId, quint32 startFreq, quint32 endFreq) { // 更新频率范围
        AfhPortRange range;
        range.portId = portId;
        range.startFreqKhz = startFreq;
        range.endFreqKhz = endFreq;
        rangeMap[portId] = range;
    }
    inline void updateFreqId(uint8_t portId, uint8_t chId) { freqIdMap[portId] = chId; } // 更新频点ID
    inline AfhPortRange getRange(uint8_t portId) const { // 获取指定端口频率范围
        auto it = rangeMap.find(portId);
        return (it != rangeMap.end()) ? it.value() : AfhPortRange();
    }
    inline uint8_t getFreqId(uint8_t portId) const { // 获取指定端口频点ID
        auto it = freqIdMap.find(portId);
        return (it != freqIdMap.end()) ? it.value() : 0;
    }
    inline quint32 getFreqKhz(uint8_t portId, const FreqPoolTable& pool) const { // 获取指定端口频率值
        uint8_t chId = getFreqId(portId);
        return pool.getFreqInfo(chId).freqKhz;
    }
    inline QVariantList getRangeList() const { // 获取范围列表供 QML 使用
        QVariantList list;
        for (auto it = rangeMap.constBegin(); it != rangeMap.constEnd(); ++it) {
            list.append(it->toVariantMap());
        }
        return list;
    }
    inline QVariantList getFreqIdList() const { // 获取频点ID列表供 QML 使用
        QVariantList list;
        for (auto it = freqIdMap.constBegin(); it != freqIdMap.constEnd(); ++it) {
            QVariantMap map;
            map["portId"] = it.key();
            map["chId"] = it.value();
            list.append(map);
        }
        return list;
    }
};

// 6. 端口状态表（动态数据，周期性轮询刷新）
// 对应协议命令：0x0601, 0x0603, 0x0604, 0x0605, 0x0606
struct PortStatusTable {
    QMap<uint8_t, AfhPortStatus> statusMap; // portId -> 实时状态

    inline void clear() { statusMap.clear(); } // 清空所有状态
    inline void initPorts(int portCount) { // 初始化端口状态
        statusMap.clear();
        for (int i = 0; i < portCount; ++i) {
            AfhPortStatus status;
            status.portId = static_cast<uint8_t>(i);
            statusMap[static_cast<uint8_t>(i)] = status;
        }
    }
    inline void updatePhaseLock(uint8_t portId, bool locked) { // 更新锁相状态（0x0604）
        auto it = statusMap.find(portId);
        if (it != statusMap.end()) {
            it->phaseLock = locked ? AfhPhaseLock::Locked : AfhPhaseLock::Unlocked;
            it->online = locked;
            it->updateTime = QDateTime::currentDateTime();
        }
    }
    inline void updateRssi(uint8_t portId, qint8 rssi) { // 更新 RSSI（0x0605）
        auto it = statusMap.find(portId);
        if (it != statusMap.end()) {
            it->rssi = rssi;
            it->rfPercent = AfhFreqInfo::rssiToPercent(rssi);
            it->updateTime = QDateTime::currentDateTime();
        }
    }
    inline void updateAudioRssi(uint8_t portId, quint16 audioRssi) { // 更新音频幅度（0x0606）
        auto it = statusMap.find(portId);
        if (it != statusMap.end()) {
            it->audioRssi = audioRssi;
            it->afPercent = static_cast<uint8_t>(qMin(100.0, qSqrt(static_cast<double>(audioRssi)) * 2.0));
            it->updateTime = QDateTime::currentDateTime();
        }
    }
    inline void updateUserData(uint8_t portId, uint8_t userData, uint8_t batteryPercent) { // 更新 UserData 和电量（0x0603）
        Q_UNUSED(userData)
        auto it = statusMap.find(portId);
        if (it != statusMap.end()) {
            it->batteryPercent = batteryPercent;
            it->updateTime = QDateTime::currentDateTime();
        }
    }
    inline void updateFreq(uint8_t portId, quint32 freqKhz, uint8_t chId) { // 更新频率和频点号（0x0601/0x0602）
        auto it = statusMap.find(portId);
        if (it != statusMap.end()) {
            it->freqKhz = freqKhz;
            it->chId = chId;
            it->updateTime = QDateTime::currentDateTime();
        }
    }
    inline bool isPhaseLocked(uint8_t portId) const { // 检查是否锁频
        auto it = statusMap.find(portId);
        return (it != statusMap.end()) ? (it->phaseLock == AfhPhaseLock::Locked) : false;
    }
    inline bool isOnline(uint8_t portId) const { return isPhaseLocked(portId); } // 检查是否在线（锁频即在线）
    inline qint8 getRssi(uint8_t portId) const { // 获取 RSSI
        auto it = statusMap.find(portId);
        return (it != statusMap.end()) ? it->rssi : -127;
    }
    inline uint8_t getRfPercent(uint8_t portId) const { // 获取 RF 百分比
        auto it = statusMap.find(portId);
        return (it != statusMap.end()) ? it->rfPercent : 0;
    }
    inline uint8_t getAfPercent(uint8_t portId) const { // 获取音频百分比
        auto it = statusMap.find(portId);
        return (it != statusMap.end()) ? it->afPercent : 0;
    }
    inline uint8_t getBatteryPercent(uint8_t portId) const { // 获取电量百分比
        auto it = statusMap.find(portId);
        return (it != statusMap.end()) ? it->batteryPercent : 0;
    }
    inline QVariantList toVariantList() const { // 转换为 QVariantList 供 QML 使用
        QVariantList list;
        for (auto it = statusMap.constBegin(); it != statusMap.constEnd(); ++it) {
            list.append(it->toVariantMap());
        }
        return list;
    }
};

// 7. 扫描数据缓存（动态数据，扫描时周期性刷新）
// 扫描数据通过读取频点池并获取对应 RSSI 来填充
struct ScanDataCache {
    QVector<AfhFreqInfo> rssiCache;     // chId -> RSSI 缓存
    uint8_t apId = 0;                   // 当前扫描的天线盒
    int progress = 0;                   // 扫描进度 0-100
    int currentChId = 0;                // 当前扫描的频点号
    bool isScanning = false;            // 是否正在扫描

    inline void clear() { rssiCache.clear(); apId = 0; progress = 0; currentChId = 0; isScanning = false; } // 清空缓存
    inline void initCache(int chCount) { // 初始化缓存大小
        rssiCache.resize(chCount);
        for (int i = 0; i < chCount; ++i) {
            rssiCache[i].chId = static_cast<uint8_t>(i);
            rssiCache[i].rssi = -127;
        }
    }
    inline void updateRssi(uint8_t chId, qint8 rssi) { // 更新指定频点的 RSSI
        if (chId < static_cast<uint8_t>(rssiCache.size())) {
            rssiCache[chId].rssi = rssi;
            rssiCache[chId].updateTime = QDateTime::currentDateTime();
        }
    }
    inline qint8 getRssi(uint8_t chId) const { // 获取指定频点的 RSSI
        if (chId < static_cast<uint8_t>(rssiCache.size())) return rssiCache[chId].rssi;
        return -127;
    }
    inline void setProgress(int current, int total) { // 设置扫描进度
        if (total > 0) progress = (current * 100) / total;
        currentChId = current;
    }
    inline QVariantList toVariantList() const { // 转换为 QVariantList 供 QML 使用
        QVariantList list;
        for (const auto& info : rssiCache) list.append(info.toVariantMap());
        return list;
    }
};

// 8. 无线报文统计表（动态数据，按需查询）
// 对应协议命令：0x0401 (GET_RF_STATS)
struct RfStatsTable {
    QMap<uint8_t, AfhRfStats> statsMap; // apId -> 统计信息

    inline void clear() { statsMap.clear(); } // 清空统计信息
    inline void updateStats(const AfhRfStats& stats) { statsMap[stats.apId] = stats; } // 更新指定天线盒的统计信息
    inline AfhRfStats getStats(uint8_t apId) const { // 获取指定天线盒的统计信息
        auto it = statsMap.find(apId);
        return (it != statsMap.end()) ? it.value() : AfhRfStats();
    }
    inline double getSuccessRate(uint8_t apId) const { // 获取通信成功率
        auto it = statsMap.find(apId);
        return (it != statsMap.end()) ? it->successRate() : 0.0;
    }
    inline QVariantMap getStatsMap(uint8_t apId) const { // 获取统计信息 Map 供 QML 使用
        auto it = statsMap.find(apId);
        return (it != statsMap.end()) ? it->toVariantMap() : AfhRfStats().toVariantMap();
    }
};


/* ==================== AfhController 类 ==================== */

class AfhController : public QObject
{
    Q_OBJECT

    // ==================== 基础属性（QML 可绑定）====================
    Q_PROPERTY(uint8_t address READ address CONSTANT FINAL)              // 主机地址（只读）
    Q_PROPERTY(QString addressHex READ addressHex CONSTANT)              // 主机地址十六进制字符串
    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY initializedChanged)  // 是否已完成初始化
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY scanningChanged)   // 是否正在扫描
    Q_PROPERTY(int initProgress READ initProgress NOTIFY initProgressChanged) // 初始化进度 0-100

    // ==================== 主机信息 ====================
    // 包含：版本号、编译日期、端口数量、天线盒数量
    Q_PROPERTY(QVariantMap viewHostInfo READ viewHostInfo NOTIFY hostInfoChanged)

    // ==================== 天线盒信息 ====================
    // 包含所有天线盒的在线状态、类型、版本等
    Q_PROPERTY(QVariantList viewApInfoList READ viewApInfoList NOTIFY apInfoChanged)

    // ==================== 系统配置 ====================
    // 包含：工作模式、频点池类型、灵敏度、音量等
    Q_PROPERTY(QVariantMap viewSystemConfig READ viewSystemConfig NOTIFY systemConfigChanged)

    // ==================== 频点池数据 ====================
    Q_PROPERTY(QVariantList viewPresetFreqList READ viewPresetFreqList NOTIFY freqPoolChanged)   // 预置频点池列表
    Q_PROPERTY(QVariantList viewCustomFreqList READ viewCustomFreqList NOTIFY freqPoolChanged)   // 客制化频点池列表
    Q_PROPERTY(int presetFreqCount READ presetFreqCount NOTIFY freqPoolChanged)                  // 预置频点数量
    Q_PROPERTY(int customFreqCount READ customFreqCount NOTIFY freqPoolChanged)                  // 客制化频点数量

    // ==================== 端口状态 ====================
    // 包含所有端口（8个）的锁相状态、RSSI、音频电平、电量等
    Q_PROPERTY(QVariantList viewPortStatusList READ viewPortStatusList NOTIFY portStatusChanged)

    // ==================== 端口配置 ====================
    Q_PROPERTY(QVariantList viewPortRangeList READ viewPortRangeList NOTIFY portRangeChanged)    // 端口频率范围配置
    Q_PROPERTY(QVariantList viewPortFreqList READ viewPortFreqList NOTIFY portFreqChanged)       // 端口当前频点分配

    // ==================== 扫描数据 ====================
    Q_PROPERTY(QVariantList viewScanDataList READ viewScanDataList NOTIFY scanDataChanged)       // 扫描 RSSI 数据列表
    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)                    // 扫描进度 0-100
    Q_PROPERTY(int scanApId READ scanApId NOTIFY scanApIdChanged)                                // 当前扫描的天线盒 ID
    Q_PROPERTY(QString scanApName READ scanApName NOTIFY scanApIdChanged)                        // 当前扫描的天线盒名称

public:
    // 构造函数
    // @param hostAddress: 主机地址（从机地址，0x01-0x08），已由 HostManager 探测确认在线
    // @param parent: 父对象（HostManager）
    explicit AfhController(uint8_t hostAddress, HostManager* parent);
    virtual ~AfhController();

    // ==================== 状态导出/导入 ====================
    QVariantMap exportState() const;        // 导出当前控制器状态（用于切换主机时保存/恢复）
    void importState(const QVariantMap& state);  // 导入之前保存的状态

    // ==================== 辅助函数 ====================
    QString formatAddressHex(uint8_t addr) const;   // 格式化地址为十六进制字符串（如 "0x01"）
    QString addressHex() const;                     // 获取本机地址十六进制字符串
    uint8_t address() const;                        // 获取本机地址
    bool isInitialized() const;                     // 获取初始化状态
    bool isScanning() const;                        // 获取扫描状态
    int initProgress() const;                       // 获取初始化进度
    QString trimNullTerminator(const QByteArray& data) const;  // 去除字节数组末尾的空字符

    // ==================== 发送指令 ====================
    void sendToHost(const QByteArray& data);        // 直接发送原始数据到主机（通过 HostManager）

    // 构建并发送单个协议指令
    // @param opCode: 操作码（0x01=请求，0x02=通知）
    // @param command: 命令码
    // @param payload: 负载数据
    // @param userData: 用户自定义数据（用于响应匹配）
    Q_INVOKABLE void sendCommand(uint8_t opCode, uint16_t command,
                                 const QByteArray& payload = QByteArray(),
                                 const QVariant& userData = QVariant());

    Q_INVOKABLE void sendCommandQueue(const QVariantList& commands);    // 批量发送指令队列
    Q_INVOKABLE QVariantList buildCommandQueue(const QVariantList& commands); // 构建批量获取指令队列

    // ==================== 接收指令分发 ====================
    void handleIncomingFrame(const ProtocolProcessor::ParsedFrameData& frame);  // 处理从 HostManager 转发来的接收帧

    // ==================== Getter（供 QML 属性绑定）====================
    QVariantMap viewHostInfo() const;           // 获取主机信息 Map
    QVariantList viewApInfoList() const;        // 获取天线盒信息列表
    QVariantMap viewSystemConfig() const;       // 获取系统配置 Map
    QVariantList viewPresetFreqList() const;    // 获取预置频点列表
    QVariantList viewCustomFreqList() const;    // 获取客制化频点列表
    int presetFreqCount() const;                // 获取预置频点数量
    int customFreqCount() const;                // 获取客制化频点数量
    QVariantList viewPortStatusList() const;    // 获取端口状态列表
    QVariantList viewPortRangeList() const;     // 获取端口范围列表
    QVariantList viewPortFreqList() const;      // 获取端口频点列表
    QVariantList viewScanDataList() const;      // 获取扫描数据列表
    int scanProgress() const;                   // 获取扫描进度
    int scanApId() const;                       // 获取扫描天线盒 ID
    QString scanApName() const;                 // 获取扫描天线盒名称

    // ==================== 分组1：初始化与主机信息 ====================
    // 对应协议命令：0x0202 (GET_VERSION), 0x0203 (GET_BUILD_DATE)
    //             0x0204 (GET_PORT_COUNT), 0x0205 (GET_AP_COUNT)

    Q_INVOKABLE void initialize();              // 初始化主机（启动初始化序列）
    Q_INVOKABLE void fetchHostHardwareInfo();   // 批量获取主机硬件信息（4个请求）

    // ==================== 分组2：天线盒信息 ====================
    // 对应协议命令：0x0206 (GET_AP_STATUS), 0x0207 (GET_AP_TYPE)
    //             0x0208 (GET_AP_VERSION), 0x0209 (GET_AP_DATE)
    //             0x0112 (SET_AP_CONFIG), 0x0212 (GET_AP_CONFIG)

    Q_INVOKABLE void fetchApInfo(uint8_t apId = 0xFF);    // 批量获取天线盒信息（0xFF=全部）
    Q_INVOKABLE void setApConfigType(uint8_t apId, uint8_t type);  // 设置天线盒配置类型（0x0112）

    // ==================== 分组3：系统配置 ====================
    // 对应协议命令：
    // 工作模式：0x0113/0x0213，频点池：0x0114/0x0214
    // 灵敏度：0x0119/0x0219，音量：0x0110/0x0210

    Q_INVOKABLE void fetchSystemConfig();       // 批量获取系统配置（5个请求）
    Q_INVOKABLE void setWorkMode(uint8_t mode);     // 设置工作模式（0x0113）
    Q_INVOKABLE void setFreqPool(uint8_t pool);     // 设置频点池类型（0x0114）
    Q_INVOKABLE void setSensitivity(uint8_t sens);  // 设置跳频灵敏度（0x0119）
    Q_INVOKABLE void setVolume(uint8_t index);      // 设置无线音量（0x0110）
    Q_INVOKABLE void getWorkMode();             // 单独获取工作模式（0x0213）
    Q_INVOKABLE void getFreqPool();             // 单独获取频点池类型（0x0214）
    Q_INVOKABLE void getSensitivity();          // 单独获取跳频灵敏度（0x0219）
    Q_INVOKABLE void getVolume();               // 单独获取无线音量（0x0210）
    Q_INVOKABLE void getApConfigType(uint8_t apId); // 单独获取天线盒配置类型（0x0212）

    // ==================== 分组4：频点池管理 ====================
    // 对应协议命令：
    // 预置频点：0x0215 (GET_PRESET_NS), 0x0216 (GET_PRESET_FREQ)
    // 客制化频点：0x0117/0x0217, 0x0118/0x0218

    Q_INVOKABLE void loadActiveFreqPool();          // 根据当前配置加载对应频点池
    Q_INVOKABLE void setCustomFreqCount(uint8_t count);         // 设置客制化频点数量（0x0117）
    Q_INVOKABLE void setCustomFreq(uint8_t chId, quint32 freqKhz);  // 设置客制化频点值（0x0118）
    Q_INVOKABLE void getCustomFreqCount();                      // 获取客制化频点数量（0x0217）
    Q_INVOKABLE void getCustomFreq(uint8_t chId);               // 获取客制化频点值（0x0218）

    // ==================== 分组5：端口配置 ====================
    // 对应协议命令：
    // 端口频点：0x011A/0x021A，频率范围：0x011B/0x021B

    Q_INVOKABLE void fetchAllPortConfig();      // 批量获取所有端口配置
    Q_INVOKABLE void setPortFreq(uint8_t portId, uint8_t chId);   // 设置端口频点（0x011A）
    Q_INVOKABLE void setPortRange(uint8_t portId, quint32 startFreq, quint32 endFreq); // 设置端口频率范围（0x011B）
    Q_INVOKABLE void getPortFreq(uint8_t portId);   // 获取端口频点（0x021A）
    Q_INVOKABLE void getPortRange(uint8_t portId);  // 获取端口频率范围（0x021B）

    // ==================== 分组6：端口状态 ====================
    // 对应协议命令：
    // 0x0601 (GET_PORT_FREQ), 0x0603 (GET_PORT_USERDATA)
    // 0x0604 (GET_PORT_PHASELOCK), 0x0605 (GET_PORT_RSSI), 0x0606 (GET_PORT_AUDIO_RSSI)

    Q_INVOKABLE void setStatusPolling(bool enabled, int intervalMs = 500); // 开启/关闭状态轮询
    Q_INVOKABLE void refreshPortStatus(uint8_t portId = 0xFF);  // 手动刷新端口状态（0xFF=全部）

    // ==================== 分组7：频谱扫描 ====================
    // 主机内部通过天线盒的扫描通道持续轮询各频点 RSSI
    // 上位机通过定时读取频点频率和 RSSI 数据

    Q_INVOKABLE void startScan(uint8_t apId, int intervalMs = 500);   // 开始频谱扫描
    Q_INVOKABLE void stopScan();                    // 停止频谱扫描
    Q_INVOKABLE void scanOnce(uint8_t apId);        // 单次扫描（立即执行一次）
    Q_INVOKABLE void refreshScanProgress();         // 刷新扫描进度

    // ==================== 分组8：统计与高级功能 ====================
    // 对应协议命令：0x0401 (GET_RF_STATS)

    Q_INVOKABLE void getRfStats(uint8_t apId);      // 获取无线报文统计（0x0401）
    Q_INVOKABLE void autoAssignFrequencies();       // 一键自动分配频点（基于扫描结果）

signals:
    // ==================== 操作反馈信号 ====================
    void operationResult(uint8_t address, const QString& operation,
                         bool success, const QString& message);  // 单个操作完成时发射
    void commandFailed(uint16_t command, const QString& error);  // 指令执行失败时发射

    // ==================== 状态变化信号 ====================
    void initializedChanged();      // 初始化完成状态变化
    void initProgressChanged();     // 初始化进度变化

    // ==================== 数据变化信号（QML 绑定）====================
    void hostInfoChanged();         // 主机信息变更（版本、日期、端口数等）
    void apInfoChanged();           // 天线盒信息变更（在线状态、类型等）
    void systemConfigChanged();     // 系统配置变更（模式、频点池、灵敏度等）
    void freqPoolChanged();         // 频点池数据变更（预置/客制化）
    void portStatusChanged();       // 端口状态变更（锁相、RSSI、电量等）
    void portRangeChanged();        // 端口频率范围变更
    void portFreqChanged();         // 端口频点分配变更

    // ==================== 扫描相关信号 ====================
    void scanDataChanged();         // 扫描 RSSI 数据更新
    void scanProgressChanged();     // 扫描进度更新
    void scanApIdChanged();         // 扫描天线盒切换
    void scanningChanged();         // 扫描状态变化（开始/停止）
    void scanCompleted(uint8_t apId, bool success, const QString& message);  // 扫描完成

    // ==================== 主动上报信号 ====================
    // 当端口状态发生显著变化时主动发射（供 QML 做动画/提示）
    void portPhaseLockChanged(uint8_t portId, bool locked);      // 锁相状态变化（0x0604）
    void portRssiChanged(uint8_t portId, qint8 rssi);            // RSSI 变化（0x0605）
    void portAudioLevelChanged(uint8_t portId, uint8_t percent); // 音频电平变化（0x0606）
    void portBatteryChanged(uint8_t portId, uint8_t percent);    // 电量变化（0x0603）
    void apOnlineChanged(uint8_t apId, bool online);             // AP 在线状态变化（0x0206）

    // ==================== 统计信号 ====================
    void rfStatsReceived(uint8_t apId, const AfhRfStats& stats); // 无线报文统计接收完成（0x0401）

private slots:
    // ==================== 协议处理器回调 ====================
    void onCommandResponseCompleted(const ProtocolProcessor::ResponseResult& result);  // 请求-响应完成
    void onCommandFailed(const ProtocolProcessor::PendingRequest& request, const QString& error); // 请求失败
    void onUnsolicitedNotifyReceived(const QByteArray& rawFrame,
                                     const ProtocolProcessor::ParsedFrameData& parsedData); // 主动上报（opCode=0x02）

    // ==================== 定时器回调 ====================
    void onStatusPollTimer();       // 状态轮询定时器（发送 0x0601/0x0603/0x0604/0x0605/0x0606）
    void onScanTimer();             // 扫描定时器（发送 0x0216/0x0218）
    void onInitStepTimer();         // 初始化步骤定时器

private:
    // ==================== 常量定义 ====================
    static constexpr uint8_t MASTER_ADDR = 0x80;        // 中控地址（源地址）
    static constexpr int DEFAULT_PORT_COUNT = 8;        // 默认端口数量
    static constexpr int MAX_AP_COUNT = 2;              // 最大天线盒数量
    static constexpr int PRESET_FREQ_COUNT = 64;        // 预置频点数量
    static constexpr int MAX_CUSTOM_FREQ_COUNT = 64;    // 最大客制化频点数
    static constexpr int MIN_CUSTOM_FREQ_COUNT = 8;     // 最小客制化频点数

    // ==================== 命令码常量（按功能分组）====================
    struct Command {
        struct Host {
            static constexpr uint16_t GET_VERSION    = 0x0202;  // 获取主机版本号
            static constexpr uint16_t GET_BUILD_DATE = 0x0203;  // 获取主机编译日期
            static constexpr uint16_t GET_PORT_COUNT = 0x0204;  // 获取端口数量
            static constexpr uint16_t GET_AP_COUNT   = 0x0205;  // 获取天线盒数量
        };
        struct Ap {
            static constexpr uint16_t GET_STATUS  = 0x0206;  // 获取天线盒在线状态
            static constexpr uint16_t GET_TYPE    = 0x0207;  // 获取天线盒实际类型
            static constexpr uint16_t GET_VERSION = 0x0208;  // 获取天线盒版本号
            static constexpr uint16_t GET_DATE    = 0x0209;  // 获取天线盒编译日期
            static constexpr uint16_t SET_CONFIG  = 0x0112;  // 设置天线盒配置类型
            static constexpr uint16_t GET_CONFIG  = 0x0212;  // 获取天线盒配置类型
        };
        struct Config {
            static constexpr uint16_t SET_VOLUME      = 0x0110;  // 设置无线音量
            static constexpr uint16_t GET_VOLUME      = 0x0210;  // 获取无线音量
            static constexpr uint16_t SET_WORK_MODE   = 0x0113;  // 设置工作模式
            static constexpr uint16_t GET_WORK_MODE   = 0x0213;  // 获取工作模式
            static constexpr uint16_t SET_FREQ_POOL   = 0x0114;  // 设置频点池
            static constexpr uint16_t GET_FREQ_POOL   = 0x0214;  // 获取频点池
            static constexpr uint16_t SET_SENSITIVITY = 0x0119;  // 设置跳频灵敏度
            static constexpr uint16_t GET_SENSITIVITY = 0x0219;  // 获取跳频灵敏度
        };
        struct FreqPool {
            static constexpr uint16_t GET_PRESET_NS   = 0x0215;  // 获取预置频点数量
            static constexpr uint16_t GET_PRESET_FREQ = 0x0216;  // 获取预置频点值
            static constexpr uint16_t SET_CUSTOM_NS   = 0x0117;  // 设置客制化频点数量
            static constexpr uint16_t GET_CUSTOM_NS   = 0x0217;  // 获取客制化频点数量
            static constexpr uint16_t SET_CUSTOM_FREQ = 0x0118;  // 设置客制化频点值
            static constexpr uint16_t GET_CUSTOM_FREQ = 0x0218;  // 获取客制化频点值
        };
        struct PortConfig {
            static constexpr uint16_t SET_FREQ_ID = 0x011A;  // 设置端口频点ID
            static constexpr uint16_t GET_FREQ_ID = 0x021A;  // 获取端口频点ID（配置值）
            static constexpr uint16_t SET_RANGE   = 0x011B;  // 设置端口频率范围
            static constexpr uint16_t GET_RANGE   = 0x021B;  // 获取端口频率范围
        };
        struct PortStatus {
            static constexpr uint16_t GET_FREQ         = 0x0601;  // 获取端口当前频率
            static constexpr uint16_t GET_CURRENT_CHID = 0x0602;  // 获取端口当前CHID
            static constexpr uint16_t GET_USERDATA     = 0x0603;  // 获取端口UserData（含电量）
            static constexpr uint16_t GET_PHASELOCK    = 0x0604;  // 获取端口PhaseLock
            static constexpr uint16_t GET_RSSI         = 0x0605;  // 获取端口RSSI
            static constexpr uint16_t GET_AUDIO_RSSI   = 0x0606;  // 获取端口Audio RSSI
        };
        struct Stats {
            static constexpr uint16_t GET_RF_STATS = 0x0401;  // 获取无线报文统计
        };
    };

    // ==================== 核心数据容器（8个）====================
    HostBaseInfo        m_hostInfo;         // 主机基础信息
    SystemConfig        m_sysConfig;        // 系统配置
    ApInfoTable         m_apTable;          // 天线盒信息表
    FreqPoolTable       m_freqPool;         // 频点池数据
    PortConfigTable     m_portConfig;       // 端口配置表
    PortStatusTable     m_portStatus;       // 端口状态表
    ScanDataCache       m_scanCache;        // 扫描数据缓存
    RfStatsTable        m_rfStats;          // 无线报文统计

    // ==================== 通信与依赖 ====================
    HostManager*        m_hostManager = nullptr;        // 主机管理器指针
    ProtocolProcessor*  m_protocolProcessor = nullptr;  // 协议处理器指针

    // ==================== 初始化状态 ====================
    int m_initProgress = 0;     // 初始化进度 0-100
    int m_initStep = 0;         // 当前初始化步骤 (1-6)

    // ==================== 定时器 ====================
    QTimer* m_statusPollTimer = nullptr;    // 状态轮询定时器
    QTimer* m_scanTimer = nullptr;          // 扫描定时器
    QTimer* m_initStepTimer = nullptr;      // 初始化步骤定时器

    // ==================== 轮询控制 ====================
    int m_statusPollIndex = 0;      // 当前轮询的端口索引
    bool m_isStatusPolling = false; // 是否正在轮询

    // ==================== 批量请求管理 ====================
    struct BatchContext {
        QString batchId;        // 批次唯一标识
        int expected = 0;       // 期望响应数（硬编码）
        int received = 0;       // 已收到响应数
        bool completed = false; // 是否已完成
        QString operation;      // 操作名称
    };
    QMap<QString, BatchContext> m_activeBatches;  // 活跃批次映射表

    // ==================== 私有方法 ====================
    void setupProtocolProcessor();      // 初始化协议处理器（设置发送函数、连接信号槽）
    void startInitSequence();           // 开始初始化序列
    void processInitStep();             // 处理初始化步骤
    bool hasActiveBatch(const QString& operation) const;  // 检查指定操作的批次是否仍在进行中

    void parseAndDispatch(const ProtocolProcessor::ParsedFrameData& frame);  // 解析并分发响应数据
    void handleSetResponse(const ProtocolProcessor::ParsedFrameData& frame, const QString& operation);  // 通用设置响应处理
    void notifyStateSnapshot();         // 通知状态快照（供 HostManager 保存状态）

    // ==================== 响应处理函数 ====================
    void handleVersionResponse(const QByteArray& payload);      // 0x0202 主机版本
    void handleBuildDateResponse(const QByteArray& payload);    // 0x0203 编译日期
    void handlePortCountResponse(const QByteArray& payload);    // 0x0204 端口数量
    void handleApCountResponse(const QByteArray& payload);      // 0x0205 天线盒数量
    void handleApStatusResponse(const QByteArray& payload);     // 0x0206 天线盒在线状态
    void handleApTypeResponse(const QByteArray& payload);       // 0x0207 天线盒实际类型
    void handleApVersionResponse(const QByteArray& payload);    // 0x0208 天线盒版本
    void handleApDateResponse(const QByteArray& payload);       // 0x0209 天线盒编译日期
    void handleApConfigResponse(const QByteArray& payload);     // 0x0212 天线盒配置类型
    void handleWorkModeResponse(const QByteArray& payload);     // 0x0213 工作模式
    void handleFreqPoolResponse(const QByteArray& payload);     // 0x0214 频点池类型
    void handleSensitivityResponse(const QByteArray& payload);  // 0x0219 跳频灵敏度
    void handleVolumeResponse(const QByteArray& payload);       // 0x0210 无线音量
    void handlePresetNsResponse(const QByteArray& payload);     // 0x0215 预置频点数量
    void handlePresetFreqResponse(const QByteArray& payload);   // 0x0216 预置频点值
    void handleCustomNsResponse(const QByteArray& payload);     // 0x0217 客制化频点数量
    void handleCustomFreqResponse(const QByteArray& payload);   // 0x0218 客制化频点值
    void handlePortFreqResponse(const QByteArray& payload);     // 0x021A 端口频点ID
    void handlePortRangeResponse(const QByteArray& payload);    // 0x021B 端口频率范围
    void handlePortPhaseLockResponse(const QByteArray& payload); // 0x0604 端口锁相状态
    void handlePortRssiResponse(const QByteArray& payload);      // 0x0605 端口RSSI
    void handlePortAudioRssiResponse(const QByteArray& payload); // 0x0606 端口音频幅度
    void handlePortUserDataResponse(const QByteArray& payload);  // 0x0603 端口UserData（含电量）
    void handlePortFreqInfoResponse(const QByteArray& payload);  // 0x0601 端口当前频率
    void handleRfStatsResponse(const QByteArray& payload);       // 0x0401 无线报文统计

    // ==================== 辅助方法 ====================
    quint32 parseFreqFromPayload(const QByteArray& payload, int offset) const;  // 从负载解析频率值（4字节大端序）
    int8_t parseRssiFromPayload(const QByteArray& payload, int offset) const;   // 从负载解析 RSSI 值（1字节有符号）

    // ==================== 扫描辅助 ====================
    void requestNextScanFreq();     // 请求下一个频点的 RSSI

    // ==================== 自动分配频点算法 ====================
    uint8_t findBestChannelForPort(uint8_t portId) const;  // 为指定端口找到最优频点
    bool isChannelAvailable(uint8_t chId, uint8_t excludePortId) const;  // 检查频点是否可用
};

#endif // AFHCONTROLLER_H