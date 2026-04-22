// code/core/MeetingManager.h
#ifndef MEETINGMANAGER_H
#define MEETINGMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QList>
#include <map>
#include <vector>


// 前向声明
class HostManager;

// ==================== 枚举与数据结构定义 ====================
/**
 * @brief 会议状态枚举
 */
enum class MeetingStatus {
    NotStarted,   // 未开始（计划会议）
    InProgress,   // 进行中
    Ended         // 已结束
};

/**
 * @brief 签到事件结构
 */
struct CheckInEvent {
    quint16 cid = 0;                  // 签到事件ID
    QDateTime startTime;              // 开始时间
    QDateTime endTime;                // 结束时间
    int durationSecs = 0;             // 时长(秒)
    bool isActive = false;            // 是否激活

    // 结果数据：unitId -> 签到时间
    std::map<quint16, QDateTime> results;

    void reset() {
        cid = 0;
        isActive = false;
        results.clear();
    }

    QVariantMap toVariantMap(bool includeResults = false) const {
        QVariantMap map;
        map["cid"] = cid;
        map["isActive"] = isActive;
        map["startTime"] = startTime;
        map["endTime"] = endTime;
        map["checkedInCount"] = static_cast<int>(results.size());

        if (includeResults) {
            QVariantMap resMap;
            for (const auto& pair : results) {
                resMap[QString::number(pair.first)] = pair.second.toString("hh:mm:ss");
            }
            map["results"] = resMap;
        }
        return map;
    }
};

/**
 * @brief 投票选项结构
 */
struct VotingOption {
    QString label;    // 选项标签 (如: "赞成")
    qint16 value;     // 选项值 (如: 1)
};

/**
 * @brief 投票事件结构
 */
struct VotingEvent {
    quint16 vid = 0;                  // 投票事件ID
    QString subject;                  // 投票主题
    QDateTime startTime;              // 开始时间
    QDateTime endTime;                // 结束时间
    int durationSecs = 0;             // 时长(秒)
    bool isActive = false;            // 是否激活
    qint16 optionMin = 0;             // 选项最小值
    qint16 optionMax = 0;             // 选项最大值
    std::vector<VotingOption> options; // 选项列表

    // 结果数据：unitId -> {time, optionValue}
    std::map<quint16, std::pair<QDateTime, int>> results;

    void reset() {
        vid = 0;
        isActive = false;
        results.clear();
        options.clear();
    }

    QVariantMap toVariantMap(bool includeResults = false) const {
        QVariantMap map;
        map["vid"] = vid;
        map["subject"] = subject;
        map["isActive"] = isActive;
        map["totalVotes"] = static_cast<int>(results.size());

        QVariantList opts;
        for (const auto& opt : options) {
            QVariantMap o;
            o["label"] = opt.label;
            o["value"] = opt.value;
            opts.append(o);
        }
        map["options"] = opts;

        if (includeResults) {
            QVariantMap resMap;
            for (const auto& entry : results) {
                // 1. 显式获取 key 和 value，避免直接操作 pair 导致的歧义
                quint16 unitId = entry.first;
                QDateTime time = entry.second.first;
                int optionVal = entry.second.second;

                // 2. 创建嵌套 Map
                QVariantMap detailMap;
                detailMap["time"] = time.toString("hh:mm:ss");
                detailMap["option"] = optionVal;

                // 3. 存入结果集
                resMap.insert(QString::number(unitId), detailMap);
            }
            map["results"] = resMap;
        }
        return map;
    }
};

/**
 * @brief 聚合的会议记录结构体
 */
struct MeetingRecord {
    QString mid;                      // 会议ID
    QString subject;                  // 会议主题
    QString description;              // 会议描述
    QDateTime startTime;              // 开始时间
    QDateTime endTime;                // 结束时间
    int durationSecs = 0;             // 总时长(秒)
    MeetingStatus status = MeetingStatus::NotStarted;

    int expectedParticipants = 0;     // 预期人数(参考用)

    // === 当前活跃事件 (同一时刻各自唯一) ===
    CheckInEvent currentCheckIn;      // 当前正在进行的签到
    VotingEvent currentVoting;        // 当前正在进行的投票

    // === 历史事件归档 (支持多次事件回溯) ===
    QList<CheckInEvent> checkInHistory;
    QList<VotingEvent> votingHistory;

    void reset() {
        mid.clear();
        subject.clear();
        description.clear();
        status = MeetingStatus::NotStarted;
        expectedParticipants = 0;

        currentCheckIn.reset();
        currentVoting.reset();

        checkInHistory.clear();
        votingHistory.clear();
    }

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["mid"] = mid;
        map["subject"] = subject;
        map["status"] = static_cast<int>(status);
        map["startTime"] = startTime;

        // 暴露当前活跃状态
        map["currentCheckIn"] = currentCheckIn.toVariantMap();
        map["currentVoting"] = currentVoting.toVariantMap();

        // 暴露历史记录摘要
        map["checkInHistoryCount"] = checkInHistory.size();
        map["votingHistoryCount"] = votingHistory.size();

        return map;
    }
};


// ==================== MeetingManager 类定义 ====================
class MeetingManager : public QObject
{
    Q_OBJECT

    // ==================== QML 属性暴露 ====================
    Q_PROPERTY(QVariantMap currentMeeting READ getCurrentMeetingData NOTIFY signal_meetingUpdated)
    Q_PROPERTY(QList<uint8_t> meetingHosts READ getMeetingHosts WRITE setMeetingHosts NOTIFY signal_meetingHostsChanged)

public:
    explicit MeetingManager(QObject* parent = nullptr);
    ~MeetingManager();

    // ==================== 初始化接口 ====================
    void setHostManager(HostManager* manager);

    // ==================== 会议主机管理 ====================
    Q_INVOKABLE void setMeetingHosts(const QList<uint8_t>& addresses);
    QList<uint8_t> getMeetingHosts() const { return m_meetingHosts; }

    // ==================== 会议管理功能 ====================
    Q_INVOKABLE void createQuickMeeting(const QString& subject,
                                        const QString& description,
                                        int durationMinutes);
    Q_INVOKABLE void createScheduledMeeting(const QString& subject,
                                            const QString& description,
                                            QDateTime startTime,
                                            int durationMinutes);
    Q_INVOKABLE void startCurrentMeeting();
    Q_INVOKABLE void endCurrentMeeting();

    // ==================== 签到管理功能 ====================
    Q_INVOKABLE void createCheckInEvent(int durationMinutes);
    Q_INVOKABLE void resetCheckInEvent(int durationMinutes);

    // ==================== 投票管理功能 ====================
    Q_INVOKABLE void createReferendumVoting(int durationMinutes);
    Q_INVOKABLE void createCustomVoting(int durationMinutes, const QVariantList& options);
    Q_INVOKABLE void resetVotingEvent(int durationMinutes);

    // ==================== 数据查询接口 ====================
    QVariantMap getCurrentMeetingData() const;
    Q_INVOKABLE int getTotalParticipantCount() const;

    // ==================== 历史事件查询接口 ====================
    Q_INVOKABLE QVariantList getCheckInHistory() const;
    Q_INVOKABLE QVariantList getVotingHistory() const;
    Q_INVOKABLE QVariantMap getCheckInHistoryDetail(int index) const;
    Q_INVOKABLE QVariantMap getVotingHistoryDetail(int index) const;

signals:
    void signal_meetingUpdated();
    void signal_meetingHostsChanged();
    void signal_warning(const QString& msg);

private slots:
    /**
     * @brief 处理 HostManager 路由过来的会议命令响应 (分支3)
     */
    void onMeetingCommandResponse(uint8_t hostAddress, uint16_t command, const QByteArray& payload);

private:
    // ==================== 内部辅助函数 ====================
    bool sendToMultipleHosts(uint16_t command, const QByteArray& payload);
    void parseCheckInResponse(uint8_t hostAddr, const QByteArray& payload);
    void parseVotingResponse(uint8_t hostAddr, const QByteArray& payload);

    // 事件归档辅助
    void archiveCurrentCheckIn();
    void archiveCurrentVoting();

    // 生成唯一ID
    quint16 generateEventId();
    QString generateMeetingId();

private:
    HostManager* m_hostManager = nullptr;
    MeetingRecord m_currentMeeting;       // 核心状态：聚合的当前会议记录
    QList<uint8_t> m_meetingHosts;        // 关联的主机地址列表
    quint16 m_globalEventId = 1;          // 全局事件ID计数器
};

#endif // MEETINGMANAGER_H