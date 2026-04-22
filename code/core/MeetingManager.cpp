// code/core/MeetingManager.cpp
#include "MeetingManager.h"
#include "HostManager.h"
#include <QDebug>
#include <QThread>

// ==================== 构造与析构 ====================
MeetingManager::MeetingManager(QObject* parent)
    : QObject(parent)
    , m_hostManager(nullptr)
    , m_globalEventId(1)
{
}

MeetingManager::~MeetingManager()
{
}

void MeetingManager::setHostManager(HostManager* manager)
{
    m_hostManager = manager;
    if (m_hostManager) {
        // 连接 HostManager 的路由信号 (分支3: 会议命令响应)
        connect(m_hostManager, &HostManager::meetingCommandResponse,
                this, &MeetingManager::onMeetingCommandResponse);
    }
}

// ==================== 会议主机管理 ====================
void MeetingManager::setMeetingHosts(const QList<uint8_t>& addresses)
{
    m_meetingHosts = addresses;
    emit signal_meetingHostsChanged();
    qDebug() << "MeetingManager: 会议关联主机已更新，数量:" << m_meetingHosts.size();
}

// ==================== 会议管理功能 ====================
void MeetingManager::createQuickMeeting(const QString& subject,
                                        const QString& description,
                                        int durationMinutes)
{
    if (m_currentMeeting.status == MeetingStatus::InProgress) {
        emit signal_warning("当前已有进行中的会议");
        return;
    }

    m_currentMeeting.reset();
    m_currentMeeting.mid = generateMeetingId();
    m_currentMeeting.subject = subject;
    m_currentMeeting.description = description;
    m_currentMeeting.startTime = QDateTime::currentDateTime();
    m_currentMeeting.durationSecs = durationMinutes * 60;
    m_currentMeeting.status = MeetingStatus::InProgress;

    qDebug() << "MeetingManager: 快速会议已创建 - ID:" << m_currentMeeting.mid;
    emit signal_meetingUpdated();
}

void MeetingManager::createScheduledMeeting(const QString& subject,
                                            const QString& description,
                                            QDateTime startTime,
                                            int durationMinutes)
{
    m_currentMeeting.reset();
    m_currentMeeting.mid = generateMeetingId();
    m_currentMeeting.subject = subject;
    m_currentMeeting.description = description;
    m_currentMeeting.startTime = startTime;
    m_currentMeeting.durationSecs = durationMinutes * 60;
    m_currentMeeting.status = MeetingStatus::NotStarted;

    qDebug() << "MeetingManager: 计划会议已创建 - ID:" << m_currentMeeting.mid;
    emit signal_meetingUpdated();
}

void MeetingManager::startCurrentMeeting()
{
    if (m_currentMeeting.status != MeetingStatus::NotStarted) {
        emit signal_warning("当前会议状态不允许启动");
        return;
    }
    m_currentMeeting.status = MeetingStatus::InProgress;
    m_currentMeeting.startTime = QDateTime::currentDateTime();
    emit signal_meetingUpdated();
}

void MeetingManager::endCurrentMeeting()
{
    if (m_currentMeeting.status != MeetingStatus::InProgress) return;

    // 结束前自动归档当前活跃的事件
    if (m_currentMeeting.currentCheckIn.isActive) archiveCurrentCheckIn();
    if (m_currentMeeting.currentVoting.isActive) archiveCurrentVoting();

    m_currentMeeting.endTime = QDateTime::currentDateTime();
    m_currentMeeting.status = MeetingStatus::Ended;

    qDebug() << "MeetingManager: 会议已结束 - ID:" << m_currentMeeting.mid;
    emit signal_meetingUpdated();
}

// ==================== 签到管理功能 ====================
void MeetingManager::createCheckInEvent(int durationMinutes)
{
    if (m_currentMeeting.status != MeetingStatus::InProgress) {
        emit signal_warning("请先开始会议");
        return;
    }
    if (m_currentMeeting.currentVoting.isActive) {
        emit signal_warning("当前有投票正在进行，请先结束投票");
        return;
    }

    // 如果已有签到在进行，先归档
    if (m_currentMeeting.currentCheckIn.isActive) {
        archiveCurrentCheckIn();
    }

    m_currentMeeting.currentCheckIn.cid = generateEventId();
    m_currentMeeting.currentCheckIn.startTime = QDateTime::currentDateTime();
    m_currentMeeting.currentCheckIn.durationSecs = durationMinutes * 60;
    m_currentMeeting.currentCheckIn.isActive = true;

    // 发送指令到所有关联主机 (假设命令码 0x0701 为开启签到)
    QByteArray payload;
    payload.append(static_cast<char>(durationMinutes));
    sendToMultipleHosts(0x0701, payload);

    emit signal_meetingUpdated();
}

void MeetingManager::resetCheckInEvent(int durationMinutes)
{
    if (!m_currentMeeting.currentCheckIn.isActive) return;

    if (durationMinutes == 0) {
        // 结束签到并归档
        archiveCurrentCheckIn();
    } else {
        // 重置时长
        m_currentMeeting.currentCheckIn.durationSecs = durationMinutes * 60;
        // 发送重置指令 (假设命令码 0x0702)
        QByteArray payload;
        payload.append(static_cast<char>(durationMinutes));
        sendToMultipleHosts(0x0702, payload);
    }
    emit signal_meetingUpdated();
}

// ==================== 投票管理功能 ====================
void MeetingManager::createReferendumVoting(int durationMinutes)
{
    QVariantList options = {
        QVariantMap{{"label", "赞成"}, {"value", 1}},
        QVariantMap{{"label", "弃权"}, {"value", 0}},
        QVariantMap{{"label", "反对"}, {"value", -1}}
    };
    createCustomVoting(durationMinutes, options);
}

void MeetingManager::createCustomVoting(int durationMinutes, const QVariantList& options)
{
    if (m_currentMeeting.status != MeetingStatus::InProgress) {
        emit signal_warning("请先开始会议");
        return;
    }
    if (m_currentMeeting.currentCheckIn.isActive) {
        emit signal_warning("当前有签到正在进行，请先结束签到");
        return;
    }

    if (m_currentMeeting.currentVoting.isActive) {
        archiveCurrentVoting();
    }

    m_currentMeeting.currentVoting.vid = generateEventId();
    m_currentMeeting.currentVoting.startTime = QDateTime::currentDateTime();
    m_currentMeeting.currentVoting.durationSecs = durationMinutes * 60;
    m_currentMeeting.currentVoting.isActive = true;
    m_currentMeeting.currentVoting.options.clear();

    for (const auto& optVar : options) {
        QVariantMap optMap = optVar.toMap();
        VotingOption vo;
        vo.label = optMap["label"].toString();
        vo.value = optMap["value"].toInt();
        m_currentMeeting.currentVoting.options.push_back(vo);
    }

    // 发送投票开启指令 (假设命令码 0x0703)
    // 实际项目中需根据协议构建复杂的 payload
    sendToMultipleHosts(0x0703, QByteArray());

    emit signal_meetingUpdated();
}

void MeetingManager::resetVotingEvent(int durationMinutes)
{
    if (!m_currentMeeting.currentVoting.isActive) return;

    if (durationMinutes == 0) {
        archiveCurrentVoting();
    } else {
        m_currentMeeting.currentVoting.durationSecs = durationMinutes * 60;
        sendToMultipleHosts(0x0704, QByteArray()); // 假设的重置指令
    }
    emit signal_meetingUpdated();
}

// ==================== 数据查询与历史接口 ====================
QVariantMap MeetingManager::getCurrentMeetingData() const
{
    return m_currentMeeting.toVariantMap();
}

int MeetingManager::getTotalParticipantCount() const
{
    // 简单实现：返回预期人数，实际应从 HostManager 获取在线单元总数
    return qMax(1, m_currentMeeting.expectedParticipants);
}

QVariantList MeetingManager::getCheckInHistory() const
{
    QVariantList list;
    for (const auto& event : m_currentMeeting.checkInHistory) {
        list.append(event.toVariantMap(false)); // 不携带详细名单
    }
    return list;
}

QVariantList MeetingManager::getVotingHistory() const
{
    QVariantList list;
    for (const auto& event : m_currentMeeting.votingHistory) {
        list.append(event.toVariantMap(false)); // 不携带详细结果
    }
    return list;
}

QVariantMap MeetingManager::getCheckInHistoryDetail(int index) const
{
    if (index < 0 || index >= m_currentMeeting.checkInHistory.size()) return {};
    return m_currentMeeting.checkInHistory.at(index).toVariantMap(true);
}

QVariantMap MeetingManager::getVotingHistoryDetail(int index) const
{
    if (index < 0 || index >= m_currentMeeting.votingHistory.size()) return {};
    return m_currentMeeting.votingHistory.at(index).toVariantMap(true);
}

// ==================== 内部逻辑与响应处理 ====================
void MeetingManager::onMeetingCommandResponse(uint8_t hostAddress, uint16_t command, const QByteArray& payload)
{
    // 检查该主机是否属于当前会议
    if (!m_meetingHosts.contains(hostAddress)) return;

    switch (command) {
    case 0x0701: // 签到响应
    case 0x0702: // 签到结果上报
        parseCheckInResponse(hostAddress, payload);
        break;
    case 0x0703: // 投票响应
    case 0x0704: // 投票结果上报
        parseVotingResponse(hostAddress, payload);
        break;
    default:
        break;
    }
}

bool MeetingManager::sendToMultipleHosts(uint16_t command, const QByteArray& payload)
{
    if (!m_hostManager) return false;

    bool success = false;
    for (uint8_t addr : m_meetingHosts) {
        // 调用 HostManager 的统一发送接口
        // 参数：目标地址, 源地址(中控), 操作码(请求), 命令码, 负载
        if (m_hostManager->sendProtocolFrame(addr, 0x80, 0x01, command, payload)) {
            success = true;
            QThread::msleep(10); // 避免总线冲突，微小延时
        }
    }
    return success;
}

void MeetingManager::parseCheckInResponse(uint8_t hostAddr, const QByteArray& payload)
{
    if (!m_currentMeeting.currentCheckIn.isActive) return;

    // 协议解析示例：假设 payload 包含 unitId (2字节)
    if (payload.size() >= 2) {
        quint16 unitId = (static_cast<quint8>(payload[0]) << 8) | static_cast<quint8>(payload[1]);
        m_currentMeeting.currentCheckIn.results[unitId] = QDateTime::currentDateTime();
        emit signal_meetingUpdated();
    }
}

void MeetingManager::parseVotingResponse(uint8_t hostAddr, const QByteArray& payload)
{
    if (!m_currentMeeting.currentVoting.isActive) return;

    // 协议解析示例：假设 payload 包含 unitId (2字节) + option (1字节)
    if (payload.size() >= 3) {
        quint16 unitId = (static_cast<quint8>(payload[0]) << 8) | static_cast<quint8>(payload[1]);
        int optionVal = static_cast<int8_t>(payload[2]); // 支持负数（如反对票）

        m_currentMeeting.currentVoting.results[unitId] = std::make_pair(QDateTime::currentDateTime(), optionVal);
        emit signal_meetingUpdated();
    }
}

void MeetingManager::archiveCurrentCheckIn()
{
    m_currentMeeting.currentCheckIn.isActive = false;
    m_currentMeeting.currentCheckIn.endTime = QDateTime::currentDateTime();
    m_currentMeeting.checkInHistory.append(m_currentMeeting.currentCheckIn);
    m_currentMeeting.currentCheckIn.reset();
}

void MeetingManager::archiveCurrentVoting()
{
    m_currentMeeting.currentVoting.isActive = false;
    m_currentMeeting.currentVoting.endTime = QDateTime::currentDateTime();
    m_currentMeeting.votingHistory.append(m_currentMeeting.currentVoting);
    m_currentMeeting.currentVoting.reset();
}

quint16 MeetingManager::generateEventId()
{
    return m_globalEventId++;
}

QString MeetingManager::generateMeetingId()
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}