#ifndef HOSTMANAGER_H
#define HOSTMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMap>
#include <QList>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QQmlPropertyMap>
#include <QElapsedTimer>
#include "ProtocolProcessor.h"

// 前向声明 - 避免循环依赖
class ConnectionFactory;
class XdcController;
class AfhController;

namespace DeviceType {
Q_NAMESPACE
enum Type : quint8 {
    Unknown = 0x00,     // 未知设备类型
    xdc236 = 0x01,      // 会议主机，主机控制器 xdcController
    DS240 = 0x02,       // 跳频主机，主机控制器 afhController
    PowerAmplifier = 0x03,  // 功放，占位，未设计
    WirelessMic = 0x04,     // 无线麦克风主机，占位，未设计
};
Q_ENUM_NS(Type)

inline Type fromByte(quint8 byte) {
    switch (byte) {
    case 0x00: return Unknown;
    case 0x01: return xdc236;
    case 0x02: return DS240;
    case 0x03: return PowerAmplifier;
    case 0x04: return WirelessMic;
    default:   return Unknown;
    }
}

inline QString typeName(Type type) {
    switch (type) {
    case Type::Unknown: return QObject::tr("未知设备");
    case Type::xdc236: return QObject::tr("XDC236 MEETING HOST");
    case Type::DS240: return QObject::tr("DS240 AFH HOST");
    case Type::PowerAmplifier: return QObject::tr("Power Amplifier");
    case Type::WirelessMic: return QObject::tr("无线麦克风");
    default: return QObject::tr("其他");
    }
}
}

namespace HostStatusDefs {
Q_NAMESPACE
enum class HostStatus {
    None = 0,        // 无状态/未知 - 从未探测过或地址无效
    Connected = 1,   // 已探测 - 硬件存在，但未创建控制器实例
    Ready = 2,       // 已就绪 - 已创建实例，可接收指令
    Busy = 3,        // 忙碌中 - 正在执行耗时操作
    Error = 4        // 错误 - 通信异常或故障
};
Q_ENUM_NS(HostStatus)

inline QString toString(int status) {
    switch (status) {
    case 0: return "None";
    case 1: return "Connected";
    case 2: return "Ready";
    case 3: return "Busy";
    case 4: return "Error";
    default: return "Unknown";
    }
}
}

struct HostInfo {
    Q_GADGET
public:
    HostInfo(): address(0), status(HostStatusDefs::HostStatus::None) {}

    // 结构体属性，供 QML 绑定使用
    Q_PROPERTY(uint8_t address MEMBER address CONSTANT)         // 主机地址，通信标识
    Q_PROPERTY(QString addressHex READ addressHex CONSTANT)     // 主机地址，用于显示（如 0x01）
    Q_PROPERTY(QString serialNumber MEMBER serialNumber)        // 序列号，主机唯一标识
    Q_PROPERTY(QString aliasName MEMBER aliasName)              // 用户自定义名称
    Q_PROPERTY(DeviceType::Type deviceType MEMBER deviceType)   // 设备类型枚举
    Q_PROPERTY(QString deviceTypeName READ deviceTypeName)      // 设备类型中文名称
    Q_PROPERTY(HostStatusDefs::HostStatus status MEMBER status) // 主机当前状态
    Q_PROPERTY(QString statusText READ statusText)              // 主机状态中文描述
    Q_PROPERTY(bool isConnected READ isConnected)               // 是否已探测到硬件
    Q_PROPERTY(bool isReady READ isReady)                       // 是否已创建控制器实例
    Q_PROPERTY(QString lastSeenTime READ lastSeenTime)          // 最后一次通信时间

    QString addressHex() const { return QString("0x%1").arg(address, 2, 16, QChar('0')).toUpper(); }
    bool isConnected() const { return status == HostStatusDefs::HostStatus::Connected; }
    bool isReady() const { return status == HostStatusDefs::HostStatus::Ready; }
    QString lastSeenTime() const { return lastSeen.toString("hh:mm:ss"); }
    QString deviceTypeName() const { return DeviceType::typeName(deviceType); }

    QString statusText() const {
        switch (status) {
        case HostStatusDefs::HostStatus::None: return QObject::tr("未知");
        case HostStatusDefs::HostStatus::Connected: return QObject::tr("已连接");
        case HostStatusDefs::HostStatus::Ready: return QObject::tr("已就绪");
        case HostStatusDefs::HostStatus::Busy: return QObject::tr("忙碌");
        case HostStatusDefs::HostStatus::Error: return QObject::tr("错误");
        default: return QObject::tr("其它");
        }
    }

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["address"] = static_cast<int>(address);
        map["addressHex"] = addressHex();
        map["serialNumber"] = serialNumber;
        map["aliasName"] = aliasName;
        map["deviceType"] = static_cast<int>(deviceType);
        map["deviceTypeName"] = deviceTypeName();
        map["status"] = static_cast<int>(status);
        map["statusText"] = statusText();
        map["isConnected"] = isConnected();
        map["isReady"] = isReady();
        map["lastSeen"] = lastSeenTime();
        map["customData"] = customData;
        return map;
    }

    uint8_t address;                // 通信指令地址
    QString serialNumber;           // 序列号
    QString aliasName;              // 别名
    DeviceType::Type deviceType;    // 设备类型
    HostStatusDefs::HostStatus status;  // 状态
    QDateTime lastSeen;             // 最后活跃时间
    QVariantMap customData;         // 扩展数据
};

/**
 * @brief 主机管理器类 - 负责管理多个主机控制器实例
 */
class HostManager : public QObject
{
    Q_OBJECT

public:
    using HostStatus = HostStatusDefs::HostStatus;
    using DeviceTypeEnum = DeviceType::Type;

    explicit HostManager(ConnectionFactory* factory = nullptr, QObject *parent = nullptr);
    virtual ~HostManager();

    // ==================== 核心数据发送接口 ====================
    Q_INVOKABLE bool sendRawData(const QByteArray &data); // 发送原始字节流到适配器
    Q_INVOKABLE bool sendProtocolFrame(uint8_t destAddr, uint8_t srcAddr, uint8_t opCode, uint16_t command, const QByteArray &payload = QByteArray()); // 构建并发送协议帧
    Q_INVOKABLE bool isConnected() const; // 检查底层适配器连接状态

    // ==================== 主机生命周期管理接口 ====================
    // 0. 探测主机地址 - 探测地址范围，主机是否连线
    Q_INVOKABLE void detectHosts(uint8_t maxAddress);

    // 1.1 创建主机实例（如若地址已被探测到，为指定地址创建主机实例）
    Q_INVOKABLE bool createHost(uint8_t hostAddress);

    // 1.2 创建单主机控制器实例 - 根据设备类型实例化 Xdc/Afh Controller
    QObject* createControllerByType(uint8_t hostAddress, DeviceTypeEnum type);

    // 2. 移除主机实例（销毁指定地址的控制器实例，状态回退到 Connected）
    Q_INVOKABLE bool removeHost(uint8_t hostAddress);

    // 3. 移除所有主机实例（释放所有控制器实例）
    Q_INVOKABLE void removeAllHosts();

    // 4. 选择主机（如若地址主机已经实例化，选择该实例；如果没有实例化则创建实例，再选择）
    Q_INVOKABLE bool selectHost(uint8_t hostAddress);

    // ==================== 多主机管理（主机集合信息）====================
    // 5.1 获取所有连接的 Host Address - 返回地址整数列表
    Q_INVOKABLE QVariantList getConnectedHostList() const;

    // 5.2 获取已就绪的主机列表 - 返回包含完整信息的 Map 列表
    Q_INVOKABLE QVariantList getReadyHostList() const;

    // 5.3 通过地址获取指定的 HostInfo 数据结构
    Q_INVOKABLE QVariantMap getAllHostMap(uint8_t hostAddress) const;

    // ==================== 单主机管理（通过地址或者特定变量管理单主机）====================
    // 6.1.1 获取 HostInfo 指针 - 用于内部快速访问
    HostInfo* getHostInfoPtr(uint8_t address);
    const HostInfo* getHostInfoPtr(uint8_t address) const;

    // 6.2.1 更新单主机信息（UPSERT 模式：已存在则更新，不存在则创建）
    void upsertHostInfo(uint8_t address, HostStatus status, DeviceTypeEnum deviceType);

    // 6.2.2 更新单主机最后通信时间
    void updateHostLastSeen(uint8_t address);

    // 6.2.3 更新选中主机映射表 (供 QML 绑定) - 同步 m_selectedHostMap 的数据
    void updateSelectedHostMap();

    // 6.3 移除单主机控制器实例（不删除主机信息档案）
    void removeController(uint8_t address);

    // 6.4 获取单主机控制器实例 - 返回 QObject 指针供 QML 调用业务方法
    Q_INVOKABLE QObject* getController(uint8_t address) const;

    // ==================== 其它模块特别需求 ====================
    // 获取可用主机列表（供会议模块使用）- 目前等同于 Ready 列表
    Q_INVOKABLE QVariantList getAvailableHostsForMeeting() const;

    // ==================== QML 属性支持 ====================
    Q_PROPERTY(QVariantList connectedHostList READ getConnectedHostList NOTIFY hostInfoChanged)             // 连线主机地址列表
    Q_PROPERTY(QString selectedHostAddress READ getSelectedHostAddress NOTIFY selectedHostChanged)          // 文本格式-选中主机地址
    Q_PROPERTY(uint8_t selectedHostAddressInt READ getSelectedHostAddressInt NOTIFY selectedHostChanged)    // 数字格式-选中主机地址
    Q_PROPERTY(QQmlPropertyMap* selectedHostMap READ getSelectedHostMap NOTIFY selectedHostChanged)         // 选中主机地址信息集合

    // 辅助函数 & Getter
    QString formatAddressHex(uint8_t address) const { return QString::asprintf("0x%02X", address); }

    QString getSelectedHostAddress() const { return m_selectedHostAddress == 0 ? "N/A" : formatAddressHex(m_selectedHostAddress); }
    uint8_t getSelectedHostAddressInt() const { return m_selectedHostAddress; }
    QQmlPropertyMap* getSelectedHostMap() const { return m_selectedHostMap; }

signals:
    /* 数据分发类信号
     * 作用：广播解析后的协议帧，供业务控制器（Xdc/Afh）进行二次处理。
     * 触发条件：数据解析成功、非探测响应(0x0201)、且非会议命令(0x07xx)时，在完成 Controller 转发后触发。
     * 接收者：全局日志记录器、UI 数据监控面板。*/
    void hostFrameParsed(uint8_t address, const ProtocolProcessor::ParsedFrameData &data);

    /* 数据分发类信号 (分支3：会议路由)
     * 作用：将会议相关的硬件响应直接投递给 MeetingManager，实现业务解耦。
     * 触发条件：在 onFactoryDataReceived 中解析到的命令码在 0x0700 - 0x07FF 范围内时立即触发。
     * 接收者：MeetingManager。*/
    void meetingCommandResponse(uint8_t hostAddress, uint16_t command, const QByteArray& payload);

    void errorOccurred(const QString &error); // 通用错误信号，接收者：UI 提示框

    /* 主机探测与发现信号
     * 作用：通知系统“已成功探测并归档一个硬件主机”。
     * 触发条件：收到 0x0201 响应并完成 upsertHostInfo 档案更新后触发。
     * 接收者：host_detecting.qml（驱动 UI 实时刷新“已发现主机列表”）。*/
    void detectedHostsResponseReceived(uint8_t address);

    /* 状态与生命周期信号 (通用)
     * 作用：主机档案发生任何变动时的统一通知入口。
     * 触发条件：主机状态流转（如 Connected->Ready）、最后通信时间(lastSeen)更新、或别名修改时触发。
     * 接收者：QML 界面（利用此信号实现主机卡片状态的自动重绘）。*/
    void hostInfoChanged(uint8_t hostAddress);

    /* 选中主机变更信号
     * 作用：标记当前“界面选中项”的主机。
     * 触发条件：调用 selectHost 成功切换时触发。
     * 用途：1. UI 高亮显示当前选中的主机。2. 更新顶部状态栏。
     * 接收者：host_connection.qml, Main.qml。*/
    void selectedHostChanged(uint8_t oldAddress, uint8_t newAddress);

public slots:
    void onFactoryDataReceived(const QByteArray &data); // 处理工厂数据接收
    void onFactoryErrorOccurred(const QString &error);  // 处理工厂错误

private:
    // ==================== 1. 硬件通信相关 ====================
    ConnectionFactory* m_factory;           // 硬件连接工厂实例
    QByteArray m_receivedData;              // 原始数据接收缓冲区
    void connectToFactorySignals();         // [私有] 连接工厂硬件信号
    void disconnectFromFactorySignals();    // [私有] 断开工厂连接信号

    // ==================== 2. 主机探测相关 ====================
    QMap<uint8_t, bool> m_detectHostResponse; // 探测响应记录表
    int m_timeoutMs;                        // 默认探测超时时间（毫秒）
    int m_maxRetries;                       // 默认最大重试次数
    QMutex m_detectHostMutex;               // 探测响应线程锁
    QDateTime m_lastDetectionTime;          // 最后一次探测时间
    bool waitForDetectHostResponse(uint8_t address, int timeoutMs, int maxRetries); // [私有] 等待响应

    // ==================== 3. 数据路由相关 ====================
    void forwardFrameToHostController(const ProtocolProcessor::ParsedFrameData &data); // [私有] 将帧数据转发到对应主机控制器
    void handleUnknownAddress(uint8_t address, const ProtocolProcessor::ParsedFrameData &data); // [私有] 处理未知地址数据

    // 路由判断辅助函数：是否为探测主机响应指令
    bool isHostDetectingCommand(uint16_t command) const {
        // 探测主机相关的指令集，作为HostManager分发数据的条件。 目前只有 0x0201，预留扩展空间（如：return command == 0x0201 || command == 0x0301;）
        return (command == 0x0201);
    }

    // 路由判断辅助函数：是否为会议相关响应指令
    bool isMeetingCommand(uint16_t command) const {
        // 会议活动相关指令集， 作为HostManager分发数据的条件
        return (command >= 0x0700 && command <= 0x07FF);
    }


    // ==================== 4. 主机数据管理（核心）====================
    QMap<uint8_t, HostInfo> m_hostInfoMap;  // 所有主机信息的唯一数据源

    // ==================== 5. 主机控制器管理 ====================
    QMap<uint8_t, QObject*> m_controllers;    // 独立的控制器容器，HostInfo 不含控制器信息

    // ==================== 6. 选中主机管理 ====================
    uint8_t m_selectedHostAddress;    // 当前选中的主机地址（光标变量）
    QQmlPropertyMap* m_selectedHostMap; // 供 QML 绑定的选中主机详细属性集
};

#endif // HOSTMANAGER_H