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
        DS240 = 0x02,      // 跳频主机，主机控制器 afhController
        PowerAmplifier = 0x03,  // 功放，占位，未设计
        WirelessMic = 0x04,     // 无线麦克风主机，占位，未设计
    };
    Q_ENUM_NS(Type)

    // 由值获取类型（可用于探测主机）
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

    // 由类型显示设备名称（可用于QML显示）
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

// 定义主机状态枚举值类型
namespace HostStatusDefs {
    Q_NAMESPACE
        enum class HostStatus {
            None = 0,        // 无状态/未知 - 从未探测过或地址无效
            Connected = 1,   // 已探测 - 通过探测指令发现硬件存在，但未创建实例（状态中，探测到的即表示已经连接）
            Ready = 2,       // 已就绪 - 已创建实例，可操作但非当前活动
            Active = 3,      // 活动中 - 当前选中的活动主机
            Busy = 4,        // 忙碌中 - 正在执行操作
            Error = 5        // 错误 - 通信异常或故障
        };
    Q_ENUM_NS(HostStatus)

    inline QString toString(int status) {
        switch (status) {
        case 0: return "None";
        case 1: return "Connected";
        case 2: return "Ready";
        case 3: return "Active";
        case 4: return "Busy";
        case 5: return "Error";
        default: return "None";
        }
    }
}

// 定义主机信息结构体,管理全局状态
struct HostInfo {
    Q_GADGET

public:
    // 结构体构造函数
    HostInfo(): address(0), status(HostStatusDefs::HostStatus::None) {}

    // 结构体属性， 供QML绑定使用
    Q_PROPERTY(uint8_t address MEMBER address CONSTANT)         // 主机地址，通信标识
    Q_PROPERTY(QString addressHex READ addressHex CONSTANT)     // 主机地址，用于显示
    Q_PROPERTY(QString serialNumber MEMBER serialNumber)        // 序列号
    Q_PROPERTY(QString aliasName MEMBER aliasName)              // 名称
    Q_PROPERTY(DeviceType::Type deviceType MEMBER deviceType)   // 设备类型
    Q_PROPERTY(QString deviceTypeName READ deviceTypeName)      // 设备类型名称
    Q_PROPERTY(HostStatusDefs::HostStatus status MEMBER status) // 主机状态，枚举值
    Q_PROPERTY(QString statusText READ statusText)              // 主机状态，用于显示
    Q_PROPERTY(bool isConnected READ isConnected)               // 主机状态，是否已经连接（探测到地址）
    Q_PROPERTY(bool isReady READ isReady)                       // 主机状态，是否已经创建主机实例
    Q_PROPERTY(bool isActive READ isActive)                     // 主机状态，是否当前活动主机
    Q_PROPERTY(QString lastSeenTime READ lastSeenTime)

    // 成员变量函数，用于QML属性
    QString addressHex() const { return QString("0x%1").arg(address, 2, 16, QChar('0')).toUpper(); }
    bool isConnected() const { return status == HostStatusDefs::HostStatus::Connected; }
    bool isReady() const { return status == HostStatusDefs::HostStatus::Ready; }
    bool isActive() const { return status == HostStatusDefs::HostStatus::Active; }
    QString lastSeenTime() const { return lastSeen.toString("hh:mm:ss"); }
    QString deviceTypeName() const { return DeviceType::typeName(deviceType); }

    // 状态转为中文文本
    QString statusText() const {
        switch (status) {
        case HostStatusDefs::HostStatus::None: return QObject::tr("未知");
        case HostStatusDefs::HostStatus::Connected: return QObject::tr("已连接");
        case HostStatusDefs::HostStatus::Ready: return QObject::tr("已就绪");
        case HostStatusDefs::HostStatus::Active: return QObject::tr("活动中");
        case HostStatusDefs::HostStatus::Busy: return QObject::tr("忙碌");
        case HostStatusDefs::HostStatus::Error: return QObject::tr("错误");
        default: return QObject::tr("其它");
        }
    }

    // 结构体信息转换为VariantMap
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
        map["isActive"] = isActive();
        map["lastSeen"] = lastSeenTime();
        map["customData"] = customData;
        return map;
    }

    // 结构体成员变量
    uint8_t address;                // 通信指令地址
    QString serialNumber;           // 序列号，主机唯一标识
    QString aliasName;              // 用户自定义名称
    DeviceType::Type deviceType;    // 设备类型
    HostStatusDefs::HostStatus status;  // 主机状态
    QDateTime lastSeen;
    QVariantMap customData;     // 其它扩展信息
};


/**
 * @brief 主机管理器类 - 负责管理多个xxxController实例
 * 主要功能：
 * - 管理接口连接
 * - 管理多主机的生命周期（探测、创建、切换、停用、删除）
 * - 管理多主机全局状态，管理活动主机
 * - 管理数据路由，由接口至主机
 */
class HostManager : public QObject
{
    Q_OBJECT

public:
    // 为了方便在类内部使用外部Status、DeviceType枚举类型，添加 using
    using HostStatus = HostStatusDefs::HostStatus;
    using DeviceTypeEnum = DeviceType::Type;

    // 构造函数和析构函数
    explicit HostManager(ConnectionFactory* factory = nullptr, QObject *parent = nullptr); // 构造函数，可传入工厂实例
    virtual ~HostManager(); // 析构函数，清理资源


    // 属性
    Q_PROPERTY(QVariantList connectedHostList READ getConnectedHostList NOTIFY hostInfoChanged) // 探测到的所有的主机地址（多个主机），供界面调用
    Q_PROPERTY(QString activeHostAddress READ getActiveHostAddress NOTIFY activeHostChanged)    // 当前活动主机地址（唯一主机），供界面调用
    Q_PROPERTY(QQmlPropertyMap* activeHostMap READ getActiveHostMap NOTIFY activeHostChanged)   // 当前活动主机信息（唯一主机），供界面调用

    // ==== 核心数据发送接口 ====
    Q_INVOKABLE bool sendRawData(const QByteArray &data); // 发送原始数据到适配器
    Q_INVOKABLE bool sendProtocolFrame(uint8_t destAddr, uint8_t srcAddr, uint8_t opCode, uint16_t command, const QByteArray &payload = QByteArray()); // 发送协议帧
    Q_INVOKABLE bool isConnected() const; // 检查适配器连接状态

    // ==== 主机生命周期管理接口 ====
    // 0. 探测主机地址 - 探测地址范围，主机是否连线
    Q_INVOKABLE void detectHosts(uint8_t maxAddress);

    // 1. 创建主机（如若地址已被探测到，为指定地址创建主机实例）
    Q_INVOKABLE bool createHost(uint8_t hostAddress);

    // 2. 设置活动主机（如若地址主机实例已经创建，设置为当前活动主机；如未创建，则创建实例）
    Q_INVOKABLE bool activateHost(uint8_t hostAddress);

    // 3. 切换主机（如若地址主机实例已经创建，切换地址主机实例为当前活动主机；如果主机实例未创建则先创建实例，再设置为活动主机）
    Q_INVOKABLE bool switchHost(uint8_t hostAddress);

    // 4. 停用主机（如若地址主机是当前活动主机，则设置为非活动主机；如若不是，则不改变）
    Q_INVOKABLE bool stopHost(uint8_t hostAddress);

    // 5. 移除主机（如若指定地址主机实例存在且为活动主机，则停用再删除实例；如若实例存在但非活动删除实例）
    Q_INVOKABLE bool removeHost(uint8_t hostAddress);

    // 6. 移除所有主机（停止活动主机并释放所有控制器实例）
    Q_INVOKABLE void removeAllHosts();

    // 7. 获取当前主机信息
    QVariantList getConnectedHostList() const;  // connectedHostList的getter函数
    QString getActiveHostAddress() const;   // activeHostAddress的getter函数

    Q_INVOKABLE QVariantMap getAllHostMap(uint8_t hostAddress) const;    // 通过地址，获取一个完整的HostInfo
    QQmlPropertyMap* getActiveHostMap() const { return m_activeHostMap;}  // activeHostMap的getter函数，返回m_activeHostMap 指针给QML引擎；QML通过该指针访问 QQmlPropertyMap 中的属性

    // 8. 获取主机控制器（供 QML 调用）
    Q_INVOKABLE QObject* getController(uint8_t address) const;  // 获取指定地址主机控制器

    // ======= 辅助函数 =======
    QString formatAddressHex(uint8_t addr) {
        return QString::asprintf("0x%02X", addr);
    }


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

    // ==== 收发数据信号 ====
    /* 基础通信类信号 (当前未启用)
     * 作用：透传最原始的字节流数据，不进行任何解析。
     * 状态：已声明但目前在 HostManager 内部未连接发射源，建议后续根据调试需求决定是否移除。*/
    void hostDataReceived(const QByteArray &data); // 声明了但未使用
    void errorOccurred(const QString &error); // 错误发生信号

    /* 主机探测与发现信号
     * 作用：通知系统“已成功探测并归档一个硬件主机”。
     * 触发条件：收到 0x0201 响应并完成 upsertHostInfo 档案更新后触发。
     * 用途：驱动 UI 实时刷新“已发现主机列表”。*/ 
    void detectedHostsResponseReceived(uint8_t address);  // 探测主机过程中收到响应指令帧信号

    // ==== 活动主机变更信号 及主机状态变更信号 ====
    /* 状态与生命周期信号 (通用)
     * 作用：主机档案发生任何变动时的统一通知入口。
     * 触发条件：主机状态流转（如 Connected->Ready）、最后通信时间(lastSeen)更新、或别名修改时触发。
     * 用途：QML 界面利用此信号实现主机卡片状态的自动重绘。*/
    void hostInfoChanged(uint8_t hostAddress);      // 使用结构体，HostInfo数据变动统一发射同一个信号
    
    /* 状态与生命周期信号
    * 作用：标记当前“主控权”的转移。在多主机系统中，虽然有很多主机在线，但同一时刻通常只有一个主机处于“活动”状态（Active），负责处理主要的交互。
    * 触发条件：调用 activateHost 或 switchHost 成功切换活动主机时触发。
    * 用途：1. UI 高亮显示当前选中的主机。2. 通知旧的 Controller 暂停高频轮询以节省带宽，通知新的 Controller 开始初始化。*/
    void activeHostChanged(uint8_t oldAddress, uint8_t newAddress); // 活动主机变更信号，对于活动主机，单独设计一个信号
    
    /* 废弃的信号，但未完成修改*/
    // void hostStatusChanged( uint8_t hostAddress, HostStatusDefs::HostStatus hostStatus); // 在TCPSocketAdapter中仍有残余，待更新

public slots:
    // ==== 工厂信号处理槽 ====
    void onFactoryDataReceived(const QByteArray &data); // 处理工厂数据接收
    void onFactoryErrorOccurred(const QString &error);  // 处理工厂错误

private:
    // ==================== 1. 硬件通信相关 ====================
    // 与底层硬件直接交互的成员
    ConnectionFactory* m_factory;           // 硬件连接工厂，连接工厂实例
    QByteArray m_receivedData;              // 原始数据接收缓冲区

    // 硬件信号连接管理
    void connectToFactorySignals();         // 连接工厂硬件信号
    void disconnectFromFactorySignals();    // 断开工厂连接信号

    // ==================== 2. 主机探测相关 ====================
    // 探测主机时使用的成员
    QMap<uint8_t, bool> m_detectHostResponse;// 探测响应记录表，记录每个地址的探测响应帧状态（是否收到响应帧）
    int m_timeoutMs;                        // 默认探测超时时间，单位毫秒，默认3000ms
    int m_maxRetries;                       // 默认最大重试次数，默认3次
    QMutex m_detectHostMutex;               // 探测响应线程锁
    QDateTime m_lastDetectionTime;          // 最后一次探测时间

    // 探测主机方法
    // 注：直接使用 sendProtocolFrame 发送探测指令，无需额外封装
    // bool sendProbeCommand(uint8_t address);  // 已删除，不需要这个封装
    bool waitForDetectHostResponse(uint8_t address, int timeoutMs, int maxRetries); // 等待响应

    // ==================== 3. 数据路由相关 ====================
    void forwardFrameToHostController(const ProtocolProcessor::ParsedFrameData &data); // 将帧数据转发到对应主机控制器
    void handleUnknownAddress(uint8_t address, const ProtocolProcessor::ParsedFrameData &data); // 处理未知地址数据

    // 路由判断辅助函数
    bool isMeetingCommand(uint16_t command) const {
        // 会议活动相关指令集， 作为HostManager分发数据的条件
        return (command >= 0x0700 && command <= 0x07FF);
    }

    bool isHostDetectingCommand(uint16_t command) const {
        // 探测主机相关的指令集，作为HostManager分发数据的条件。 目前只有 0x0201，预留扩展空间（如：return command == 0x0201 || command == 0x0301;）
        return (command == 0x0201);
    }

    // ==================== 4. 主机数据管理（核心）====================
    // 主机信息管理 ---------------------
    QMap<uint8_t, HostInfo> m_hostInfoMap;  // 所有主机信息的唯一数据源

    void upsertHostInfo(uint8_t address, HostStatus status, DeviceTypeEnum deviceType);  // 更新指定地址主机的状态
    void updateHostLastSeen(uint8_t address);               // 更新主机最后通信时间
    HostInfo* getHostInfoPtr(uint8_t address);              // 获取主机信息指针（可修改版本）
    const HostInfo* getHostInfoPtr(uint8_t address) const;  // 获取主机信息指针（只读版本）

    // 主机控制器管理 --------------------
    QMap<uint8_t, QObject*> m_controllers;    // 独立的控制器容器，HostInfo不含控制器信息
    QObject* createControllerByType(uint8_t hostAddress, DeviceTypeEnum type);
    void removeController(uint8_t address);

    // 活动主机管理 --------------------
    uint8_t m_activeHostAddress;    // 当前活动主机地址

    // ==================== 5. QML界面支持 ====================
    // 为QML提供数据的方法 --------------------
    QQmlPropertyMap* m_activeHostMap;   // 定义支持绑定的主机数据类型
    void updateActiveHostMap();         // 更新m_activeHostMap 对象内部的数据（不是修改指针本身，不发射信号，只负责更新数据）
};

#endif // HOSTMANAGER_H
