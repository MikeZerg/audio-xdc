// source/MeetingManager.h
#ifndef MEETINGMANAGER_H  // 防止头文件重复包含的预处理指令
#define MEETINGMANAGER_H  // 定义MEETINGMANAGER_H宏

#pragma once              // 现代C++防止头文件重复包含的方法
#include <QObject>        // 包含Qt对象基类
#include <QString>        // 包含Qt字符串类
#include <QDateTime>      // 包含Qt日期时间类
#include <vector>         // 包含STL向量容器
#include <map>            // 包含STL映射容器

// 前向声明，避免包含整个HostManager、HostController头文件，减少编译依赖
class HostManager;
class HostController;

// 定义参与者结构体，用于存储参会者信息
struct Participant {
    QString pid;           // 参与者Id
    QString mid;           // 关联会议Id
    QString alias;         // 参与者别名
    QString role;          // 角色(主席、列席、贵宾等)
};

struct CheckInEvent {
    quint16 cid;                                // 签到事件Id
    QString mid;                                // 关联会议Id
    QDateTime startTime;                        // 签到开始时间
    QDateTime endTime;                          // 签到结束时间
    int duration;                               // 签到事件时长(秒)
    bool isActive;                              // 签到是否激活状态
};

struct VotingEvent {
    quint16 vid;                                // 投票事件ID
    QString mid;                                // 关联会议Id
    QString subject;                            // 投票标题
    QDateTime startTime;                        // 投票开始时间
    QDateTime endTime;                          // 投票结束时间
    int duration;                               // 投票事件时长(秒)
    bool isActive;                              // 投票是否激活状态
};

struct VotingOptions {
    QString option;     // 选项文本
    qint16 value;       // 选项值，使用 qint16 支持更大范围
};

enum class MeetingStatus {
    NotStarted,  // 会议未开始
    InProgress,  // 会议进行中
    Ended        // 会议已结束
};

struct Meeting {
    QString mid;            // 会议记录Id，创建会议时自动生成
    QString subject;        // 会议标题，在界面输入
    QString description;    // 会议说明，在界面输入
    QDateTime startTime;    // 会议开始时间，创建会议时自动生成
    QDateTime endTime;      // 会议结束时间，结束会议时自动生成
    int duration;           // 会议时长，记录会议持续时间（单位：秒），在界面输入
    int numParticipant;     // 会议参与人数
    MeetingStatus status;   // 会议状态，在界面由时间控制或者按钮控制
};

class MeetingManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantMap meetingData READ getter_AllMeetings NOTIFY signal_meetingsChanged)                       // 所有会议数据
    Q_PROPERTY(QVariantMap currentMeetingData READ getter_CurrentMeeting NOTIFY signal_meetingsChanged)             // 当前进行中会议数据
    Q_PROPERTY(QString currentMeetingStatus READ getter_CurrentMeetingStatus NOTIFY signal_meetingsChanged)         // 会议状态属性
    Q_PROPERTY(bool has_ActiveMeeting READ has_ActiveMeeting NOTIFY signal_meetingsChanged)

    Q_PROPERTY(QVariantMap participantsLData READ getter_Participants NOTIFY signal_participantsChanged)        // 所有参与者数据
    Q_PROPERTY(int numParticipants READ getter_NumParticipants NOTIFY signal_participantsChanged)               // 总参与人数

    Q_PROPERTY(QVariantMap checkInEventData READ getter_CurrentCheckInEvent NOTIFY signal_checkInEventsChanged)             // 当前签到事件
    Q_PROPERTY(QVariantMap checkInResultData READ getter_CurrentCheckInResults NOTIFY signal_checkInResultsChanged)         // 当前签到事件结果
    Q_PROPERTY(QVariantMap checkInStatistics READ getter_CurrentCheckInStatistics NOTIFY signal_checkInResultsChanged)      // 签到事件结果汇总
    Q_PROPERTY(bool has_ActiveCheckInEvent READ has_ActiveCheckInEvent NOTIFY signal_checkInEventsChanged)                  // 签到事件状态属性

    Q_PROPERTY(QVariantMap votingEventData READ getter_CurrentVotingEvent NOTIFY signal_votingEventsChanged)                // 当前投票事件
    Q_PROPERTY(QVariantMap votingResultData READ getter_CurrentVotingResult NOTIFY signal_votingResultsChanged)             // 当前投票事件结果
    Q_PROPERTY(QVariantMap votingStatistics READ getter_CurrentVotingStatistics NOTIFY signal_votingResultsChanged)         // 投票事件结果汇总
    Q_PROPERTY(QVariantList votingOptionsList READ get_CurrentVotingOptions NOTIFY signal_votingOptionsChanged)             // 当前投票事件选项
    Q_PROPERTY(bool has_ActiveVotingEvent READ has_ActiveVotingEvent NOTIFY signal_votingEventsChanged)                     // 投票事件状态属性

public:
    explicit MeetingManager(QObject *parent = nullptr); // 构造函数

    // 与主机管理相关的接口
    void setHostManager(HostManager* hostManager);      // 设置HostManager引用
    quint8 getCurrentHostAddress() const;               // 获取当前主机地址
    HostController* getCurrentHostController() const;   // 获取当前主机控制器实例


    // 属性读取函数
    QVariantMap getter_AllMeetings() const;         // 获取所有会议数据
    QVariantMap getter_CurrentMeeting() const;      // 获取当前进行中会议数据
    QString getter_CurrentMeetingStatus() const;    // 获取会议状态字符串
    bool has_ActiveMeeting() const;                 // 检测是否有会议在进行中

    QVariantMap getter_Participants() const;    // 获取所有参与者数据
    int getter_NumParticipants() const;         // 获取会议参与总人数

    QVariantMap getter_CurrentCheckInEvent() const;                     // 获取当前签到事件
    QVariantMap getter_CurrentCheckInResults() const;                   // 获取签到结果
    Q_INVOKABLE QVariantMap getter_CurrentCheckInStatistics() const;    // 获取签到事件结果汇总
    bool has_ActiveCheckInEvent() const;                                // 检测当前是否有签到事件在进行中

    QVariantMap getter_CurrentVotingEvent() const;                  // 获取所有投票事件
    QVariantMap getter_CurrentVotingResult() const;                 // 获取投票结果
    Q_INVOKABLE QVariantMap getter_CurrentVotingStatistics() const; // 获取投票事件结果汇总
    bool has_ActiveVotingEvent() const;                             // 检测当前是否有投票事件在进行中

    // 0. 会议中与主机通信的方法
    Q_INVOKABLE void sendMeetingCommandToHost(const QString& command);  // 发送会议命令到当前主机


    // 1. 会议管理功能
    Q_INVOKABLE void start_QuickMeeting(const QString& subject, const QString& description, int duration, int numParticipant);    // 创建新会议，会议记录数据
    Q_INVOKABLE void start_ScheduleMeeting(const QString& subject, const QString& description, QDateTime startTime, int duration, int numParticipant);    // 创建新会议，会议记录数据
    Q_INVOKABLE void end_Meeting(QString mid);                                                 // 重置或置零会议时间
    Q_INVOKABLE void send_MeetingSubjectCommand(const QString& subject);                            // 0x0701(    ) - 发送会议主题到当前主机
    quint16 generate_UniqueEventId();

    // 2. 参会者管理功能
    Q_INVOKABLE void addParticipant(quint16 unitId, const QString& name);   // 添加参会者
    Q_INVOKABLE void removeParticipant(quint16 unitId);                     // 移除参会者

    // 3. 签到管理功能
    Q_INVOKABLE void create_CheckInEvent(int duration);                  // 创建签到事件，签到事件记录数据
    Q_INVOKABLE void reset_CheckInEvent(int resetDuration);       // 重置或置零签到时间
    Q_INVOKABLE void send_CheckInCommand(quint16 cid, int duration);   // 0x0702(    ) - 发送指令开始签到
    void save_CheckInResult(quint16 cid, quint16 unitId);               // 保存签到响应数据

    // 4. 投票管理功能
    Q_INVOKABLE void create_VotingEvent(int duration, qint16 optionValue1, qint16 optionValue2);                   // 创建投票事件，投票事件记录数据
    Q_INVOKABLE void reset_VotingEvent(quint16 vid, int duration, qint16 optionValue1, qint16 optionValue2);       // 重置或置零投票时间
    Q_INVOKABLE void send_VotingEventCommand(quint16 vid, int duration, qint16 optionValue1, qint16 optionValue2);    // 0x0703(    ) - 发送指令开始投票
    void save_VotingResult(quint16 vid, quint16 unitId, qint16 optionId);// 保存投票响应数据

    // 设置表决投票选项
    Q_INVOKABLE void set_ReferendumVotingOptions();
    // 设置自定义选项投票选项
    Q_INVOKABLE void set_CustomVotingOptions(const QVariantList& options);
    // 获取当前投票选项
    Q_INVOKABLE QVariantList get_CurrentVotingOptions() const;
    // 获取投票选项范围
    Q_INVOKABLE QVariantMap get_VotingOptionRange() const;
    // 设置投票模式
    Q_INVOKABLE void set_VotingMode(bool isReferendum);



signals:
    void signal_participantsChanged();          // 🔶 参与人数变化信号，添加删除参会者时发射信号，检测到单元加入、退出时发射信号
    void signal_checkInUpdated();               // 签到更新信号

    void signal_meetingsChanged();              // 🔶 会议数据变化信号。创建会议，会议状态发生变化时发射信号，更新界面

    void signal_checkInEventsChanged();         // 🔶 签到事件变化信号。创建签到事件，签到开始，签到结束时发射信号，更新界面
    void signal_checkInResultsChanged();        // 🔶 签到结果变化信号。收到单元签到指令时发射信号

    void signal_votingEventsChanged();          // 🔶 投票事件变化信号。创建投票事件，投票开始，投票结束时发射信号，更新界面
    void signal_votingResultsChanged();         // 🔶 投票结果变化信号。收到单元投票指令时发射信号
    void signal_votingOptionsChanged();         // 🔶 投票选项变化信号

    void signal_participantAdded(quint16 unitId);       // 🔶 参会者添加信号
    void signal_participantRemoved(quint16 unitId);     // 🔶 参会者移除信号

    void signal_durationChanged();                      // 🔶 会议持续时间变更信号


private:
    HostManager* m_hostManager;                         // HostManager指针，用于获取当前主机信息
    Meeting m_currentMeeting;                           // 当前会议数据
    std::map<QString, Meeting> m_meetings;              // 所有会议数据
    QString meetingStatusToString(MeetingStatus status) const;  // 会议状态转字符串

    CheckInEvent m_currentCheckInEvent;                 // 当前签到事件
    // std::map<QString, CheckInEvent> m_checkInEvents;    // 所有签到事件
    std::map<QString, std::map<quint16, QDateTime>> m_checkInResults;   // eventId -> {unitId -> checkInTime} // 签到事件结果

    VotingEvent m_currentVotingEvent;                           // 当前投票事件
    // std::map<QString, VotingEvent> m_votingEvents;              // 所有投票事件
    std::map<QString, std::map<quint16, std::pair<QDateTime, int>>> m_votingResults;    // 投票结果: eventId -> {unitId -> {time, option}}

    std::vector<VotingOptions> m_currentVotingOptions;   // 当前投票选项
    bool m_isReferendumVoting;                          // 是否为表决投票模式

    std::map<QString, Participant> m_participants;      // 所有参与者数据

    QString m_duration;                  // 会议持续时间
    quint16 m_globalEventId;             // 运行时全局ID，避免不同会议中相同的事件ID，造成数据覆盖错误

    void updateDuration();                              // 更新会议持续时间
    QString formatTime(const QDateTime& time) const;    // 格式化时间

    // 辅助方法， 创建预定义的签到事件
    CheckInEvent create_CheckInStruct(int duration);   // 创建新的签到事件
    // 辅助方法， 创建预定义的投票事件
    VotingEvent create_VotingEventStruct(int duration, qint16 optionValue1, qint16 optionValue2); // 创建新的投票事件

    // 日志保存函数
    void saveMeetingToLog(const Meeting& meeting);          // 保存会议到日志
    void save_CheckInEventLog(const CheckInEvent& event);  // 保存签到事件到日志
    void save_VotingEventToLog(const VotingEvent& event);    // 保存投票事件到日志

};

#endif // MEETINGMANAGER_H
