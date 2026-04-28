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
#include <QTimer> // [新增] 引入定时器


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
 * @brief 投票类型枚举
 */
enum class VotingType {
    Referendum,   // 表决投票（赞成/弃权/反对）
    Custom        // 自定义投票
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
    int totalExpected = 0;            // 应到人数

    // 结果数据：unitId -> 签到时间
    std::map<quint16, QDateTime> results;

    void reset() {
        cid = 0;
        isActive = false;
        totalExpected = 0;
        results.clear();
    }

    QVariantMap toVariantMap(bool includeResults = false) const {
        QVariantMap map;
        map["cid"] = cid;
        map["isActive"] = isActive;
        map["startTime"] = startTime;
        map["endTime"] = endTime;
        map["durationSecs"] = durationSecs;
        map["checkedInCount"] = static_cast<int>(results.size());
        map["totalExpected"] = totalExpected;

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
    int count = 0;    // 得票数（用于显示）
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
    VotingType type = VotingType::Referendum;  // 投票类型
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

    // 计算每个选项的得票数
    void calculateOptionCounts() {
        for (auto& opt : options) {
            opt.count = 0;
        }
        for (const auto& entry : results) {
            int optionVal = entry.second.second;
            for (auto& opt : options) {
                if (opt.value == optionVal) {
                    opt.count++;
                    break;
                }
            }
        }
    }

    QVariantMap toVariantMap(bool includeResults = false) const {
        QVariantMap map;
        map["vid"] = vid;
        map["subject"] = subject;
        map["isActive"] = isActive;
        map["startTime"] = startTime;
        map["endTime"] = endTime;
        map["durationSecs"] = durationSecs;
        map["totalVotes"] = static_cast<int>(results.size());
        map["type"] = (type == VotingType::Referendum) ? "referendum" : "custom";

        QVariantList opts;
        for (const auto& opt : options) {
            QVariantMap o;
            o["label"] = opt.label;
            o["value"] = opt.value;
            o["count"] = opt.count;
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
    QList<uint8_t> hostAddresses;     // [新增] 关联的主机地址列表

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
        hostAddresses.clear();
        startTime = QDateTime();
        endTime = QDateTime();
        durationSecs = 0;

        currentCheckIn.reset();
        currentVoting.reset();

        checkInHistory.clear();
        votingHistory.clear();
    }

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["mid"] = mid;
        map["subject"] = subject;
        map["description"] = description;
        map["status"] = static_cast<int>(status);
        map["startTime"] = startTime;
        map["endTime"] = endTime;
        map["durationSecs"] = durationSecs;
        map["expectedParticipants"] = expectedParticipants;

        // [新增] 转换主机地址列表为 QML 可用的格式
        QVariantList hosts;
        for (uint8_t addr : hostAddresses) {
            hosts.append(static_cast<int>(addr));
        }
        map["hostAddresses"] = hosts;

        // 暴露当前活跃状态
        map["currentCheckIn"] = currentCheckIn.toVariantMap();
        map["currentVoting"] = currentVoting.toVariantMap();

        // [修改] 暴露完整的历史记录，而不是仅计数
        QVariantList checkInList;
        for (const auto& event : checkInHistory) {
            checkInList.append(event.toVariantMap(true));
        }
        map["checkInHistory"] = checkInList;

        QVariantList votingList;
        for (const auto& event : votingHistory) {
            votingList.append(event.toVariantMap(true));
        }
        map["votingHistory"] = votingList;

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

    // [新增] 暴露会议列表属性，当 signal_meetingUpdated 触发时，QML 会自动刷新
    Q_PROPERTY(QVariantList scheduledMeetings READ getScheduledMeetings NOTIFY signal_meetingUpdated)
    Q_PROPERTY(QVariantList historicalMeetings READ getHistoricalMeetings NOTIFY signal_meetingUpdated)

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
                                            int durationMinutes,
                                            int expectedParticipants = 0);

    Q_INVOKABLE void startCurrentMeeting();
    Q_INVOKABLE void endCurrentMeeting();

    Q_INVOKABLE void setExpectedParticipants(int count);
    Q_INVOKABLE int getExpectedParticipants() const { return m_currentMeeting.expectedParticipants; }

    // ==================== 签到管理功能 ====================
    Q_INVOKABLE void createCheckInEvent(int durationMinutes);
    Q_INVOKABLE void resetCheckInEvent(int durationMinutes);

    // ==================== 投票管理功能 ====================
    Q_INVOKABLE void createReferendumVoting(int durationMinutes);
    Q_INVOKABLE void createCustomVoting(int durationMinutes, const QVariantList& options);
    Q_INVOKABLE void resetVotingEvent(int durationMinutes);

    // ==================== 数据持久化与查询接口 [新增] ====================

    /**
     * @brief 从 JSON 文件加载所有会议数据到内存缓存
     * @param filePath 文件路径（可选，默认根据编译模式自动选择）
     */
    Q_INVOKABLE bool loadMeetingsFromJson(const QString& filePath = "");

    /**
     * @brief 将内存缓存同步保存到 JSON 文件 (原子写入)
     * @param filePath 文件路径（可选，默认根据编译模式自动选择）
     */
    Q_INVOKABLE bool saveMeetingsToJson(const QString& filePath = "");

    /**
     * @brief 获取预定会议列表 (供 QML ListView 使用)
     */
    Q_INVOKABLE QVariantList getScheduledMeetings() const;

    /**
     * @brief 获取历史会议列表 (供 QML ListView 使用)
     */
    Q_INVOKABLE QVariantList getHistoricalMeetings() const;

    /**
     * @brief 根据 ID 启动一个预定会议，并将其设为当前活跃会议
     */
    Q_INVOKABLE bool startScheduledMeeting(const QString& mid);

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

    // ==================== JSON 序列化辅助 [新增] ====================
    static MeetingRecord jsonToRecord(const QJsonObject& obj);
    static QJsonObject recordToJson(const MeetingRecord& record);
    static QString statusToString(MeetingStatus status);
    static MeetingStatus stringToStatus(const QString& str);
    static QString votingTypeToString(VotingType type);
    static VotingType stringToVotingType(const QString& str);

private:
    HostManager* m_hostManager = nullptr;
    MeetingRecord m_currentMeeting;       // 核心状态：聚合的当前会议记录
    QList<uint8_t> m_meetingHosts;        // 关联的主机地址列表
    quint16 m_globalEventId = 1;          // 全局事件ID计数器

    QList<MeetingRecord> m_allMeetingsCache; // [新增] 全量会议数据缓存
    QTimer m_saveTimer;                   // [新增] 防抖保存定时器
};

#endif // MEETINGMANAGER_H