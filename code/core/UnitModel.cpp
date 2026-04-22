#include "UnitModel.h"
#include <QDebug>
#include <QByteArray>

// ==================== 构造函数/析构函数 ====================

UnitModel::UnitModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qDebug() << "UnitModel: 创建实例";
}

UnitModel::~UnitModel()
{
    qDebug() << "UnitModel: 析构实例";
}

// ==================== QAbstractListModel 接口实现 ====================

int UnitModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_unitIdList.size();
}

QHash<int, QByteArray> UnitModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UnitIdRole] = "unitId";
    roles[UnitIdHexRole] = "unitIdHex";
    roles[PhysicalAddrRole] = "physicalAddr";
    roles[AliasRole] = "alias";
    roles[TypeRole] = "type";
    roles[TypeNameRole] = "typeName";
    roles[IdentityRole] = "identity";
    roles[IdentityNameRole] = "identityName";
    roles[IsOnlineRole] = "isOnline";
    roles[IsOpenedRole] = "isOpened";
    roles[IsSpeakingRole] = "isSpeaking";
    roles[IsWirelessRole] = "isWireless";
    roles[StatusTextRole] = "statusText";
    roles[LastSeenTimeRole] = "lastSeenTime";
    roles[BatteryPercentRole] = "batteryPercent";
    roles[BatteryVoltageRole] = "batteryVoltage";
    roles[ChargingStatusRole] = "chargingStatus";
    roles[ChargingStatusTextRole] = "chargingStatusText";
    roles[RssiRole] = "rssi";
    roles[IsLowBatteryRole] = "isLowBattery";
    roles[BatteryLevelTextRole] = "batteryLevelText";
    roles[RssiLevelTextRole] = "rssiLevelText";
    roles[FullDataRole] = "fullData";
    return roles;
}

QVariant UnitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_unitIdList.size()) {
        return QVariant();
    }

    uint16_t unitId = m_unitIdList.at(index.row());
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        return QVariant();
    }

    const UnitData& data = it.value();

    switch (role) {
    case UnitIdRole: return static_cast<int>(data.unitId);
    case UnitIdHexRole: return QString("0x%1").arg(data.unitId, 4, 16, QChar('0')).toUpper();
    case PhysicalAddrRole: return data.physicalAddr;
    case AliasRole: return data.alias;
    case TypeRole: return static_cast<int>(data.type);
    case TypeNameRole: return data.typeName();
    case IdentityRole: return static_cast<int>(data.identity);
    case IdentityNameRole: return data.identityName();
    case IsOnlineRole: return data.isOnline;
    case IsOpenedRole: return data.isOpened;
    case IsSpeakingRole: return data.isSpeaking;
    case IsWirelessRole: return data.isWireless();
    case StatusTextRole: return data.statusText();
    case LastSeenTimeRole: return data.lastSeenTime();
    case BatteryPercentRole: return data.isWireless() ? data.wireless.batteryPercent : -1;
    case BatteryVoltageRole: return data.isWireless() ? static_cast<int>(data.wireless.batteryVoltage) : 0;
    case ChargingStatusRole: return data.isWireless() ? static_cast<int>(data.wireless.chargingStatus) : 0;
    case ChargingStatusTextRole: return data.isWireless() ? chargingStatusDisplayName(data.wireless.chargingStatus) : "";
    case RssiRole: return data.isWireless() ? static_cast<int>(data.wireless.rssi) : 0;
    case IsLowBatteryRole: return data.isWireless() && data.wireless.isLowBattery();
    case BatteryLevelTextRole: return data.isWireless() ? data.wireless.batteryLevelText() : "";
    case RssiLevelTextRole: return data.isWireless() ? data.wireless.rssiLevelText() : "";
    case FullDataRole: return data.toVariantMap();
    default: return QVariant();
    }
}

// ==================== 数据更新接口 ====================

void UnitModel::updateUnitInfo(uint16_t unitId, const QString& physicalAddr,
                               UnitIdentity identity, UnitType type)
{
    // 0x0601 响应格式：55 0C F3 01 80 01 06 01 00 01 86 AA --> 55 14 EB 80 01 04 06 01 (00 01) (F8 D4 83 B9) (30) (22) (00) (00） 87 AA
    // unitId(2) 
    // + physicalAddr(4) 
    // + identity(1)    [0x10: 代表（Normal）;0x20: VIP; 0x30: 主席（Chairman）] 
    // + type(1)        [0x02: 有线座麦;0x20: 无线座麦;0x22: 有线无线融合座麦;0x40: 无线手持]
    // + reserved(2)

    UnitData data;
    data.unitId = unitId;
    data.physicalAddr = physicalAddr;
    data.identity = identity;
    data.type = type;
    data.isOnline = true;
    data.lastSeen = QDateTime::currentDateTime();

    // 保留原有的别名和无线状态（如果有）
    auto it = m_units.find(unitId);
    if (it != m_units.end()) {
        data.alias = it->alias;
        if (data.isWireless()) {
            data.wireless = it->wireless;
        }
    }

    addOrUpdateUnit(data);

    qDebug().noquote() << QString("UnitModel: 更新单元信息 - ID:0x%1, 类型:%2, 身份:%3, 物理地址:%4")
                              .arg(QString::asprintf("0x%04X", unitId),
                                   data.typeName(),
                                   data.identityName(),
                                   data.physicalAddr);
}

void UnitModel::updateUnitAlias(uint16_t unitId, const QString& alias)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        qWarning() << "UnitModel: 单元不存在，无法更新别名 - ID:" << unitId;
        return;
    }

    it->alias = alias;
    emitDataChanged(findIndex(unitId));
    emit unitDataChanged(unitId);

    qDebug().noquote() << QString("UnitModel: 更新单元别名 - ID:0x%1, 别名:%2")
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(alias);
}

void UnitModel::updateWirelessState(uint16_t unitId, uint16_t batteryVoltage,
                                    uint8_t batteryPercent, ChargingStatus chargingStatus,
                                    int8_t rssi)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        qWarning() << "UnitModel: 单元不存在，无法更新无线状态 - ID:"
                   << QString("0x%1").arg(unitId, 4, 16, QChar('0'));
        return;
    }

    if (!it->isWireless()) {
        qWarning() << "UnitModel: 非无线单元，无法更新无线状态 - ID:"
                   << QString("0x%1").arg(unitId, 4, 16, QChar('0'));
        return;
    }

    it->wireless.batteryVoltage = batteryVoltage;
    it->wireless.batteryPercent = batteryPercent;
    it->wireless.chargingStatus = chargingStatus;
    it->wireless.rssi = rssi;
    it->lastSeen = QDateTime::currentDateTime();

    emitDataChanged(findIndex(unitId));
    emit unitDataChanged(unitId);
    emit wirelessStateUpdated(unitId, batteryPercent, rssi);

    // 检查低电量告警
    checkLowBattery(*it);

    qDebug().noquote() << QString("UnitModel: 更新无线状态 - ID:0x%1, 电压:%2mV, 电量:%3%%, 充电:%4, RSSI:%5dBm")
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(batteryVoltage)
                              .arg(batteryPercent)
                              .arg(chargingStatusDisplayName(chargingStatus))
                              .arg(rssi);
}

void UnitModel::setUnitSpeaking(uint16_t unitId, bool isSpeaking)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        return;
    }

    if (it->isSpeaking != isSpeaking) {
        it->isSpeaking = isSpeaking;
        emitDataChanged(findIndex(unitId));
        emit unitDataChanged(unitId);

        qDebug().noquote() << QString("UnitModel: 发言状态变化 - ID:0x%1, 发言中:%2")
                                  .arg(unitId, 4, 16, QChar('0'))
                                  .arg(isSpeaking);
    }
}

void UnitModel::setUnitOpened(uint16_t unitId, bool isOpened)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        return;
    }

    if (it->isOpened != isOpened) {
        it->isOpened = isOpened;
        emitDataChanged(findIndex(unitId));
        emit unitDataChanged(unitId);

        qDebug().noquote() << QString("UnitModel: 开启状态变化 - ID:0x%1, 已开启:%2")
                                  .arg(unitId, 4, 16, QChar('0'))
                                  .arg(isOpened);
    }
}

void UnitModel::setUnitOnline(uint16_t unitId, bool isOnline)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        return;
    }

    if (it->isOnline != isOnline) {
        it->isOnline = isOnline;
        it->lastSeen = QDateTime::currentDateTime();
        emitDataChanged(findIndex(unitId));
        emit unitDataChanged(unitId);

        qDebug().noquote() << QString("UnitModel: 在线状态变化 - ID:0x%1, 在线:%2")
                                  .arg(unitId, 4, 16, QChar('0'))
                                  .arg(isOnline);
    }
}

void UnitModel::removeUnit(uint16_t unitId)
{
    int index = findIndex(unitId);
    if (index < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_unitIdList.removeAt(index);
    m_units.remove(unitId);
    endRemoveRows();

    emit unitListChanged();
    emit unitRemoved(unitId);

    qDebug().noquote() << QString("UnitModel: 删除单元 - ID:0x%1")
                              .arg(unitId, 4, 16, QChar('0'));
}

void UnitModel::clearAll()
{
    if (m_unitIdList.isEmpty()) {
        return;
    }

    beginResetModel();
    m_units.clear();
    m_unitIdList.clear();
    endResetModel();

    emit unitListChanged();

    qDebug() << "UnitModel: 清空所有单元";
}

void UnitModel::syncUnits(const QList<UnitData>& units)
{
    beginResetModel();
    m_units.clear();
    m_unitIdList.clear();

    for (const UnitData& data : units) {
        m_units[data.unitId] = data;
        m_unitIdList.append(data.unitId);
    }

    endResetModel();
    emit unitListChanged();

    qDebug().noquote() << QString("UnitModel: 同步单元列表，共 %1 个单元").arg(units.size());
}

// ==================== 操作接口 ====================

void UnitModel::openUnit(uint16_t unitId)
{
    if (!unitExists(unitId)) {
        qWarning() << "UnitModel: 单元不存在，无法打开 - ID:" << unitId;
        return;
    }

    qDebug().noquote() << QString("UnitModel: 请求打开单元 - ID:0x%1")
                              .arg(unitId, 4, 16, QChar('0'));

    emit requestOpenUnit(unitId);
}

void UnitModel::closeUnit(uint16_t unitId)
{
    if (!unitExists(unitId)) {
        qWarning() << "UnitModel: 单元不存在，无法关闭 - ID:" << unitId;
        return;
    }

    qDebug().noquote() << QString("UnitModel: 请求关闭单元 - ID:0x%1")
                              .arg(unitId, 4, 16, QChar('0'));

    emit requestCloseUnit(unitId);
}

void UnitModel::deleteUnit(uint16_t unitId)
{
    if (!unitExists(unitId)) {
        qWarning() << "UnitModel: 单元不存在，无法删除 - ID:" << unitId;
        return;
    }

    qDebug().noquote() << QString("UnitModel: 请求删除单元 - ID:0x%1")
                              .arg(unitId, 4, 16, QChar('0'));

    emit requestDeleteUnit(unitId);
}

void UnitModel::setUnitIdentity(uint16_t unitId, int identity)
{
    if (!unitExists(unitId)) {
        qWarning() << "UnitModel: 单元不存在，无法设置身份 - ID:" << unitId;
        return;
    }

    uint8_t identityValue = static_cast<uint8_t>(identity);

    qDebug().noquote() << QString("UnitModel: 请求设置身份 - ID:0x%1, 身份:0x%2")
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(identityValue, 2, 16, QChar('0'));

    emit requestSetIdentity(unitId, identityValue);
}

void UnitModel::setUnitAlias(uint16_t unitId, const QString& alias)
{
    if (!unitExists(unitId)) {
        qWarning() << "UnitModel: 单元不存在，无法设置别名 - ID:" << unitId;
        return;
    }

    qDebug().noquote() << QString("UnitModel: 请求设置别名 - ID:0x%1, 别名:%2")
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(alias);

    emit requestSetAlias(unitId, alias);
}

void UnitModel::refreshWirelessState(uint16_t unitId)
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        qWarning() << "UnitModel: 单元不存在，无法刷新状态 - ID:" << unitId;
        return;
    }

    if (!it->isWireless()) {
        qWarning() << "UnitModel: 非无线单元，无法刷新状态 - ID:" << unitId;
        return;
    }

    qDebug().noquote() << QString("UnitModel: 请求刷新无线状态 - ID:0x%1")
                              .arg(unitId, 4, 16, QChar('0'));

    emit requestRefreshWirelessState(unitId);
}

void UnitModel::openUnits(const QList<uint16_t>& unitIds)
{
    qDebug().noquote() << QString("UnitModel: 批量打开单元，数量:%1").arg(unitIds.size());
    for (uint16_t unitId : unitIds) {
        openUnit(unitId);
    }
}

void UnitModel::closeUnits(const QList<uint16_t>& unitIds)
{
    qDebug().noquote() << QString("UnitModel: 批量关闭单元，数量:%1").arg(unitIds.size());
    for (uint16_t unitId : unitIds) {
        closeUnit(unitId);
    }
}

void UnitModel::closeAllUnits()
{
    qDebug().noquote() << "UnitModel: 请求关闭所有单元";
    closeUnits(m_unitIdList);
}

void UnitModel::deleteUnits(const QList<uint16_t>& unitIds)
{
    qDebug().noquote() << QString("UnitModel: 批量删除单元，数量:%1").arg(unitIds.size());
    for (uint16_t unitId : unitIds) {
        deleteUnit(unitId);
    }
}

// ==================== 查询接口 ====================

QVariantMap UnitModel::getUnitInfo(uint16_t unitId) const
{
    auto it = m_units.find(unitId);
    if (it == m_units.end()) {
        return QVariantMap();
    }
    return it->toVariantMap();
}

QList<uint16_t> UnitModel::getUnitIds() const
{
    return m_unitIdList;
}

QList<uint16_t> UnitModel::getWirelessUnitIds() const
{
    QList<uint16_t> result;
    for (const UnitData& data : m_units) {
        if (data.isWireless()) {
            result.append(data.unitId);
        }
    }
    return result;
}

QList<uint16_t> UnitModel::getWiredUnitIds() const
{
    QList<uint16_t> result;
    for (const UnitData& data : m_units) {
        if (!data.isWireless()) {
            result.append(data.unitId);
        }
    }
    return result;
}

QList<uint16_t> UnitModel::getSpeakingUnitIds() const
{
    QList<uint16_t> result;
    for (const UnitData& data : m_units) {
        if (data.isSpeaking) {
            result.append(data.unitId);
        }
    }
    return result;
}

QList<uint16_t> UnitModel::getOpenedUnitIds() const
{
    QList<uint16_t> result;
    for (const UnitData& data : m_units) {
        if (data.isOpened) {
            result.append(data.unitId);
        }
    }
    return result;
}

bool UnitModel::unitExists(uint16_t unitId) const
{
    return m_units.contains(unitId);
}

// ==================== 统计信息 ====================

int UnitModel::wirelessCount() const
{
    int count = 0;
    for (const UnitData& data : m_units) {
        if (data.isWireless()) count++;
    }
    return count;
}

int UnitModel::wiredCount() const
{
    return m_units.size() - wirelessCount();
}

int UnitModel::onlineCount() const
{
    int count = 0;
    for (const UnitData& data : m_units) {
        if (data.isOnline) count++;
    }
    return count;
}

int UnitModel::speakingCount() const
{
    int count = 0;
    for (const UnitData& data : m_units) {
        if (data.isSpeaking) count++;
    }
    return count;
}

int UnitModel::openedCount() const
{
    int count = 0;
    for (const UnitData& data : m_units) {
        if (data.isOpened) count++;
    }
    return count;
}

// ==================== 私有方法 ====================

int UnitModel::findIndex(uint16_t unitId) const
{
    return m_unitIdList.indexOf(unitId);
}

void UnitModel::addOrUpdateUnit(const UnitData& data)
{
    int index = findIndex(data.unitId);

    if (index >= 0) {
        // 更新现有单元
        m_units[data.unitId] = data;
        emitDataChanged(index);
        emit unitDataChanged(data.unitId);
    } else {
        // 添加新单元
        beginInsertRows(QModelIndex(), m_unitIdList.size(), m_unitIdList.size());
        m_units[data.unitId] = data;
        m_unitIdList.append(data.unitId);
        endInsertRows();
        emit unitListChanged();
        emit unitAdded(data.unitId);
    }
}

void UnitModel::emitDataChanged(int row)
{
    if (row >= 0 && row < m_unitIdList.size()) {
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }
}

void UnitModel::checkLowBattery(const UnitData& data)
{
    if (data.isWireless() && data.wireless.isLowBattery()) {
        emit lowBatteryWarning(data.unitId, data.wireless.batteryPercent);

        if (data.wireless.isCriticalBattery()) {
            qWarning().noquote() << QString("UnitModel: 单元电量危急！ID:0x%1, 电量:%2%")
                                        .arg(data.unitId, 4, 16, QChar('0'))
                                        .arg(data.wireless.batteryPercent);
        } else {
            qDebug().noquote() << QString("UnitModel: 单元电量较低 - ID:0x%1, 电量:%2%")
                                      .arg(data.unitId, 4, 16, QChar('0'))
                                      .arg(data.wireless.batteryPercent);
        }
    }
}
