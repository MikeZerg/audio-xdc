// source/HostController.h
#ifndef HOSTCONTROLLER_H
#define HOSTCONTROLLER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantList>
#include <QThread>
#include "ProtocolProcessor.h"
#include "SerialPortHandler.h"

class SerialPortHandler;
class ProtocolProcessor;

class HostController : public QObject
{
    Q_OBJECT
    // 添加属性用于界面显示
    Q_PROPERTY(QString systemDatetime READ getter_systemDatetime NOTIFY signal_systemDatetime)                                 // 0x0204-(    ) 获取主机系统时间
    Q_PROPERTY(QString hostKernelVersion READ getter_hostKernelVersion NOTIFY signal_hostKernelVersion)                        // 0x0202-(0x00) 获取主机版本号(Kernel)
    Q_PROPERTY(QString hostLinkVersion READ getter_hostLinkVersion NOTIFY signal_hostLinkVersion)                              // 0x0202-(0x01) 获取主机版本号(Link)
    Q_PROPERTY(QString hostAntaBoxVersion READ getter_hostAntaBoxVersion NOTIFY signal_hostAntaBoxVersion)                     // 0x0202-(0x02) 获取主机版本号(Anta)
    Q_PROPERTY(QString hostKernelBuildDate READ getter_hostKernelBuildDate NOTIFY signal_hostKernelBuildDate)                  // 0x0203-(0x00) 获取主机编译日期(Kernel)
    Q_PROPERTY(QString hostLinkBuildDate READ getter_hostLinkBuildDate NOTIFY signal_hostLinkBuildDate)                        // 0x0203-(0x01) 获取主机编译日期(Link)
    Q_PROPERTY(QString hostAntaBoxBuildDate READ getter_hostAntaBoxBuildDate NOTIFY signal_hostAntaBoxBuildDate)               // 0x0203-(0x02) 获取主机编译日期(Anta)

    Q_PROPERTY(QString speechMode READ getter_speechMode NOTIFY signal_speechMode)                                             // 0x0208-(    ) 获取发言模式
    Q_PROPERTY(QString maxSpeechUnits READ getter_maxSpeechUnits NOTIFY signal_maxSpeechUnits)                                 // 0x0209-(    ) 获取最大发言数量

    Q_PROPERTY(QString wirelessVolume READ getter_wirelessVolume NOTIFY signal_wirelessVolume)                                 // 0x0205-(    ) 获取无线音量
    Q_PROPERTY(QString wiredVolume READ getter_wiredVolume NOTIFY signal_wiredVolume)                                          // 0x0206-(    ) 获取有线音量
    Q_PROPERTY(QString antaBoxVolume READ getter_antaBoxVolume NOTIFY signal_antaBoxVolume)                                    // 0x0207-(    ) 获取天线盒音量

    Q_PROPERTY(QString wirelessAudioPath READ getter_wirelessAudioPath NOTIFY signal_wirelessAudioPath)                        // 0x020D-(    ) 获取无线音频路径

    Q_PROPERTY(QString monitorAudioChannels READ getter_monitorAudioChannels NOTIFY signal_monitorAudioChannels)               // 0x0211-(    ) 获取监测音频信道数量
    Q_PROPERTY(QString unitCapacity READ getter_unitCapacity NOTIFY signal_unitCapacity)                                       // 0x0210-(    ) 获取单元容量

    Q_PROPERTY(QString cameraTrackingProtocol READ getter_cameraTrackingProtocol NOTIFY signal_cameraTrackingProtocol)         // 0x020A-(    ) 获取摄像跟踪协议
    Q_PROPERTY(QString cameraTrackingAddress READ getter_cameraTrackingAddress NOTIFY signal_cameraTrackingAddress)            // 0x020B-(    ) 获取摄像跟踪地址
    Q_PROPERTY(QString cameraTrackingBaudrate READ getter_cameraTrackingBaudrate NOTIFY signal_cameraTrackingBaudrate)         // 0x020C-(    ) 获取摄像跟踪波特率

    Q_PROPERTY(QVariantList audioChannelInfo READ getter_audioChannelInfo NOTIFY signal_audioChannelInfo)                      // 0x0901/0x0404-(    ) 获取无线音频信道RSSI/FREQ

    // 非指令属性
    Q_PROPERTY(int sendQueueLength READ getter_sendQueueLength NOTIFY signal_sendQueueChange)                                  // 发送队列长度


    /*************暂时未使用*************/
    Q_PROPERTY(QString currentWirelessSpeakerId READ getter_currentWirelessSpeakerId NOTIFY signal_currentWirelessSpeakerId)   // 0x020E-(    ) 获取当前无线发言Id
    Q_PROPERTY(QString currentWiredSpeakerId READ getter_currentWiredSpeakerId NOTIFY signal_currentWiredSpeakerId)            // 0x020F-(    ) 获取当前有线发言Id

    Q_PROPERTY(QString wirelessPacketStatistics READ getter_wirelessPacketStatistics NOTIFY signal_wirelessPacketStatistics)   // 0x0401-(    ) 获取无线报文统计
    Q_PROPERTY(QString wiredPacketStatistics READ getter_wiredPacketStatistics NOTIFY signal_wiredPacketStatistics)            // 0x0402-(    ) 获取有线报文统计
    Q_PROPERTY(QString antaBoxPacketStatistics READ getter_antaBoxPacketStatistics NOTIFY signal_antaBoxPacketStatistics)      // 0x0403-(    ) 获取天线盒报文统计

    Q_PROPERTY(QString wirelessMgmtChannel READ getter_wirelessMgmtChannel NOTIFY signal_wirelessMgmtChannel)                  // 0x0405-(    ) 获取无线管理信道RSSI

    Q_PROPERTY(QVariantList unitInfo READ getter_unitInfo NOTIFY signal_unitInfo)                                              // 0x0601-(    ) 获取单元信息
    Q_PROPERTY(QVariantMap unitAlias READ getter_unitAlias NOTIFY signal_unitAlias)                                            // 0x0602-(    ) 获取单元别名
    Q_PROPERTY(QString unitWirelessStatus READ getter_unitWirelessStatus NOTIFY signal_unitWirelessStatus)                     // 0x0606-(    ) 获取无线单元状态信息

    Q_PROPERTY(QString meetingName READ getter_meetingName NOTIFY signal_meetingName)                                          // 0x0801-(    ) 获取会议名称


public:
    // 定义查询命令结构体， 用于队列发送查询指令
    struct HostQueryInfo {
        uint8_t opCode;      // 操作类型
        uint16_t command;    // 操作命令
        QByteArray payload;  // 参数
        
        HostQueryInfo() : opCode(0x01), command(0), payload(QByteArray()) {}
        HostQueryInfo(uint8_t op, uint16_t cmd, const QByteArray& data = QByteArray()) 
            : opCode(op), command(cmd), payload(data) {}
    };

    // 定义发送队列结构体
    struct SendQueueItem {
        quint8 address;             // 发送目标主机地址
        HostQueryInfo queryInfo;    // 查询信息
        int delayMS;                // 发指令前延迟时间(毫秒)

        SendQueueItem() : address(0x01), delayMS(0) {}
        SendQueueItem(quint8 addr, const HostQueryInfo& info, int delay = 0)
            : address(addr), queryInfo(info), delayMS(delay) {}
    };


    // 定义音频通道信号强度结构体，监测通道及输出通道
    struct audioChannelInfo  {
        // 基础信息
        quint8 index = 0;               // 索引号 (1-20)
        // 监测通道数据
        quint32 monitorFREQ = 0;        // 监测通道频率 (Hz)
        double monitorRSSI = 0.0;       // 监测通道信号强度 (dBm)

        // 输出通道数据 (4个输出通道，每个有频率和RSSI)
        quint32 outputFREQ1 = 0;        // 输出通道1频率
        quint32 outputFREQ2 = 0;        // 输出通道2频率
        quint32 outputFREQ3 = 0;        // 输出通道3频率
        quint32 outputFREQ4 = 0;        // 输出通道4频率

        double outputRSSI1 = 0.0;       // 输出通道1信号强度
        double outputRSSI2 = 0.0;       // 输出通道2信号强度
        double outputRSSI3 = 0.0;       // 输出通道3信号强度
        double outputRSSI4 = 0.0;       // 输出通道4信号强度

        // 获取输出通道实际使用的频率（简单工具方法，可选）
        // quint32 getEffectiveoutputFreq(int channel) const {
        //     switch(channel) {
        //     case 1: return outputFREQ1 ? outputFREQ1 : monitorFREQ;
        //     case 2: return outputFREQ2 ? outputFREQ2 : monitorFREQ;
        //     case 3: return outputFREQ3 ? outputFREQ3 : monitorFREQ;
        //     case 4: return outputFREQ4 ? outputFREQ4 : monitorFREQ;
        //     default: return 0;
        //     }
        // }
    };


    // 定义单元信息结构体
    struct unitInformation {
        quint32 uID = 1;            // 单元ID
        quint32 uAddr = 0;          // 单元物理地址
        quint8 uRole = 0;           // 单元角色身份
        quint8 uType = 0;           // 单元类型
        quint8 uStatus = 0;         // 单元状态
        quint8 uResponse = 0;       // 单元预留参数
    };


    // 定义常用的查询命令参数常量， 用于设置发送查询指令, 适用于单次发送指令，不适用于批量发送
    static const HostQueryInfo const_systemDatetime;            // 0x0204-(    ) 获取主机系统时间
    static const HostQueryInfo const_hostKernelVersion;         // 0x0202-(0x00) 获取主机版本号(Kernel)
    static const HostQueryInfo const_hostLinkVersion;           // 0x0202-(0x01) 获取主机版本号(Link)
    static const HostQueryInfo const_hostAntaBoxVersion;        // 0x0202-(0x02) 获取主机版本号(Anta)
    static const HostQueryInfo const_hostKernelBuildDate;       // 0x0203-(0x00) 获取主机编译日期(Kernel)
    static const HostQueryInfo const_hostLinkBuildDate;         // 0x0203-(0x01) 获取主机编译日期(Link)
    static const HostQueryInfo const_hostAntaBoxBuildDate;      // 0x0203-(0x02) 获取主机编译日期(Anta)

    static const HostQueryInfo const_speechMode;                // 0x0208-(    ) 获取发言模式
    static const HostQueryInfo const_maxSpeechUnits;            // 0x0209-(    ) 获取最大发言数量

    static const HostQueryInfo const_wirelessVolume;            // 0x0205-(    ) 获取无线音量
    static const HostQueryInfo const_wiredVolume;               // 0x0206-(    ) 获取有线音量
    static const HostQueryInfo const_antaBoxVolume;             // 0x0207-(    ) 获取天线盒音量

    static const HostQueryInfo const_wirelessAudioPath;         // 0x020D-(    ) 获取无线音频路径

    static const HostQueryInfo const_monitorAudioChannels;      // 0x0211-(    ) 获取监测音频信道数量
    static const HostQueryInfo const_unitCapacity;              // 0x0210-(    ) 获取单元容量

    static const HostQueryInfo const_cameraTrackingProtocol;	// 0x020A-(    ) 获取摄像跟踪协议
    static const HostQueryInfo const_cameraTrackingAddress;     // 0x020B-(    ) 获取摄像跟踪地址
    static const HostQueryInfo const_cameraTrackingBaudrate;	// 0x020C-(    ) 获取摄像跟踪波特率

    // static const HostQueryInfo const_monitorAudioChannelInfo;   // 0x0212-(0-19) 获取监测音频信道信息, 被requestMonitorChannelInfo()替代
    // static const HostQueryInfo const_setSpeechChannelFreq;      // 0x0901-(    ) 设置无线发言通道和频率，被requestOutputChannelInfo()替代
    // static const HostQueryInfo const_outputAudioChannelInfo;    // 0x0404-(    ) 获取无线音频信道RSSI，被requestOutputChannelInfo()替代


    /*************暂时未使用*************/
    static const HostQueryInfo const_currentWirelessSpeakerId;	// 0x020E-(    ) 获取当前无线发言Id
    static const HostQueryInfo const_currentWiredSpeakerId;     // 0x020F-(    ) 获取当前有线发言Id

    static const HostQueryInfo const_wirelessPacketStatistics;	// 0x0401-(    ) 获取无线报文统计
    static const HostQueryInfo const_wiredPacketStatistics;     // 0x0402-(    ) 获取有线报文统计
    static const HostQueryInfo const_antaBoxPacketStatistics;	// 0x0403-(    ) 获取天线盒报文统计

    static const HostQueryInfo const_unitInfo;                  // 0x0601-(    ) 获取“注册”单元信息
    static const HostQueryInfo const_unitAlias;                 // 0x0602-(    ) 获取单元别名
    static const HostQueryInfo const_unitWirelessStatus;        // 0x0606-(    ) 获取无线单元状态信息

    static const HostQueryInfo const_meetingName;               // 0x0801-(    ) 获取会议名称

    static const HostQueryInfo const_wirelessMgmtChannel;       // 0x0405-(    ) 获取无线管理信道RSSI


    // 构建函数
    explicit HostController(quint8 address, SerialPortHandler *serialHandler,
                            ProtocolProcessor *protocolProcessor, 
                            QObject *parent = nullptr);

    // 析构函数
    virtual ~HostController();

    // ***************************缓存数据管理函数****************************************
    Q_INVOKABLE void loadFromCache();   // 从缓存加载数据
    Q_INVOKABLE void saveToCache();     // 保存数据到缓存
    bool isDataCached() const;          // 判断数据是否缓存


    // ***************************查询函数，发送指令给主机，用于界面调用***************************
    // 队列处理相关方法
    void startSendingQueue();                                                           // 开始发送队列
    void sendNextQueuedCommand();                                                       // 发送队列中的下一条指令
    void handleResponseReceived(quint8 address, uint8_t opCode, uint16_t command);      // 处理收到的响应
    void handleTimeout();                                                               // 处理超时


    // 主机相关操作-队列发送指令
    Q_INVOKABLE void queryByCommandQueue();             // 启动队列，开始发送指令
    Q_INVOKABLE void stopSendingQueue();                // 清空队列，停止发送指令

    // 主机相关操作-查询
    Q_INVOKABLE void queryHostInfo(const HostQueryInfo& queryInfo);             // 通用查询函数, 构建查询命令常量
    Q_INVOKABLE void query_hostInformation(uint8_t opCode,                      // 通过参数构建帧查询（单次）
                                           uint16_t command, const QByteArray &payload);

    Q_INVOKABLE void initializeHostProperties();    // 初始化主机属性, 发送18个查询指令，更新基本属性

    Q_INVOKABLE void requestMonitorChannelInfo();   // 0x0212，请求监测通道信息
    Q_INVOKABLE void requestOutputChannelInfo();    // 0x0901 & 0x0404，请求输出通道信息

    Q_INVOKABLE void detectingRegisteredUnits(quint32 max_UID);   // 0x0601-(    ) 获取“注册”单元信息
    // Q_INVOKABLE void requestUnitAlias();                    // 0x0602-(    ) 获取单元别名

    // 测试使用
    Q_INVOKABLE void printAudioChannelData() const; // 打印验证monitor和output输出数据
    Q_INVOKABLE void testAudioChannelDataDisplay(); // 测试monitor和output输出样式


    // ***************************接收函数，以下都与接收指令，显示指令结果相关******************************************
    // ***************************添加属性的getter函数，直接返回成员变量值，与Q_PROPERTY的READ关联(内联函数)**************
    Q_INVOKABLE quint8 address() const { return m_address; }                                  // 返回主机地址的内联函数

    QString getter_systemDatetime() const { return m_systemDatetime; }                        // 0x0204-(    ) 获取主机系统时间
    QString getter_hostKernelVersion() const { return m_hostKernelVersion; }                  // 0x0202-(0x00) 获取主机版本号(Kernel)
    QString getter_hostLinkVersion() const { return m_hostLinkVersion; }                      // 0x0202-(0x01) 获取主机版本号(Link)
    QString getter_hostAntaBoxVersion() const { return m_hostAntaBoxVersion; }                // 0x0202-(0x02) 获取主机版本号(Anta)
    QString getter_hostKernelBuildDate() const { return m_hostKernelBuildDate; }              // 0x0203-(0x00) 获取主机编译日期(Kernel)
    QString getter_hostLinkBuildDate() const { return m_hostLinkBuildDate; }                  // 0x0203-(0x01) 获取主机编译日期(Link)
    QString getter_hostAntaBoxBuildDate() const { return m_hostAntaBoxBuildDate; }            // 0x0203-(0x02) 获取主机编译日期(Anta)

    QString getter_speechMode() const { return m_speechMode; }                                // 0x0208-(    ) 获取发言模式
    QString getter_maxSpeechUnits() const { return m_maxSpeechUnits; }                        // 0x0209-(    ) 获取最大发言数量

    QString getter_wirelessVolume() const { return m_wirelessVolume; }                        // 0x0205-(    ) 获取无线音量
    QString getter_wiredVolume() const { return m_wiredVolume; }                              // 0x0206-(    ) 获取有线音量
    QString getter_antaBoxVolume() const { return m_antaBoxVolume; }                          // 0x0207-(    ) 获取天线盒音量

    QString getter_wirelessAudioPath() const { return m_wirelessAudioPath; }                  // 0x020D-(    ) 获取无线音频路径

    QString getter_monitorAudioChannels() const { return m_monitorAudioChannels; }            // 0x0211-(    ) 获取监测音频信道数量
    QString getter_unitCapacity() const { return m_unitCapacity; }                            // 0x0210-(    ) 获取单元容量


    QString getter_cameraTrackingProtocol() const { return m_cameraTrackingProtocol; }        // 0x020A-(    ) 获取摄像跟踪协议
    QString getter_cameraTrackingAddress() const { return m_cameraTrackingAddress; }          // 0x020B-(    ) 获取摄像跟踪地址
    QString getter_cameraTrackingBaudrate() const { return m_cameraTrackingBaudrate; }        // 0x020C-(    ) 获取摄像跟踪波特率

    QVariantList getter_audioChannelInfo() const{                                             // 0x0901/0x0404-(    ) 获取无线音频信道RSSI/FREQ(只需要一个槽函数)
        QVariantList result = m_audioChannelDataToQVariantList();
        return result;
    }

    int getter_sendQueueLength() const { return m_sendQueue.size(); };                       // 获取指令发送队列长度(***暂时未使用***)

    QString getter_currentWirelessSpeakerId() const { return m_currentWirelessSpeakerId; }    // 0x020E-(    ) 获取当前无线发言Id
    QString getter_currentWiredSpeakerId() const { return m_currentWiredSpeakerId; }          // 0x020F-(    ) 获取当前有线发言Id

    QString getter_wirelessPacketStatistics() const { return m_wirelessPacketStatistics; }    // 0x0401-(    ) 获取无线报文统计
    QString getter_wiredPacketStatistics() const { return m_wiredPacketStatistics; }          // 0x0402-(    ) 获取有线报文统计
    QString getter_antaBoxPacketStatistics() const { return m_antaBoxPacketStatistics; }      // 0x0403-(    ) 获取天线盒报文统计

    QString getter_wirelessMgmtChannel() const { return m_wirelessMgmtChannel; }              // 0x0405-(    ) 获取无线管理信道RSSI

    QVariantList getter_unitInfo() const {                                                    // 0x0601-(    ) 获取“注册”单元信息
        QVariantList result = m_unitInfoToVariantList();
        return result;
    }
    QVariantMap getter_unitAlias() const {                                                    // 0x0602-(    ) 获取单元别名
        QVariantMap result = m_unitAliasMapToVariantMap();
        return result;
    }
    QString getter_unitWirelessStatus() const { return m_unitWirelessStatus; }                // 0x0606-(    ) 获取无线单元状态信息

    QString getter_meetingName() const { return m_meetingName; }                              // 0x0801-(    ) 获取会议名称


    // ***************************添加属性的setter函数，在ProtocolProcessor::dispatchFrame()中使用更新数据******************
    void save_systemDatetime(const QString& datetime);                      // 0x0204-(    ) 获取主机系统时间
    void save_hostKernelVersion(const QString& version);                    // 0x0202-(0x00) 获取主机版本号(Kernel)
    void save_hostLinkVersion(const QString& version);                      // 0x0202-(0x01) 获取主机版本号(Link)
    void save_hostAntaBoxVersion(const QString& version);                   // 0x0202-(0x02) 获取主机版本号(Anta)
    void save_hostKernelBuildDate(const QString& date);                     // 0x0203-(0x00) 获取主机编译日期(Kernel)
    void save_hostLinkBuildDate(const QString& date);                       // 0x0203-(0x01) 获取主机编译日期(Link)
    void save_hostAntaBoxBuildDate(const QString& date);                    // 0x0203-(0x02) 获取主机编译日期(Anta)

    void save_speechMode(const QString& mode);                              // 0x0208-(    ) 获取发言模式
    void save_maxSpeechUnits(const QString& maxUnit);                       // 0x0209-(    ) 获取最大发言数量

    void save_wirelessVolume(const QString& volume);                        // 0x0205-(    ) 获取无线音量
    void save_wiredVolume(const QString& volume);                           // 0x0206-(    ) 获取有线音量
    void save_antaBoxVolume(const QString& volume);                         // 0x0207-(    ) 获取天线盒音量

    void save_wirelessAudioPath(const QString& audioPath);                  // 0x020D-(    ) 获取无线音频路径

    void save_monitorAudioChannels(const QString& audioChannel);            // 0x0211-(    ) 获取监测音频信道数量
    void save_unitCapacity(const QString& unitCapacity);                    // 0x0210-(    ) 获取单元容量

    void save_cameraTrackingProtocol(const QString& camProtocol);           // 0x020A-(    ) 获取摄像跟踪协议
    void save_cameraTrackingAddress(const QString& camAddress);             // 0x020B-(    ) 获取摄像跟踪地址
    void save_cameraTrackingBaudrate(const QString& camBaudrate);           // 0x020C-(    ) 获取摄像跟踪波特率

    void save_monitorAudioChannelInfo(const audioChannelInfo& info);        // 0x0212-(    ) 获取监测音频信道信息
    void save_outputAudioChannelInfoFREQ(const quint8 fid,
                                         quint8 cid, quint32 freq);         // 0x0901-(    ) 获取输出音频信道FREQ(需要两个save函数分别保存FREQ和RSSI)
    void save_outputAudioChannelInfoRSSI(const QByteArray rssiData);        // 0x0404-(    ) 获取无线音频信道RSSI(需要两个save函数分别保存FREQ和RSSI)



    void save_currentWirelessSpeakerId(const QString& speckerId);           // 0x020E-(    ) 获取当前无线发言Id
    void save_currentWiredSpeakerId(const QString& speckerId);              // 0x020F-(    ) 获取当前有线发言Id

    void save_wirelessPacketStatistics(const QString& packetStatistics);	// 0x0401-(    ) 获取无线报文统计
    void save_wiredPacketStatistics(const QString& packetStatistics);       // 0x0402-(    ) 获取有线报文统计
    void save_antaBoxPacketStatistics(const QString& packetStatistics);     // 0x0403-(    ) 获取天线盒报文统计

    void save_wirelessMgmtChannel(const QString& mgmtChannel);              // 0x0405-(    ) 获取无线管理信道RSSI

    void save_unitInfo(const quint32 uid, quint32 uAddr, quint8 uRole,
                       quint8 uType, quint8 uStatus, quint8 uResponse);     // 0x0601-(    ) 获取“注册”单元信息
    void save_unitAlias(quint16 uID, const QString& alias);                 // 0x0602-(    ) 获取单元别名
    void save_unitWirelessStatus(const QString& status);                    // 0x0606-(    ) 获取无线单元状态信息

    void save_meetingName(const QString& name);                             // 0x0801-(    ) 获取会议名称


    //*************************** Host Settings主机设置函数***************************
    Q_INVOKABLE void hostSet_systemDatetime(const QDateTime& para_systemDatetime);                // 0x0104-(38 6C D3 00) 设置主机系统时间
    Q_INVOKABLE void hostSet_speechMode(const QString& para_speechMode);                          // 0x0108-(    ) 设置发言模式
    Q_INVOKABLE void hostSet_maxSpeechUnits(quint8 para_maxSpeechUnits);                          // 0x0109-(    ) 设置最大发言数量
    Q_INVOKABLE void hostSet_wirelessVolume(quint8 para_wirelessVolume);                          // 0x0105-(    ) 设置无线音量
    Q_INVOKABLE void hostSet_wiredVolume(quint8 para_wiredVolume);                                // 0x0106-(    ) 设置有线音量
    Q_INVOKABLE void hostSet_antaBoxVolume(quint8 para_antaBoxVolume);                            // 0x0107-(    ) 设置天线盒音量
    Q_INVOKABLE void hostSet_wirelessAudioPath(const QString& para_wirelessAudioPath);            // 0x010D-(    ) 设置无线音频路径
    Q_INVOKABLE void hostSet_cameraTrackingAddress(quint16 para_cameraTrackingAddress);           // 0x010B-(    ) 设置摄像跟踪地址
    Q_INVOKABLE void hostSet_cameraTrackingProtocol(const QString& para_cameraTrackingProtocol);  // 0x010A-(    ) 设置摄像跟踪协议
    Q_INVOKABLE void hostSet_cameraTrackingBaudrate(quint32 para_cameraTrackingBaudrate);         // 0x010C-(    ) 设置摄像跟踪波特率

    Q_INVOKABLE void unitSet_Remove(quint32 uid);       // 0x0503-(    ) 删除单元配对
    Q_INVOKABLE void unitSet_On(quint32 uid);           // 0x0504-(    ) 打开单元发言
    Q_INVOKABLE void unitSet_Off(quint32 uid);          // 0x0505-(    ) 关闭单元静音
    Q_INVOKABLE void unitSet_Info(quint32 uid, quint32 uAddr, quint8 uRole, quint8 uType, quint8 uStatus, quint8 uResponse);    // 0x0501-(    ) 设置单元属性（role）


signals:
    // 主机成员变量相关信号,可绑定到界面槽函数，根据需要继续添加
    void signal_systemDatetime();               // 0x0204-(    ) 获取主机系统时间
    void signal_hostKernelVersion();            // 0x0202-(0x00) 获取主机版本号(Kernel)
    void signal_hostLinkVersion();              // 0x0202-(0x01) 获取主机版本号(Link)
    void signal_hostAntaBoxVersion();           // 0x0202-(0x02) 获取主机版本号(Anta)
    void signal_hostKernelBuildDate();          // 0x0203-(0x00) 获取主机编译日期(Kernel)
    void signal_hostLinkBuildDate();            // 0x0203-(0x01) 获取主机编译日期(Link)
    void signal_hostAntaBoxBuildDate();         // 0x0203-(0x02) 获取主机编译日期(Anta)

    void signal_speechMode();                   // 0x0208-(    ) 获取发言模式
    void signal_maxSpeechUnits();               // 0x0209-(    ) 获取最大发言数量

    void signal_wirelessVolume();               // 0x0205-(    ) 获取无线音量
    void signal_wiredVolume();                  // 0x0206-(    ) 获取有线音量
    void signal_antaBoxVolume();                // 0x0207-(    ) 获取天线盒音量

    void signal_wirelessAudioPath();            // 0x020D-(    ) 获取无线音频路径

    void signal_monitorAudioChannels();         // 0x0211-(    ) 获取监测音频信道数量
    void signal_unitCapacity();                 // 0x0210-(    ) 获取单元容量

    void signal_cameraTrackingProtocol();       // 0x020A-(    ) 获取摄像跟踪协议
    void signal_cameraTrackingAddress();        // 0x020B-(    ) 获取摄像跟踪地址
    void signal_cameraTrackingBaudrate();       // 0x020C-(    ) 获取摄像跟踪波特率

    void signal_monitorAudioChannelInfo();      // 0x0212-(    ) 获取监测音频信道信息
    void signal_audioChannelInfo();             // 0x0901/0x0404-(    ) 获取无线音频信道RSSI及FREQ(只需要一个信号)

    void signal_currentWirelessSpeakerId();     // 0x020E-(    ) 获取当前无线发言Id
    void signal_currentWiredSpeakerId();        // 0x020F-(    ) 获取当前有线发言Id

    void signal_wirelessPacketStatistics();     // 0x0401-(    ) 获取无线报文统计
    void signal_wiredPacketStatistics();        // 0x0402-(    ) 获取有线报文统计
    void signal_antaBoxPacketStatistics();      // 0x0403-(    ) 获取天线盒报文统计

    void signal_wirelessMgmtChannel();          // 0x0405-(    ) 获取无线管理信道RSSI

    void signal_unitInfo();                     // 0x0601-(    ) 获取“注册”单元信息
    void signal_unitAlias();                    // 0x0602-(    ) 获取单元别名
    void signal_unitWirelessStatus();           // 0x0606-(    ) 获取无线单元状态信息

    void signal_meetingName();                  // 0x0801-(    ) 获取会议名称


    // 响应接收信号 - 用于通知队列机制收到响应(ProtocolProcessor::dispatchFrame 最后发送信号，返回参数）
    void signal_responseReceived(quint8 address, uint8_t opCode, uint16_t command);  // 队列发送指令，收到响应信号
    void signal_sendQueueChange();              // 队列指令变化信号，队列中添加或者减少指令时发射


protected:
    void sendToHost(const QByteArray &data);

private:
    // 存储HostController属性的成员变量
    QString m_systemDatetime;               // 0x0204-(    ) 查询主机系统时间
    QString m_hostKernelVersion;            // 0x0202-(0x00) 查询主机版本号(Kernel)
    QString m_hostLinkVersion;              // 0x0202-(0x01) 查询主机版本号(Link)
    QString m_hostAntaBoxVersion;           // 0x0202-(0x02) 查询主机版本号(Anta)
    QString m_hostKernelBuildDate;          // 0x0203-(0x00) 查询主机编译日期(Kernel)
    QString m_hostLinkBuildDate;            // 0x0203-(0x01) 查询主机编译日期(Link)
    QString m_hostAntaBoxBuildDate;         // 0x0203-(0x02) 查询主机编译日期(Anta)

    QString m_speechMode;                   // 0x0208-(    ) 查询发言模式
    QString m_maxSpeechUnits;               // 0x0209-(    ) 查询最大发言数量

    QString m_wirelessVolume;               // 0x0205-(    ) 查询无线音量
    QString m_wiredVolume;                  // 0x0206-(    ) 查询有线音量
    QString m_antaBoxVolume;                // 0x0207-(    ) 查询天线盒音量

    QString m_wirelessAudioPath;            // 0x020D-(    ) 查询无线音频路径

    QString m_monitorAudioChannels;         // 0x0211-(    ) 查询监测音频信道数量
    QString m_unitCapacity;                 // 0x0210-(    ) 查询单元容量

    QString m_cameraTrackingProtocol;       // 0x020A-(    ) 查询摄像跟踪协议
    QString m_cameraTrackingAddress;        // 0x020B-(    ) 查询摄像跟踪地址
    QString m_cameraTrackingBaudrate;       // 0x020C-(    ) 查询摄像跟踪波特率


    QString m_wirelessAudioChannel;         // 0x0404-(    ) 查询无线音频信道RSSI， 不与0x0901协作时存储RSSI

    // 将监测通道（0x0212）与输出通道设定（0x0901）与输出通道读取（0x0404）合并成一个数据模型，与0x0901协作时存储RSSI
    QVector<audioChannelInfo> m_audioChannelData;       // 自定义的音频通道数据结构，包含检测通道和输出通道
    QMutex m_audioChannelDataMutex;                     // 数据访问互斥锁
    qint8 m_AudioRSSIndex = -1;                         // 由于0x0404不带频点索引，在与0x0901协用时作为计数索引
                                                        // 但是为了区别0901协用时的0404与普通的0404，m_AudioRSSIndex = -1为普通的0404
                                                        // m_AudioRSSIndex >=0 为协用时的0404


    QString m_currentWirelessSpeakerId;     // 0x020E-(    ) 查询当前无线发言Id
    QString m_currentWiredSpeakerId;        // 0x020F-(    ) 查询当前有线发言Id

    QString m_wirelessPacketStatistics;     // 0x0401-(    ) 查询无线报文统计
    QString m_wiredPacketStatistics;        // 0x0402-(    ) 查询有线报文统计
    QString m_antaBoxPacketStatistics;      // 0x0403-(    ) 查询天线盒报文统计

    QString m_wirelessMgmtChannel;          // 0x0405-(    ) 查询无线管理信道RSSI

    QVector<unitInformation> m_unitInfo;    // 0x0601-(    ) 查询“注册”单元信息
    QHash<quint16, QString> m_unitAliasMap;         // 0x0602-(    ) 查询单元别名映射表
    QString m_unitWirelessStatus;           // 0x0606-(    ) 查询无线单元状态信息

    QString m_meetingName;                  // 0x0801-(    ) 查询会议名称


    // 添加初始化指令列表和索引（用于旧的定时器方案）
    bool m_isInitialized;                           // 初始化状态，标记主机是否已初始化

    // 添加指令发送队列相关成员变量
    QQueue<SendQueueItem> m_sendQueue;              // 发送队列
    QString m_messageQueueSendCompleted;            // 用于队列指令发送完毕提示
    QTimer* m_timeoutTimer;                         // 超时定时器
    static const int QUEUE_TIMEOUT_MS = 5000;       // 队列发送时的超时阈值
    static const int QUEUE_INTERVAL_MS =  50;       // 队列发送时的时间间隔
    static const int QUEUE_DELAYED_MS = 2000;       // 队列中需要延时的时间


    // 声明ProtocolProcessor为友元类，以便被ProtocolProcessor访问私有成员变量
    friend class ProtocolProcessosr;

    // HostController对象属性
    quint8 m_address;                               // 主机地址指针
    SerialPortHandler *m_serialHandler;             // 主机调用串口指针
    ProtocolProcessor *m_protocolProcessor;         // 主机调用协议处理器指针

    // 缓存数据成员变量处理函数
    QString getCacheFileName() const;                       // 读取缓存文件名
    QVariantList m_audioChannelDataToQVariantList() const;  // 将音频通道信息转为List的函数
    QVariantList m_unitInfoToVariantList() const;           // 将“注册”单元信息转为List的函数
    QVariantMap m_unitAliasMapToVariantMap() const;         // 将单元别名映射转为 QVariantMap 的函数
};

#endif // HOSTCONTROLLER_H


/* 在HostController.h 增加新的属性的说明
 * 1. 添加变量成员（如 m_hostKernelVersion）
 * 2. 添加信号 （如 signal_hostKernelVersion）
 * 3. 添加变量设值函数 （如 save_hostKernelVersion()）
 * 4. 添加变量取值函数 （如 getter_hostKernelVersion() (内联函数)）
 * 5. 添加属性 （如 Q_PROPERTY(QString hostKernelVersion...)）
 *
 * ProtocolProcessor::dispatFrame()调用函数 save_hostKernelVersion()，将解析的数据帧和参数赋值给成员变量 m_hostKernelVersion
 * signal_hostKernelVersion() 发射信号 signal_hostKernelVersion，在成员变量值发生变化时调用内联函数 getter_hostKernelVersion() 提取值
 * HostController更新属性Q_PROPERTY(hostKernelVersion......), hostKernelVersion直接用于界面引用
 */
