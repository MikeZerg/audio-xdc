// source/MeetingManager.cpp
#include "MeetingManager.h"
#include "HostManager.h"
#include "HostController.h"
#include <QDateTime>
#include <QDebug>
#include "tinyxml2.h"
#include <QFile>
#include <QDir>
#include <QDate>

// 添加构造函数实现
MeetingManager::MeetingManager(QObject *parent)
    : QObject(parent)
    , m_hostManager(nullptr)
    , m_globalEventId(1)        // 全局事件ID从1开始计数
    , m_isReferendumVoting(false)
{
    // 初始化成员变量
    m_currentMeeting = Meeting();
    m_currentCheckInEvent = CheckInEvent();
    m_currentVotingEvent = VotingEvent();
    m_duration = "";
}

// 设置HostManager引用
void MeetingManager::setHostManager(HostManager* hostManager)
{
    m_hostManager = hostManager;

    // 连接主机切换信号，以便在主机切换时能够更新相关状态
    if (m_hostManager != nullptr) {
        connect(m_hostManager, &HostManager::currentHostChanged, this, [this]() {
            qDebug() << "重要：MeetingManager: 检测到主机切换，更新当前主机引用";
            // 可以在这里添加其他需要在主机切换时执行的逻辑
            emit signal_meetingsChanged(); // 发出会议信息变化信号，通知UI更新
        });
    }
}

// 获取当前主机地址
quint8 MeetingManager::getCurrentHostAddress() const
{
    if (m_hostManager != nullptr) {
        HostController* currentHost = m_hostManager->currentHostController();
        if (currentHost != nullptr) {
            return currentHost->address();
        }
    }
    return 0; // 返回默认地址0表示无主机
}

// ***获取当前主机控制器实例
HostController* MeetingManager::getCurrentHostController() const
{
    if (m_hostManager != nullptr) {
        HostController* controller = m_hostManager->currentHostController();
        return controller;
    }
    return nullptr;
}

// 发送会议命令到当前主机
void MeetingManager::sendMeetingCommandToHost(const QString& command)
{
    HostController* currentHost = getCurrentHostController();
    if (currentHost != nullptr) {
        qDebug() << "发送命令至主机:" << command;
        // currentHost->someMethod(); // 调用具体的主机控制方法
    } else {
        qDebug() << "当前无可用主机";
    }
}

// 实现生成唯一签到事件ID的方法
quint16 MeetingManager::generate_UniqueEventId()
{
    // 确保ID在有效范围内（1-65535）
    if (m_globalEventId == 0) {
        m_globalEventId = 1;
    }

    quint16 uniqueId = m_globalEventId;
    m_globalEventId++;

    return uniqueId;
}

// 创建新的签到事件数据结构
CheckInEvent MeetingManager::create_CheckInStruct(int duration)
{
    CheckInEvent newEvent;
    newEvent.cid = generate_UniqueEventId();
    newEvent.mid = m_currentMeeting.mid;
    newEvent.startTime = QDateTime::currentDateTime();
    newEvent.endTime = newEvent.startTime.addSecs(duration*60);
    newEvent.duration = duration;
    newEvent.isActive = true;
    return newEvent;
}

// 创建新的投票事件数据结构
VotingEvent MeetingManager::create_VotingEventStruct(int duration, qint16 optionValue1, qint16 optionValue2) {
    VotingEvent newEvent;
    newEvent.vid = generate_UniqueEventId();
    newEvent.mid = m_currentMeeting.mid;
    newEvent.startTime = QDateTime::currentDateTime();
    newEvent.endTime = newEvent.startTime.addSecs(duration*60);
    newEvent.duration = duration;
    newEvent.isActive = true;
    return newEvent;
}


// ************************** 会议管理相关函数 ********************************//
// 创建新会议记录
void MeetingManager::start_QuickMeeting(const QString& subject, const QString& description, int duration, int numParticipant)
{
    // 检查是否已存在会议记录， 如果存在直接返回不做任何操作
    if (!m_currentMeeting.mid.isEmpty() && m_currentMeeting.status != MeetingStatus::Ended) {
        qDebug() << "警告：当前已有未结束的会议，会议ID:" << m_currentMeeting.mid;
        return;
    }

    // 使用当前时间作为会议开始时间;根据开始时间和持续时间计算结束时间
    QDateTime currentStartTime = QDateTime::currentDateTime();
    QDateTime endTime = currentStartTime.addSecs(duration*60);

    // 生成基于日期的会议编号 YYYYMMDDXXX
    QDate currentDate = QDate::currentDate();
    QString datePrefix = currentDate.toString("yyyyMMdd");
    QString newMeetingId;
    int sequenceNumber = 1;

    // 查找当天最大的序号
    for (const auto& pair : m_meetings) {
        const Meeting& meeting = pair.second;
        QString meetingId = meeting.mid;

        // 检查是否是同一天的会议ID
        if (meetingId.startsWith(datePrefix)) {
            // 提取序号部分 (新格式 YYYYMMDDXXX)
            QString seqStr = meetingId.mid(8); // 跳过前8位日期部分
            bool ok;
            int seq = seqStr.toInt(&ok);
            if (ok && seq >= sequenceNumber) {
                sequenceNumber = seq + 1;
            }
        }
    }

    // 生成新的会议ID，格式为 YYYYMMDDXXX
    newMeetingId = QString("%1%2").arg(datePrefix).arg(sequenceNumber, 3, 10, QChar('0'));

    // 创建新的会议记录
    Meeting newMeeting;
    newMeeting.mid = newMeetingId;
    newMeeting.subject = subject;
    newMeeting.description = description;
    newMeeting.startTime = currentStartTime;
    newMeeting.endTime = endTime;
    newMeeting.duration = duration;
    newMeeting.numParticipant = numParticipant;
    newMeeting.status = MeetingStatus::InProgress;

    // 将新会议存入当前会议变量
    m_currentMeeting = newMeeting;

    // 同时将会议保存到所有会议的映射中
    m_meetings[newMeeting.mid] = newMeeting;

    // 先发送会议主题到主机，指令0x0701
    send_MeetingSubjectCommand(subject);

    // 发出会议更改信号
    emit signal_meetingsChanged();
}

void MeetingManager::start_ScheduleMeeting(const QString& subject, const QString& description, QDateTime startTime, int duration, int numParticipant)
{
    // 检查会议开始时间是否晚于当前时间
    QDateTime currentTime = QDateTime::currentDateTime();
    if (startTime <= currentTime) {
        qDebug() << "警告：计划会议开始时间必须晚于当前时间";
        return;
    }

    // 根据开始时间和持续时间计算结束时间
    QDateTime endTime = startTime.addSecs(duration*60);

    // 生成基于日期的会议编号 YYYYMMDDXXX
    QDate currentDate = QDate::currentDate();
    QString datePrefix = currentDate.toString("yyyyMMdd");
    QString newMeetingId;
    int sequenceNumber = 1;

    // 查找当天最大的序号
    for (const auto& pair : m_meetings) {
        const Meeting& meeting = pair.second;
        QString meetingId = meeting.mid;

        // 检查是否是同一天的会议ID
        if (meetingId.startsWith(datePrefix)) {
            // 提取序号部分 (新格式 YYYYMMDDXXX)
            QString seqStr = meetingId.mid(8); // 跳过前8位日期部分
            bool ok;
            int seq = seqStr.toInt(&ok);
            if (ok && seq >= sequenceNumber) {
                sequenceNumber = seq + 1;
            }
        }
    }

    // 生成新的会议ID，格式为 YYYYMMDDXXX
    newMeetingId = QString("%1%2").arg(datePrefix).arg(sequenceNumber, 3, 10, QChar('0'));

    // 创建新的会议记录，状态设置为未开始
    Meeting newMeeting;
    newMeeting.mid = newMeetingId;
    newMeeting.subject = subject;
    newMeeting.description = description;
    newMeeting.startTime = startTime;
    newMeeting.endTime = endTime;
    newMeeting.duration = duration;
    newMeeting.numParticipant = numParticipant;
    newMeeting.status = MeetingStatus::NotStarted;  // 设置为未开始状态

    // 注意：计划会议不存储到 m_currentMeeting，只存储到 m_meetings
    m_meetings[newMeeting.mid] = newMeeting;

    // qDebug() << "计划会议已创建并保存到日志: ID=" << newMeeting.mid
    //          << ", 开始时间=" << startTime.toString("yyyy-MM-dd hh:mm:ss");

    // 发出会议更改信号
    emit signal_meetingsChanged();
}

// 发送会议主题指令到当前主机 (0x0701指令)
void MeetingManager::send_MeetingSubjectCommand(const QString& subject)
{
    HostController* currentHost = getCurrentHostController();
    if (currentHost != nullptr) {
        // 将QString转换为UTF-8字节数组
        QByteArray subjectBytes = subject.toLocal8Bit();

        // 创建48字节的payload，不足部分补'\0'
        QByteArray payload(48, '\0');
        int copyLength = qMin(subjectBytes.length(), 48);
        for (int i = 0; i < copyLength; ++i) {
            payload[i] = subjectBytes[i];
        }

        // 调用HostController的查询方法发送0x0701指令
        currentHost->query_hostInformation(0x01, 0x0701, payload);

        // qDebug() << "已发送会议主题到主机:" << subject;
    } else {
        qDebug() << "警告：无法发送会议主题：当前无可用主机";
    }
}

// 重置会议时间或者会议时间归零（结束）
void MeetingManager::end_Meeting(QString mid)
{
    qDebug() << "判断会议事件是否都已结束，如果结束再更新会议状态，保存会议数据";

    if (m_currentMeeting.mid == mid && m_currentMeeting.status == MeetingStatus::InProgress) {
        m_currentMeeting.status = MeetingStatus::Ended;
        m_currentMeeting.endTime = QDateTime::currentDateTime();
        m_meetings[mid] = m_currentMeeting;  // 更新映射中的记录

        // 发射信号
        emit signal_meetingsChanged();
    }
}

// 将会议状态转为字符串
QString MeetingManager::meetingStatusToString(MeetingStatus status) const
{
    switch (status) {
    case MeetingStatus::NotStarted:
        return "未开始";
    case MeetingStatus::InProgress:
        return "进行中";
    case MeetingStatus::Ended:
        return "已结束";
    default:
        return "未知状态";
    }
}

// 获取会议记录给属性
QVariantMap MeetingManager::getter_AllMeetings() const
{
    QVariantMap result;
    for (const auto& pair : m_meetings) {
        QVariantMap meetingData;
        meetingData["mid"] = pair.second.mid;
        meetingData["subject"] = pair.second.subject;
        meetingData["description"] = pair.second.description;
        meetingData["startTime"] = pair.second.startTime;
        meetingData["endTime"] = pair.second.endTime;
        meetingData["status"] = static_cast<int>(pair.second.status);
        result[pair.first] = meetingData;
    }
    return result;
}

// 获取当前进行中会议记录给属性
QVariantMap MeetingManager::getter_CurrentMeeting() const
{
    QVariantMap result;
    if (!m_currentMeeting.mid.isEmpty()) {
        result["mid"] = m_currentMeeting.mid;
        result["subject"] = m_currentMeeting.subject;
        result["description"] = m_currentMeeting.description;
        result["startTime"] = m_currentMeeting.startTime;
        result["endTime"] = m_currentMeeting.endTime;
        result["duration"] = m_currentMeeting.duration;
        result["status"] = static_cast<int>(m_currentMeeting.status);
    }
    return result;
}

// 获取会议状态给属性
QString MeetingManager::getter_CurrentMeetingStatus() const
{
    return meetingStatusToString(m_currentMeeting.status);
}

// 检测是否有会议在进行中
bool MeetingManager::has_ActiveMeeting() const
{
    return !m_currentMeeting.mid.isEmpty() &&
           m_currentMeeting.status == MeetingStatus::InProgress;
}


// ************************** 会议参与者相关函数 ********************************//
// 添加会议参与者
void MeetingManager::addParticipant(quint16 unitId, const QString& name) {
    qDebug() << "添加参加会议人员，未完成";
}

// 移除会议参与者
void MeetingManager::removeParticipant(quint16 unitId) {
    qDebug() << "移除参加会议人员，未完成";
}

// 获取会议参与者信息给属性
QVariantMap MeetingManager::getter_Participants() const {
    qDebug() << "查询参加会议人员名单";
    QVariantMap result;
    for (const auto& pair : m_participants) {
        QVariantMap participantData;
        participantData["pid"] = pair.second.pid;
        participantData["mid"] = pair.second.mid;
        participantData["alias"] = pair.second.alias;
        participantData["role"] = pair.second.role;
        result[pair.first] = participantData;
    }
    return result;
}

// 获取会议参与者人数给属性
int MeetingManager::getter_NumParticipants() const {
    // 这里还要添加，检测有线单元的数量，作为参会人数的代码。目前暂时使用创建会议时输入的人数
    return m_currentMeeting.numParticipant;
}


// ************************** 签到事件相关函数 ********************************//
// 开始新签到 → 清空旧数据 → 创建新事件 → 收集新数据 → 结束签到 → 显示结果
// 创建签到事件，参数duration单位"秒"
void MeetingManager::create_CheckInEvent(int duration)
{
    // 检查当前是否有进行中的会议
    if (m_currentMeeting.mid.isEmpty() || m_currentMeeting.status != MeetingStatus::InProgress) {
        qDebug() << "警告：无法创建签到事件：当前没有进行中的会议";
        return;
    }

    // 检查是否已存在活跃的签到事件
    if (m_currentCheckInEvent.isActive) {
        qDebug() << "警告：无法创建签到事件：已存在进行中的签到事件";
        return;
    }

    // 清空旧的签到结果数据
    QString oldEventId = QString::number(m_currentCheckInEvent.cid);
    if (m_checkInResults.find(oldEventId) != m_checkInResults.end()) {
        m_checkInResults.erase(oldEventId);
    }

    // 创建新的签到事件
    m_currentCheckInEvent = create_CheckInStruct(duration);

    // 初始化签到结果记录
    QString eventId = QString::number(m_currentCheckInEvent.cid);
    m_checkInResults[eventId] = std::map<quint16, QDateTime>();

    // 发送签到指令到主机
    send_CheckInCommand(m_currentCheckInEvent.cid, duration);

    // 发出签到事件变化信号
    emit signal_checkInEventsChanged();  // 通知签到事件已变化
    emit signal_checkInResultsChanged(); // 通知结果已清空
}

// 发送签到指令到当前主机 (0x0702指令)
void MeetingManager::send_CheckInCommand(quint16 cid, int duration)
{
    // 55 10 EF 01 80 02 07 02 00 01 00 00 00 0A 8D AA  (10s)

    HostController* currentHost = getCurrentHostController();
    if (currentHost != nullptr) {
        // 创建6字节的payload
        QByteArray payload(6, '\0');

        // 前2字节为签到事件ID (cid) - 大端序
        payload[0] = static_cast<char>((cid >> 8) & 0xFF); // 高字节在前
        payload[1] = static_cast<char>(cid & 0xFF);        // 低字节在后

        // 后4字节为签到事件时效 (duration) - 大端序
        int sDuration = 60 * duration;
        payload[2] = static_cast<char>((sDuration >> 24) & 0xFF); // 最高字节
        payload[3] = static_cast<char>((sDuration >> 16) & 0xFF); // 第三字节
        payload[4] = static_cast<char>((sDuration >> 8) & 0xFF);  // 第二字节
        payload[5] = static_cast<char>(sDuration & 0xFF);         // 最低字节

        // 调用HostController的查询方法发送0x0702指令
        currentHost->query_hostInformation(0x02, 0x0702, payload);

        // qDebug() << "已发送签到指令到主机: CID=" << cid << ", Duration=" << sDuration << " 秒";
    } else {
        qDebug() << "警告：无法发送会议签到指令：当前无可用主机";
    }
}

// 重置或者归零签到事件时间
void MeetingManager::reset_CheckInEvent(int duration)
{
    qDebug() << "警告：重置或者归零签到事件时间: Duration=" << duration;

    // 检查是否存在当前签到事件
    if (m_currentCheckInEvent.cid == 0 || !m_currentCheckInEvent.isActive) {
        qDebug() << "警告：没有活跃的签到事件";
        return;
    }

    quint16 currentCid = m_currentCheckInEvent.cid;

    if (duration == 0) {
        // 结束签到事件
        m_currentCheckInEvent.isActive = false;
        m_currentCheckInEvent.endTime = QDateTime::currentDateTime();

        // 保存事件到日志(保存方法还未完成）
        save_CheckInEventLog(m_currentCheckInEvent);

        // 发送结束指令, duration=0
        send_CheckInCommand(currentCid, duration);

        // 发出结果变化信号，让界面更新统计
        emit signal_checkInResultsChanged();
    } else {
        // 延长时长
        QDateTime resetTime = QDateTime::currentDateTime();
        m_currentCheckInEvent.endTime = resetTime.addSecs(duration*60);

        // 计算从最初开始的总时长
        m_currentCheckInEvent.duration = m_currentCheckInEvent.startTime.secsTo(m_currentCheckInEvent.endTime);

        // 重新发送签到指令
        send_CheckInCommand(currentCid, duration);
        // qDebug() << "签到事件已延长并重新发送指令: CID=" << currentCid
        //          << ", New Total Duration=" << m_currentCheckInEvent.duration;
    }

    // 发出签到事件变化信号
    emit signal_checkInEventsChanged();
}

// 保存一个签到事件响应（某个单元的指令信号）
void MeetingManager::save_CheckInResult(quint16 cid, quint16 unitId)
{
    // 将 cid 转换为 eventId 字符串格式
    QString eventId = QString::number(cid);

    // 获取当前时间作为签到时间
    QDateTime checkInTime = QDateTime::currentDateTime();

    // 更新签到结果到 m_checkInResults
    m_checkInResults[eventId][unitId] = checkInTime;

    // 发出签到结果变化信号
    emit signal_checkInResultsChanged();
}

// 获取当前签到事件信息给属性
QVariantMap MeetingManager::getter_CurrentCheckInEvent() const
{
    QVariantMap result;
    if (m_currentCheckInEvent.cid != 0) {
        result["cid"] = m_currentCheckInEvent.cid;
        result["mid"] = m_currentCheckInEvent.mid;
        result["startTime"] = m_currentCheckInEvent.startTime;
        result["endTime"] = m_currentCheckInEvent.endTime;
        result["duration"] = m_currentCheckInEvent.duration;
        result["isActive"] = m_currentCheckInEvent.isActive;
    }
    return result;
}

// 获取签到结果给属性
QVariantMap MeetingManager::getter_CurrentCheckInResults() const
{
    QVariantMap result;
    if (m_currentCheckInEvent.cid != 0 && m_currentCheckInEvent.isActive) {
        QString eventId = QString::number(m_currentCheckInEvent.cid);
        auto it = m_checkInResults.find(eventId);
        if (it != m_checkInResults.end()) {
            QVariantMap eventResults;
            for (const auto& unitPair : it->second) {
                eventResults[QString::number(unitPair.first)] = unitPair.second;
            }
            result = eventResults;
        }
    }
    return result;
}

// 获取签到结果汇总给属性
QVariantMap MeetingManager::getter_CurrentCheckInStatistics() const
{
    QVariantMap statistics;

    // 通过函数获取预期参与人数
    int totalParticipants = getter_NumParticipants();

    if (m_currentCheckInEvent.cid != 0) {
        QString eventId = QString::number(m_currentCheckInEvent.cid);
        auto it = m_checkInResults.find(eventId);

        int checkedInCount = 0;
        if (it != m_checkInResults.end()) {
            checkedInCount = static_cast<int>(it->second.size());
        }

        // 如果签到人数超过预期总人数，说明实际参会人数增加了
        int actualTotalParticipants = (checkedInCount > totalParticipants) ?
                                          checkedInCount : totalParticipants;

        int notCheckedInCount = actualTotalParticipants - checkedInCount;

        double checkInPercentage = actualTotalParticipants > 0 ?
                                       (static_cast<double>(checkedInCount) / actualTotalParticipants) * 100.0 : 0.0;

        statistics["checkedInCount"] = checkedInCount;
        statistics["notCheckedInCount"] = notCheckedInCount;
        statistics["totalExpected"] = actualTotalParticipants;
        statistics["checkInPercentage"] = checkInPercentage;
    } else {
        statistics["checkedInCount"] = 0;
        statistics["notCheckedInCount"] = totalParticipants;
        statistics["totalExpected"] = totalParticipants;
        statistics["checkInPercentage"] = 0.0;
    }

    return statistics;
}

// 检测是否有签到事件在进行中
bool MeetingManager::has_ActiveCheckInEvent() const
{
    return m_currentCheckInEvent.isActive;
}

// 保存签到事件到日志文件
void MeetingManager::save_CheckInEventLog(const CheckInEvent& event)
{
    qDebug() << "保存签到事件到日志文件: CID=" << event.cid;
    // 实现日志文件写入逻辑
}


// ************************** 投票事件相关函数 ********************************//
// 创建投票事件
void MeetingManager::create_VotingEvent(int duration, qint16 optionValue1, qint16 optionValue2)
{

    // 检查当前是否有进行中的会议
    if (m_currentMeeting.mid.isEmpty() || m_currentMeeting.status != MeetingStatus::InProgress) {
        qDebug() << "无法创建投票事件：当前没有进行中的会议";
        return;
    }

    // 检查是否已存在活跃的投票事件
    if (m_currentVotingEvent.isActive) {
        qDebug() << "无法创建投票事件：已存在活跃的投票事件";
        return;
    }

    // 清除旧的投票结果数据 - 因为一个时刻只有一个投票事件
    QString oldEventId = QString::number(m_currentVotingEvent.vid);
    if (m_votingResults.find(oldEventId) != m_votingResults.end()) {
        m_votingResults.erase(oldEventId);
    }

    // 创建新的投票事件
    m_currentVotingEvent = create_VotingEventStruct(duration, optionValue1, optionValue2);

    // 初始化新的投票事件Id和新投票结果记录
    QString newEventId = QString::number(m_currentVotingEvent.vid);
    m_votingResults[newEventId] = std::map<quint16, std::pair<QDateTime, int>>();

    // 发送投票指令到主机
    send_VotingEventCommand(m_currentVotingEvent.vid, duration, optionValue1, optionValue2);

    // 发出投票事件变化信号
    emit signal_votingEventsChanged();
    emit signal_votingResultsChanged();

    // qDebug() << "新投票事件创建完成";
}

// 发送投票指令到当前主机 (0x0703指令)
void MeetingManager::send_VotingEventCommand(quint16 vid, int duration, qint16 optionValue1, qint16 optionValue2)
{
    // 55 14 EB 01 80 02 07 03 00 01 00 00 00 0A FF FF 00 01 8D AA
    qDebug() << "发送0x0703指令";

    HostController* currentHost = getCurrentHostController();
    if (currentHost != nullptr) {
        // 创建10字节的payload
        QByteArray payload(10, '\0');

        // 第一二字节为投票事件ID (vid) - 大端序
        payload[0] = static_cast<char>((vid >> 8) & 0xFF); // 高字节在前
        payload[1] = static_cast<char>(vid & 0xFF);        // 低字节在后

        // 第三四五六字节为投票事件时效 (duration) - 大端序
        int sDuration = duration * 60;
        payload[2] = static_cast<char>((sDuration >> 24) & 0xFF); // 最高字节
        payload[3] = static_cast<char>((sDuration >> 16) & 0xFF); // 第三字节
        payload[4] = static_cast<char>((sDuration >> 8) & 0xFF);  // 第二字节
        payload[5] = static_cast<char>(sDuration & 0xFF);         // 最低字节

        // 第七八字节为投票事件选项最小值（有符号数）
        payload[6] = static_cast<char>((optionValue1 >> 8) & 0xFF); // 高字节在前
        payload[7] = static_cast<char>(optionValue1 & 0xFF);        // 低字节在后

        // 第九十字节为投票事件选项最大值（有符号数）
        payload[8] = static_cast<char>((optionValue2 >> 8) & 0xFF); // 高字节在前
        payload[9] = static_cast<char>(optionValue2 & 0xFF);        // 低字节在后

        // 调用HostController的查询方法发送0x0703指令
        currentHost->query_hostInformation(0x02, 0x0703, payload);

        // qDebug() << "已发送投票指令到主机: VID=" << vid << ", Duration=" << duration
        //          << ", Option1=" << optionValue1 << ", Option2=" << optionValue2;
    } else {
        qDebug() << "警告：无法发送会议投票指令：当前无可用主机";
    }
}

// 重置或者归零投票事件时间
void MeetingManager::reset_VotingEvent(quint16 vid, int duration, qint16 optionValue1, qint16 optionValue2)
{
    qDebug() << "重置或者归零投票事件时间: VID=" << vid << ", Duration=" << duration;

    // 检查是否是当前投票事件
    if (m_currentVotingEvent.vid != vid) {
        qDebug() << "警告：未找到投票事件: VID=" << vid;
        return;
    }

    // 检查投票事件是否处于激活状态
    if (!m_currentVotingEvent.isActive) {
        qDebug() << "警告：投票事件未激活，无法重置或结束: VID=" << vid;
        return;
    }

    if (duration == 0) {
        // 结束投票事件 - 只修改状态，绝对不清除数据！
        m_currentVotingEvent.isActive = false;
        m_currentVotingEvent.endTime = QDateTime::currentDateTime();

        // 重新计算实际持续时间（可能提前结束）
        int actualDuration = m_currentVotingEvent.startTime.secsTo(m_currentVotingEvent.endTime);
        m_currentVotingEvent.duration = actualDuration;

        // 保存事件到日志
        save_VotingEventToLog(m_currentVotingEvent);

        // 发送结束指令
        send_VotingEventCommand(vid, duration, optionValue1, optionValue2);

        // 通知投票结果变化
        emit signal_votingResultsChanged();
    } else {
        // 重新设置投票事件时长并重新发送指令
        m_currentVotingEvent.duration = duration;
        m_currentVotingEvent.endTime = m_currentVotingEvent.startTime.addSecs(duration*60);

        // 重新发送投票指令
        send_VotingEventCommand(vid, duration, optionValue1, optionValue2);
    }

    // 发出投票事件变化信号
    emit signal_votingEventsChanged();
}

// 保存一个投票事件响应（某个单元的指令信号）
void MeetingManager::save_VotingResult(quint16 vid, quint16 unitId, qint16 optionId)
{
    // 将 vid 转换为 eventId 字符串格式
    QString eventId = QString::number(vid);

    // 获取当前时间作为投票时间
    QDateTime votingTime = QDateTime::currentDateTime();

    // 检查是否存在该投票事件
    if (m_currentVotingEvent.vid != vid) {
        qDebug() << "警告：投票事件ID不匹配，当前事件ID:" << m_currentVotingEvent.vid;
        // 不保存结果，直接返回
        return;
    }

    // 打印保存前的投票结果数量
    // QString currentEventId = QString::number(m_currentVotingEvent.vid);
    // int beforeCount = 0;
    // if (m_votingResults.find(currentEventId) != m_votingResults.end()) {
    //     beforeCount = static_cast<int>(m_votingResults[currentEventId].size());
    // }
    // qDebug() << "(save_VotingResult)保存投票结果 - 事件ID:" << currentEventId << ", 单元:" << unitId
    //          << ", 选项:" << optionId << ", 当前投票数量:" << beforeCount;

    // 更新投票结果到 m_votingResults
    m_votingResults[eventId][unitId] = std::make_pair(votingTime, static_cast<int>(optionId));

    // 发出投票结果变化信号 - 确保所有相关信号都发出，更新UI
    emit signal_votingResultsChanged();
    //emit signal_votingEventsChanged();
}

// 获取投票事件信息给属性
QVariantMap MeetingManager::getter_CurrentVotingEvent() const
{
    QVariantMap result;
    if (m_currentVotingEvent.vid != 0 && m_currentVotingEvent.isActive) {
        QVariantMap eventMap;
        eventMap["vid"] = m_currentVotingEvent.vid;
        eventMap["mid"] = m_currentVotingEvent.mid;
        eventMap["startTime"] = m_currentVotingEvent.startTime;
        eventMap["endTime"] = m_currentVotingEvent.endTime;
        eventMap["duration"] = m_currentVotingEvent.duration;
        eventMap["isActive"] = m_currentVotingEvent.isActive;
        result[QString::number(m_currentVotingEvent.vid)] = eventMap;
    }
    return result;
}

// 获取投票事件结果给属性
QVariantMap MeetingManager::getter_CurrentVotingResult() const
{
    QVariantMap result;
    for (const auto& eventPair : m_votingResults) {
        QVariantMap eventResults;
        for (const auto& unitPair : eventPair.second) {
            QVariantMap voteData;
            voteData["time"] = unitPair.second.first;
            voteData["option"] = unitPair.second.second;
            eventResults[QString::number(unitPair.first)] = voteData;
        }
        result[eventPair.first] = eventResults;
    }
    return result;
}

// 获取投票事件结果汇总给属性
QVariantMap MeetingManager::getter_CurrentVotingStatistics() const
{
    QVariantMap statistics;

    // 通过函数获取预期参与人数
    int totalParticipants = getter_NumParticipants();

    if (m_currentVotingEvent.vid != 0) {
        QString eventId = QString::number(m_currentVotingEvent.vid);
        auto eventIt = m_votingResults.find(eventId);

        int totalVotes = 0;
        if (eventIt != m_votingResults.end()) {
            totalVotes = static_cast<int>(eventIt->second.size());
        } else {
            qDebug() << "警告：未找到事件" << eventId << "的投票结果数据";
        }

        // 计算投票率
        double votePercentage = totalParticipants > 0 ?
                                    (static_cast<double>(totalVotes) / totalParticipants) * 100.0 : 0.0;

        // 统计各选项的投票数和百分比
        std::map<int, int> optionCounts;

        // 初始化所有选项计数为0
        for (const auto& option : m_currentVotingOptions) {
            optionCounts[static_cast<int>(option.value)] = 0;
        }

        // 统计实际投票结果
        if (eventIt != m_votingResults.end()) {
            for (const auto& unitPair : eventIt->second) {
                int option = unitPair.second.second;
                optionCounts[option]++;
            }
        }

        // 转换为QVariantMap格式，包含数量和百分比
        QVariantMap optionStats;
        for (const auto& countPair : optionCounts) {
            QVariantMap optionDetail;
            optionDetail["count"] = countPair.second;
            double optionPercentage = totalParticipants > 0 ?
                                          (static_cast<double>(countPair.second) / totalParticipants) * 100.0 : 0.0;
            optionDetail["percentage"] = optionPercentage;

            optionStats[QString::number(countPair.first)] = optionDetail;
        }

        statistics["options"] = optionStats;
        statistics["totalVotes"] = totalVotes;
        statistics["totalExpected"] = totalParticipants;
        statistics["votePercentage"] = votePercentage;
    } else {
        statistics["totalVotes"] = 0;
        statistics["totalExpected"] = totalParticipants;
        statistics["votePercentage"] = 0.0;

        // 即使没有投票事件也返回空的选项统计
        QVariantMap optionStats;
        for (const auto& option : m_currentVotingOptions) {
            QVariantMap optionDetail;
            optionDetail["count"] = 0;
            optionDetail["percentage"] = 0.0;
            optionStats[QString::number(static_cast<int>(option.value))] = optionDetail;
        }
        statistics["options"] = optionStats;
    }
    return statistics;
}


// 检测是否有签到事件在进行中
bool MeetingManager::has_ActiveVotingEvent() const
{
    return m_currentVotingEvent.isActive;
}

// 保存投票事件到日志文件
void MeetingManager::save_VotingEventToLog(const VotingEvent& event)
{
    qDebug() << "保存投票事件到日志文件: VID=" << event.vid;
    // 实现日志文件写入逻辑
}

// 设置表决投票选项 (赞成:1, 弃权:0, 反对:-1)
void MeetingManager::set_ReferendumVotingOptions()
{
    m_currentVotingOptions.clear();

    // 按照标准表决投票选项设置
    m_currentVotingOptions.push_back({"赞成", 1});
    m_currentVotingOptions.push_back({"弃权", 0});
    m_currentVotingOptions.push_back({"反对", -1});

    m_isReferendumVoting = true;

    // 发出投票选项变化信号（被设置为表决投票）
    emit signal_votingOptionsChanged();
}

// 设置自定义选项投票选项
void MeetingManager::set_CustomVotingOptions(const QVariantList& options)
{
    m_currentVotingOptions.clear();

    // 按照传入的选项设置，值从1开始递增
    qint16 value = 1;
    for (const auto& optionVariant : options) {
        QVariantMap optionMap = optionVariant.toMap();
        VotingOptions votingOption;
        votingOption.option = optionMap["option"].toString();
        votingOption.value = value++;
        m_currentVotingOptions.push_back(votingOption);
    }

    m_isReferendumVoting = false;

    // 发出投票选项变化信号
    emit signal_votingOptionsChanged();
}

// 获取当前投票选项
QVariantList MeetingManager::get_CurrentVotingOptions() const
{
    QVariantList optionsList;
    for (const auto& option : m_currentVotingOptions) {
        QVariantMap optionMap;
        optionMap["option"] = option.option;
        optionMap["value"] = static_cast<int>(option.value);
        optionsList.append(optionMap);
    }
    return optionsList;
}

// 获取投票选项范围(用作指令参数）
QVariantMap MeetingManager::get_VotingOptionRange() const
{
    QVariantMap range;

    if (m_currentVotingOptions.empty()) {
        range["min"] = 0;
        range["max"] = 0;
        return range;
    }

    qint16 minValue = m_currentVotingOptions[0].value;
    qint16 maxValue = m_currentVotingOptions[0].value;

    for (const auto& option : m_currentVotingOptions) {
        if (option.value < minValue) {
            minValue = option.value;
        }
        if (option.value > maxValue) {
            maxValue = option.value;
        }
    }

    range["min"] = static_cast<int>(minValue);
    range["max"] = static_cast<int>(maxValue);

    return range;
}

// 设置投票模式
void MeetingManager::set_VotingMode(bool isReferendum)
{
    m_isReferendumVoting = isReferendum;

    if (isReferendum) {
        set_ReferendumVotingOptions();
    }
    // 对于自定义选项模式，需要在QML中单独调用 set_CustomVotingOptions
}

// 更新会议持续时间
void MeetingManager::updateDuration()
{
    // 实现更新会议持续时间的逻辑
}

// 格式化时间
QString MeetingManager::formatTime(const QDateTime& time) const
{
    return time.toString("yyyy-MM-dd hh:mm:ss");
}

// 保存会议到日志
void MeetingManager::saveMeetingToLog(const Meeting& meeting)
{
    qDebug() << "保存会议到日志文件: MID=" << meeting.mid;
    // 实现日志文件写入逻辑
}
