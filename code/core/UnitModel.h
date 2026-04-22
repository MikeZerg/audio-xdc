#ifndef UNITMODEL_H
#define UNITMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QDateTime>
#include <QObject>

// ==================== 枚举定义 ====================

// 单元类型枚举（与协议一致）
enum class UnitType : uint8_t {
    WiredDesktop = 0x02,       // 有线座麦
    WirelessDesktop = 0x20,    // 无线座麦
    HybridDesktop = 0x22,      // 有线无线融合座麦
    WirelessHandheld = 0x40,   // 无线手持
    Unknown = 0xFF
};
Q_DECLARE_METATYPE(UnitType)

// 单元身份枚举（与协议一致）
enum class UnitIdentity : uint8_t {
    Normal = 0x10,     // 代表
    VIP = 0x20,        // VIP
    Chairman = 0x30,   // 主席
    Unknown = 0xFF
};
Q_DECLARE_METATYPE(UnitIdentity)

// 充电状态枚举
enum class ChargingStatus : uint8_t {
    NotCharging = 0,   // 未充电
    Charging = 1,      // 充电中
    Full = 2           // 已充满
};
Q_DECLARE_METATYPE(ChargingStatus)

// ==================== 辅助函数 ====================

inline bool isWirelessType(UnitType type) {
    return type == UnitType::WirelessDesktop ||
           type == UnitType::HybridDesktop ||
           type == UnitType::WirelessHandheld;
}

inline QString typeDisplayName(UnitType type) {
    switch(type) {
    case UnitType::WiredDesktop:     return QObject::tr("有线座麦");
    case UnitType::WirelessDesktop:  return QObject::tr("无线座麦");
    case UnitType::HybridDesktop:    return QObject::tr("融合座麦");
    case UnitType::WirelessHandheld: return QObject::tr("无线手持");
    default: return QObject::tr("未知设备");
    }
}

inline QString identityDisplayName(UnitIdentity identity) {
    switch(identity) {
    case UnitIdentity::Normal:    return QObject::tr("代表");
    case UnitIdentity::VIP:       return QObject::tr("VIP");
    case UnitIdentity::Chairman:  return QObject::tr("主席");
    default: return QObject::tr("未知");
    }
}

inline QString chargingStatusDisplayName(ChargingStatus status) {
    switch(status) {
    case ChargingStatus::NotCharging: return QObject::tr("未充电");
    case ChargingStatus::Charging:    return QObject::tr("充电中");
    case ChargingStatus::Full:        return QObject::tr("已充满");
    default: return QObject::tr("未知");
    }
}

// ==================== 无线单元扩展数据 ====================

struct WirelessExtension {
    uint16_t batteryVoltage = 0;      // 电池电压 (mV)
    uint8_t batteryPercent = 0;       // 电池百分比 (0-100)
    ChargingStatus chargingStatus = ChargingStatus::NotCharging;
    int8_t rssi = 0;                  // 信号强度 (dBm)

    // 辅助方法
    bool isLowBattery() const { return batteryPercent < 20 && batteryPercent > 0; }
    bool isCriticalBattery() const { return batteryPercent < 10 && batteryPercent > 0; }
    QString batteryLevelText() const {
        if (batteryPercent >= 80) return QObject::tr("充足");
        if (batteryPercent >= 50) return QObject::tr("良好");
        if (batteryPercent >= 20) return QObject::tr("较低");
        if (batteryPercent > 0) return QObject::tr("危急");
        return QObject::tr("未知");
    }
    QString rssiLevelText() const {
        if (rssi > -50) return QObject::tr("强");
        if (rssi > -70) return QObject::tr("中");
        if (rssi > -90) return QObject::tr("弱");
        return QObject::tr("极弱");
    }

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["batteryVoltage"] = batteryVoltage;
        map["batteryPercent"] = batteryPercent;
        map["chargingStatus"] = static_cast<int>(chargingStatus);
        map["chargingStatusText"] = chargingStatusDisplayName(chargingStatus);
        map["rssi"] = rssi;
        map["isLowBattery"] = isLowBattery();
        map["batteryLevelText"] = batteryLevelText();
        map["rssiLevelText"] = rssiLevelText();
        return map;
    }
};

// ==================== 单元数据结构 ====================

struct UnitData {
    // === 标识信息 ===
    uint16_t unitId = 0;                     // 单元ID
    QString physicalAddr;                    // 物理地址 (MAC格式)
    QString alias;                           // 别名 (最多16字符)

    // === 类型信息 ===
    UnitType type = UnitType::Unknown;       // 单元类型
    UnitIdentity identity = UnitIdentity::Normal;  // 身份

    // === 状态信息 ===
    bool isOnline = false;                   // 是否在线
    bool isOpened = false;                   // 是否允许发言
    bool isSpeaking = false;                 // 是否正在发言
    QDateTime lastSeen;                      // 最后通信时间

    // === 无线单元专属（有线单元忽略） ===
    WirelessExtension wireless;              // 可选字段

    // === 辅助方法 ===
    bool isWireless() const { return ::isWirelessType(type); }
    bool hasBattery() const { return isWireless(); }
    QString typeName() const { return typeDisplayName(type); }
    QString identityName() const { return identityDisplayName(identity); }
    QString statusText() const {
        if (!isOnline) return QObject::tr("未注册");
        if (isSpeaking) return QObject::tr("发言中");
        if (isOpened) return QObject::tr("已开启");
        return QObject::tr("已注册");
    }
    QString lastSeenTime() const { return lastSeen.toString("hh:mm:ss"); }

    // 转换为 QVariantMap（供 QML 使用）
    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["unitId"] = static_cast<int>(unitId);
        map["unitIdHex"] = QString("0x%1").arg(unitId, 4, 16, QChar('0')).toUpper();
        map["physicalAddr"] = physicalAddr;
        map["alias"] = alias;
        map["type"] = static_cast<int>(type);
        map["typeName"] = typeName();
        map["identity"] = static_cast<int>(identity);
        map["identityName"] = identityName();
        map["isOnline"] = isOnline;
        map["isOpened"] = isOpened;
        map["isSpeaking"] = isSpeaking;
        map["isWireless"] = isWireless();
        map["statusText"] = statusText();
        map["lastSeenTime"] = lastSeenTime();

        if (isWireless()) {
            map["wireless"] = wireless.toVariantMap();
            map["batteryPercent"] = wireless.batteryPercent;
            map["batteryVoltage"] = wireless.batteryVoltage;
            map["chargingStatus"] = static_cast<int>(wireless.chargingStatus);
            map["rssi"] = wireless.rssi;
            map["isLowBattery"] = wireless.isLowBattery();
        } else {
            map["batteryPercent"] = -1;
            map["rssi"] = 0;
            map["isLowBattery"] = false;
        }

        return map;
    }
};

// ==================== 单元模型类 ====================

class UnitModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int unitCount READ rowCount NOTIFY unitListChanged)
    Q_PROPERTY(int wirelessCount READ wirelessCount NOTIFY unitListChanged)
    Q_PROPERTY(int wiredCount READ wiredCount NOTIFY unitListChanged)
    Q_PROPERTY(int onlineCount READ onlineCount NOTIFY unitListChanged)
    Q_PROPERTY(int speakingCount READ speakingCount NOTIFY unitListChanged)
    Q_PROPERTY(int openedCount READ openedCount NOTIFY unitListChanged)

public:
    explicit UnitModel(QObject *parent = nullptr);
    ~UnitModel();

    QString formatAddressHex(uint8_t address) { return QString::asprintf("0x%02X", address); }

    // 角色枚举
    enum UnitRoles {
        UnitIdRole = Qt::UserRole + 1,
        UnitIdHexRole,
        PhysicalAddrRole,
        AliasRole,
        TypeRole,
        TypeNameRole,
        IdentityRole,
        IdentityNameRole,
        IsOnlineRole,
        IsOpenedRole,
        IsSpeakingRole,
        IsWirelessRole,
        StatusTextRole,
        LastSeenTimeRole,
        // 无线专属角色
        BatteryPercentRole,
        BatteryVoltageRole,
        ChargingStatusRole,
        ChargingStatusTextRole,
        RssiRole,
        IsLowBatteryRole,
        BatteryLevelTextRole,
        RssiLevelTextRole,
        // 完整数据
        FullDataRole
    };

    // QAbstractListModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ========== 数据更新接口（供 XdcController 调用） ==========

    /// 更新单元基本信息（解析 0x0601 响应）
    void updateUnitInfo(uint16_t unitId, const QString& physicalAddr,
                        UnitIdentity identity, UnitType type);

    /// 更新单元别名（解析 0x0602 响应）
    void updateUnitAlias(uint16_t unitId, const QString& alias);

    /// 更新无线单元状态（解析 0x0606 响应）
    void updateWirelessState(uint16_t unitId, uint16_t batteryVoltage,
                             uint8_t batteryPercent, ChargingStatus chargingStatus,
                             int8_t rssi);

    /// 设置单元发言状态（从 0x020E/0x020F 更新）
    void setUnitSpeaking(uint16_t unitId, bool isSpeaking);

    /// 设置单元开启状态（从 0x0504/0x0505 响应更新）
    void setUnitOpened(uint16_t unitId, bool isOpened);

    /// 设置单元在线状态
    void setUnitOnline(uint16_t unitId, bool isOnline);

    /// 删除单元
    void removeUnit(uint16_t unitId);

    /// 清空所有单元
    void clearAll();

    /// 批量更新单元列表（从主机同步）
    void syncUnits(const QList<UnitData>& units);

    // ========== 操作接口（供 QML 调用，内部通过信号通知 XdcController） ==========

    Q_INVOKABLE void openUnit(uint16_t unitId);
    Q_INVOKABLE void closeUnit(uint16_t unitId);
    Q_INVOKABLE void deleteUnit(uint16_t unitId);
    Q_INVOKABLE void setUnitIdentity(uint16_t unitId, int identity);
    Q_INVOKABLE void setUnitAlias(uint16_t unitId, const QString& alias);
    Q_INVOKABLE void refreshWirelessState(uint16_t unitId);

    // 批量操作
    Q_INVOKABLE void openUnits(const QList<uint16_t>& unitIds);
    Q_INVOKABLE void closeUnits(const QList<uint16_t>& unitIds);
    Q_INVOKABLE void closeAllUnits();
    Q_INVOKABLE void deleteUnits(const QList<uint16_t>& unitIds);

    // 查询接口
    Q_INVOKABLE QVariantMap getUnitInfo(uint16_t unitId) const;
    Q_INVOKABLE QList<uint16_t> getUnitIds() const;
    Q_INVOKABLE QList<uint16_t> getWirelessUnitIds() const;
    Q_INVOKABLE QList<uint16_t> getWiredUnitIds() const;
    Q_INVOKABLE QList<uint16_t> getSpeakingUnitIds() const;
    Q_INVOKABLE QList<uint16_t> getOpenedUnitIds() const;
    Q_INVOKABLE bool unitExists(uint16_t unitId) const;

    // 统计信息
    int wirelessCount() const;
    int wiredCount() const;
    int onlineCount() const;
    int speakingCount() const;
    int openedCount() const;

signals:
    // 列表变化信号
    void unitListChanged();
    void unitDataChanged(uint16_t unitId);
    void unitAdded(uint16_t unitId);
    void unitRemoved(uint16_t unitId);

    // 无线单元状态变化信号
    void wirelessStateUpdated(uint16_t unitId, int batteryPercent, int rssi);
    void lowBatteryWarning(uint16_t unitId, int batteryPercent);

    // 操作请求信号（由 XdcController 监听并执行）
    void requestOpenUnit(uint16_t unitId);
    void requestCloseUnit(uint16_t unitId);
    void requestDeleteUnit(uint16_t unitId);
    void requestSetIdentity(uint16_t unitId, uint8_t identity);
    void requestSetAlias(uint16_t unitId, const QString& alias);
    void requestRefreshWirelessState(uint16_t unitId);

private:
    // 内部方法
    int findIndex(uint16_t unitId) const;
    void addOrUpdateUnit(const UnitData& data);
    void emitDataChanged(int row);
    void checkLowBattery(const UnitData& data);

    // 数据存储
    QHash<uint16_t, UnitData> m_units;
    QVector<uint16_t> m_unitIdList;  // 保持顺序
};

#endif // UNITMODEL_H