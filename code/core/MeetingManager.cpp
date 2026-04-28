// code/core/MeetingManager.cpp
#include "MeetingManager.h"
#include "HostManager.h"
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// ==================== 构造与析构 ====================
MeetingManager::MeetingManager(QObject* parent)
    : QObject(parent)
    , m_hostManager(nullptr)
    , m_globalEventId(1)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(2000);
    connect(&m_saveTimer, &QTimer::timeout, this, [this]() {
        saveMeetingsToJson();
    });
    qDebug() << "MeetingManager 构建成功";
}

MeetingManager::~MeetingManager()
{
}

void MeetingManager::setHostManager(HostManager* manager)
{
    m_hostManager = manager;
    if (m_hostManager) {
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
    saveMeetingsToJson();
}

void MeetingManager::createScheduledMeeting(const QString& subject,
                                            const QString& description,
                                            QDateTime startTime,
                                            int durationMinutes,
                                            int expectedParticipants)
{
    m_currentMeeting.reset();
    m_currentMeeting.mid = generateMeetingId();
    m_currentMeeting.subject = subject;
    m_currentMeeting.description = description;
    m_currentMeeting.startTime = startTime;
    m_currentMeeting.durationSecs = durationMinutes * 60;
    m_currentMeeting.expectedParticipants = expectedParticipants;
    m_currentMeeting.status = MeetingStatus::NotStarted;

    m_currentMeeting.endTime = startTime.addSecs(durationMinutes * 60);
    m_currentMeeting.hostAddresses = m_meetingHosts;

    qDebug() << "MeetingManager: 计划会议已创建 - ID:" << m_currentMeeting.mid;
    emit signal_meetingUpdated();
    saveMeetingsToJson();
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
    saveMeetingsToJson();
}

void MeetingManager::endCurrentMeeting()
{
    if (m_currentMeeting.status != MeetingStatus::InProgress) return;

    if (m_currentMeeting.currentCheckIn.isActive) archiveCurrentCheckIn();
    if (m_currentMeeting.currentVoting.isActive) archiveCurrentVoting();

    m_currentMeeting.endTime = QDateTime::currentDateTime();
    m_currentMeeting.status = MeetingStatus::Ended;

    qDebug() << "MeetingManager: 会议已结束 - ID:" << m_currentMeeting.mid;
    emit signal_meetingUpdated();
    saveMeetingsToJson();
}

void MeetingManager::setExpectedParticipants(int count)
{
    if (count > 0) {
        m_currentMeeting.expectedParticipants = count;
        emit signal_meetingUpdated();
        m_saveTimer.start();
    }
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

    if (m_currentMeeting.currentCheckIn.isActive) {
        archiveCurrentCheckIn();
    }

    m_currentMeeting.currentCheckIn.cid = generateEventId();
    m_currentMeeting.currentCheckIn.startTime = QDateTime::currentDateTime();
    m_currentMeeting.currentCheckIn.durationSecs = durationMinutes * 60;
    m_currentMeeting.currentCheckIn.isActive = true;
    m_currentMeeting.currentCheckIn.totalExpected = m_currentMeeting.expectedParticipants;

    QByteArray payload;
    payload.append(static_cast<char>(durationMinutes));
    sendToMultipleHosts(0x0701, payload);

    emit signal_meetingUpdated();
}

void MeetingManager::resetCheckInEvent(int durationMinutes)
{
    if (!m_currentMeeting.currentCheckIn.isActive) return;

    if (durationMinutes == 0) {
        archiveCurrentCheckIn();
    } else {
        m_currentMeeting.currentCheckIn.durationSecs = durationMinutes * 60;
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

    // 判断是否为表决投票
    bool isReferendum = true;
    for (const auto& optVar : options) {
        QVariantMap optMap = optVar.toMap();
        int val = optMap["value"].toInt();
        if (val != -1 && val != 0 && val != 1) {
            isReferendum = false;
        }
        VotingOption vo;
        vo.label = optMap["label"].toString();
        vo.value = val;
        vo.count = 0;
        m_currentMeeting.currentVoting.options.push_back(vo);
    }
    m_currentMeeting.currentVoting.type = isReferendum ? VotingType::Referendum : VotingType::Custom;

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
        sendToMultipleHosts(0x0704, QByteArray());
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
    return qMax(1, m_currentMeeting.expectedParticipants);
}

QVariantList MeetingManager::getCheckInHistory() const
{
    QVariantList list;
    for (const auto& event : m_currentMeeting.checkInHistory) {
        list.append(event.toVariantMap(false));
    }
    return list;
}

QVariantList MeetingManager::getVotingHistory() const
{
    QVariantList list;
    for (const auto& event : m_currentMeeting.votingHistory) {
        list.append(event.toVariantMap(false));
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
    if (!m_meetingHosts.contains(hostAddress)) return;

    switch (command) {
    case 0x0701:
    case 0x0702:
        parseCheckInResponse(hostAddress, payload);
        break;
    case 0x0703:
    case 0x0704:
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
        if (m_hostManager->sendProtocolFrame(addr, 0x80, 0x01, command, payload)) {
            success = true;
            QThread::msleep(10);
        }
    }
    return success;
}

void MeetingManager::parseCheckInResponse(uint8_t hostAddr, const QByteArray& payload)
{
    if (!m_currentMeeting.currentCheckIn.isActive) return;

    if (payload.size() >= 2) {
        quint16 unitId = (static_cast<quint8>(payload[0]) << 8) | static_cast<quint8>(payload[1]);
        m_currentMeeting.currentCheckIn.results[unitId] = QDateTime::currentDateTime();
        emit signal_meetingUpdated();
        m_saveTimer.start();
    }
}

void MeetingManager::parseVotingResponse(uint8_t hostAddr, const QByteArray& payload)
{
    if (!m_currentMeeting.currentVoting.isActive) return;

    if (payload.size() >= 3) {
        quint16 unitId = (static_cast<quint8>(payload[0]) << 8) | static_cast<quint8>(payload[1]);
        int optionVal = static_cast<int8_t>(payload[2]);

        m_currentMeeting.currentVoting.results[unitId] = std::make_pair(QDateTime::currentDateTime(), optionVal);

        // 更新选项计数
        for (auto& opt : m_currentMeeting.currentVoting.options) {
            if (opt.value == optionVal) {
                opt.count++;
                break;
            }
        }

        emit signal_meetingUpdated();
        m_saveTimer.start();
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
    m_currentMeeting.currentVoting.calculateOptionCounts();
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

// ==================== 持久化与查询接口 ====================

bool MeetingManager::loadMeetingsFromJson(const QString& filePath)
{
    QString actualPath = filePath;
    if (actualPath.isEmpty()) {
#ifdef QT_DEBUG
        actualPath = "data/meeting_data.json";
#else
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        actualPath = dataDir + "/meeting_data.json";
#endif
    }

    QFile file(actualPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开会议数据文件:" << actualPath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    m_allMeetingsCache.clear();

    auto loadArray = [&](const QJsonArray& arr) {
        for (const auto& val : arr) {
            m_allMeetingsCache.append(jsonToRecord(val.toObject()));
        }
    };

    loadArray(root["scheduledMeetings"].toArray());
    loadArray(root["historicalMeetings"].toArray());

    emit signal_meetingUpdated();
    return true;
}

bool MeetingManager::saveMeetingsToJson(const QString& filePath)
{
    QString actualPath = filePath;
    if (actualPath.isEmpty()) {
#ifdef QT_DEBUG
        actualPath = "data/meeting_data.json";
#else
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        actualPath = dataDir + "/meeting_data.json";
#endif
    }

    QFileInfo fi(actualPath);
    QDir dir(fi.absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "[ERROR] 无法创建数据目录:" << fi.absolutePath();
            return false;
        }
    }

    QJsonObject root;
    QJsonArray scheduledArr, historicalArr;

    if (!m_currentMeeting.mid.isEmpty()) {
        bool found = false;
        for (int i = 0; i < m_allMeetingsCache.size(); ++i) {
            if (m_allMeetingsCache[i].mid == m_currentMeeting.mid) {
                m_allMeetingsCache[i] = m_currentMeeting;
                found = true;
                break;
            }
        }
        if (!found) m_allMeetingsCache.append(m_currentMeeting);
    }

    for (const auto& rec : m_allMeetingsCache) {
        QJsonObject obj = recordToJson(rec);
        if (rec.status == MeetingStatus::Ended) {
            historicalArr.append(obj);
        } else {
            scheduledArr.append(obj);
        }
    }

    root["scheduledMeetings"] = scheduledArr;
    root["historicalMeetings"] = historicalArr;

    QString tempPath = actualPath + ".tmp";
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ERROR] 无法打开临时文件进行写入:" << tempPath;
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    if (QFile::exists(actualPath)) {
        QFile::remove(actualPath);
    }

    if (!QFile::rename(tempPath, actualPath)) {
        qWarning() << "[ERROR] 重命名文件失败:" << tempPath;
        return false;
    }

    qDebug() << "[SUCCESS] 会议数据已成功保存到:" << actualPath;
    return true;
}

QVariantList MeetingManager::getScheduledMeetings() const
{
    QVariantList list;
    for (const auto& rec : m_allMeetingsCache) {
        if (rec.status != MeetingStatus::Ended) {
            list.append(rec.toVariantMap());
        }
    }
    return list;
}

QVariantList MeetingManager::getHistoricalMeetings() const
{
    QVariantList list;
    for (const auto& rec : m_allMeetingsCache) {
        if (rec.status == MeetingStatus::Ended) {
            list.append(rec.toVariantMap());
        }
    }
    return list;
}

bool MeetingManager::startScheduledMeeting(const QString& mid)
{
    for (const auto& rec : m_allMeetingsCache) {
        if (rec.mid == mid && rec.status != MeetingStatus::Ended) {
            m_currentMeeting = rec;
            m_currentMeeting.status = MeetingStatus::InProgress;
            m_currentMeeting.startTime = QDateTime::currentDateTime();

            setMeetingHosts(m_currentMeeting.hostAddresses);

            for (auto& cached : m_allMeetingsCache) {
                if (cached.mid == mid) cached = m_currentMeeting;
            }
            saveMeetingsToJson();
            emit signal_meetingUpdated();
            return true;
        }
    }
    return false;
}

// ==================== JSON 序列化辅助实现 ====================

QString MeetingManager::statusToString(MeetingStatus status)
{
    switch (status) {
    case MeetingStatus::NotStarted: return "NotStarted";
    case MeetingStatus::InProgress: return "InProgress";
    case MeetingStatus::Ended: return "Ended";
    default: return "Unknown";
    }
}

MeetingStatus MeetingManager::stringToStatus(const QString& str)
{
    if (str == "NotStarted") return MeetingStatus::NotStarted;
    if (str == "InProgress") return MeetingStatus::InProgress;
    if (str == "Ended") return MeetingStatus::Ended;
    return MeetingStatus::NotStarted;
}

QString MeetingManager::votingTypeToString(VotingType type)
{
    switch (type) {
    case VotingType::Referendum: return "referendum";
    case VotingType::Custom: return "custom";
    default: return "referendum";
    }
}

VotingType MeetingManager::stringToVotingType(const QString& str)
{
    if (str == "custom") return VotingType::Custom;
    return VotingType::Referendum;
}

MeetingRecord MeetingManager::jsonToRecord(const QJsonObject& obj)
{
    MeetingRecord record;
    record.mid = obj["mid"].toString();
    record.subject = obj["subject"].toString();
    record.description = obj["description"].toString();
    record.startTime = QDateTime::fromString(obj["startTime"].toString(), Qt::ISODate);
    record.endTime = QDateTime::fromString(obj["endTime"].toString(), Qt::ISODate);
    record.durationSecs = obj["durationSecs"].toInt();
    record.expectedParticipants = obj["expectedParticipants"].toInt();
    record.status = stringToStatus(obj["status"].toString());

    QJsonArray hosts = obj["hostAddresses"].toArray();
    for (const auto& h : hosts) {
        record.hostAddresses.append(static_cast<uint8_t>(h.toInt()));
    }

    // 解析签到历史
    QJsonArray checkInArray = obj["checkInHistory"].toArray();
    for (const auto& item : checkInArray) {
        QJsonObject checkInObj = item.toObject();
        CheckInEvent event;
        event.cid = checkInObj["cid"].toInt();
        event.startTime = QDateTime::fromString(checkInObj["startTime"].toString(), Qt::ISODate);
        event.endTime = QDateTime::fromString(checkInObj["endTime"].toString(), Qt::ISODate);
        event.durationSecs = checkInObj["durationSecs"].toInt();
        event.totalExpected = checkInObj["totalExpected"].toInt();
        // 注意：checkedInCount 是动态计算的，不需要存储
        // results 在历史记录中清空
        record.checkInHistory.append(event);
    }

    // 解析投票历史
    QJsonArray votingArray = obj["votingHistory"].toArray();
    for (const auto& item : votingArray) {
        QJsonObject votingObj = item.toObject();
        VotingEvent event;
        event.vid = votingObj["vid"].toInt();
        event.subject = votingObj["subject"].toString();
        event.startTime = QDateTime::fromString(votingObj["startTime"].toString(), Qt::ISODate);
        event.endTime = QDateTime::fromString(votingObj["endTime"].toString(), Qt::ISODate);
        event.durationSecs = votingObj["durationSecs"].toInt();
        event.type = stringToVotingType(votingObj["type"].toString());
        // 注意：totalVotes 是动态计算的，不需要存储

        QJsonArray optionsArray = votingObj["options"].toArray();
        for (const auto& opt : optionsArray) {
            QJsonObject optObj = opt.toObject();
            VotingOption vo;
            vo.label = optObj["label"].toString();
            vo.value = optObj["value"].toInt();
            vo.count = optObj["count"].toInt();
            event.options.push_back(vo);
        }
        // results 在历史记录中清空
        record.votingHistory.append(event);
    }

    return record;
}

QJsonObject MeetingManager::recordToJson(const MeetingRecord& record)
{
    QJsonObject obj;
    obj["mid"] = record.mid;
    obj["subject"] = record.subject;
    obj["description"] = record.description;
    obj["startTime"] = record.startTime.toString(Qt::ISODate);
    obj["endTime"] = record.endTime.toString(Qt::ISODate);
    obj["durationSecs"] = record.durationSecs;
    obj["expectedParticipants"] = record.expectedParticipants;
    obj["status"] = statusToString(record.status);

    QJsonArray hosts;
    for (uint8_t addr : record.hostAddresses) {
        hosts.append(static_cast<int>(addr));
    }
    obj["hostAddresses"] = hosts;

    // 序列化签到历史
    QJsonArray checkInArray;
    for (const auto& event : record.checkInHistory) {
        QJsonObject checkInObj;
        checkInObj["cid"] = event.cid;
        checkInObj["startTime"] = event.startTime.toString(Qt::ISODate);
        checkInObj["endTime"] = event.endTime.toString(Qt::ISODate);
        checkInObj["durationSecs"] = event.durationSecs;
        checkInObj["checkedInCount"] = static_cast<int>(event.results.size());
        checkInObj["totalExpected"] = event.totalExpected;
        checkInArray.append(checkInObj);
    }
    obj["checkInHistory"] = checkInArray;

    // 序列化投票历史
    QJsonArray votingArray;
    for (const auto& event : record.votingHistory) {
        QJsonObject votingObj;
        votingObj["vid"] = event.vid;
        votingObj["subject"] = event.subject;
        votingObj["startTime"] = event.startTime.toString(Qt::ISODate);
        votingObj["endTime"] = event.endTime.toString(Qt::ISODate);
        votingObj["durationSecs"] = event.durationSecs;
        votingObj["totalVotes"] = static_cast<int>(event.results.size());
        votingObj["type"] = votingTypeToString(event.type);

        QJsonArray optionsArray;
        for (const auto& opt : event.options) {
            QJsonObject optObj;
            optObj["label"] = opt.label;
            optObj["value"] = opt.value;
            optObj["count"] = opt.count;
            optionsArray.append(optObj);
        }
        votingObj["options"] = optionsArray;
        votingArray.append(votingObj);
    }
    obj["votingHistory"] = votingArray;

    return obj;
}