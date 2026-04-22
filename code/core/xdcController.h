// XDC236 有线无线融合会议系统
#ifndef XDCCONTROLLER_H
#define XDCCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QDateTime>
#include <QTimer>

#include "ProtocolProcessor.h"
#include "UnitModel.h"

class HostManager;

/* -------------------枚举值------------------------*/
// 数据类别枚举 - 用于区分控制器中不同类型的数据变化
enum class DataType {
    Property,   // 属性数据（版本号、日期等只读状态）
    Setting,    // 设置数据（用户可配置参数，需持久化，如音量、增益）
    Info        // 信息数据（运行时动态信息，如温度、运行时间）
};
Q_DECLARE_METATYPE(DataType)

// 发言模式枚举
enum class SpeechMode {
    FIFO = 0x00,        // 先进先出
    SelfLock = 0x01     // 自锁模式
};
Q_DECLARE_METATYPE(SpeechMode)

// 无线音频路径枚举
enum class WirelessAudioPath {
    Host = 0x00,        // 主机
    AntennaBox = 0x01   // 天线盒
};
Q_DECLARE_METATYPE(WirelessAudioPath)

// 摄像机跟踪协议枚举
enum class CameraProtocol {
    None = 0x00,        // 无
    VISCA = 0x01,       // VISCA协议
    PELCO_D = 0x02,     // PELCO_D协议
    PELCO_P = 0x03      // PELCO_P协议
};
Q_DECLARE_METATYPE(CameraProtocol)

// 摄像机跟踪波特率枚举
enum class CameraBaudrate {
    B2400 = 0x00,       // 2400bps
    B4800 = 0x01,       // 4800bps
    B9600 = 0x02,       // 9600bps
    B115200 = 0x03      // 115200bps
};
Q_DECLARE_METATYPE(CameraBaudrate)
/* -------------------枚举值-----------------------------*/


// XDC 主机控制器 - 负责控制单个 XDC236 主机
class XdcController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint8_t address READ address CONSTANT FINAL)           // 主机地址（只读）

    // 使用 QVariantMap 暴露分组数据，供 QML 绑定（view 前缀）
    Q_PROPERTY(QVariantMap viewHardwareInfo READ viewHardwareInfo NOTIFY hardwareInfoChanged)
    Q_PROPERTY(QVariantMap viewVolumeSettings READ viewVolumeSettings NOTIFY volumeSettingsChanged)
    Q_PROPERTY(QVariantMap viewSpeechSettings READ viewSpeechSettings NOTIFY speechSettingsChanged)
    Q_PROPERTY(QVariantMap viewCameraSettings READ viewCameraSettings NOTIFY cameraSettingsChanged)
    Q_PROPERTY(QDateTime viewSystemDateTime READ viewSystemDateTime NOTIFY systemDateTimeUpdated)

    Q_PROPERTY(UnitModel* unitModel READ unitModel CONSTANT)  // 暴露单元模型给 QML

public:
    explicit XdcController(uint8_t hostAddress, HostManager* parent);
    virtual ~XdcController();

    // 状态导出/导入 - 用于切换保存/恢复
    QVariantMap exportState() const;
    void importState(const QVariantMap& state);

    // ======= 辅助函数 =======
    QString formatAddressHex(uint8_t address) { return QString::asprintf("0x%02X", address); } // 转换为标准格式 0x01
    QString trimNullTerminator(const QByteArray& data);     // 去除参数/值末尾的不可见空格


    // 发送指令函数
    Q_INVOKABLE QVariantList buildCommandQueue(const QVariantList& commands);  // 生成"批量指令集"

    void sendToHost(const QByteArray& data);        // 发送指令到主机
    Q_INVOKABLE void sendCommand(uint8_t opCode, uint16_t command, const QByteArray& payload = QByteArray(),
                                 const QVariant& userData = QVariant());  // 构建并发送单个指令
    Q_INVOKABLE void sendCommandQueue(const QVariantList& commands);  // 批量发送指令队列

    // 接收指令分发函数
    void handleIncomingFrame(const ProtocolProcessor::ParsedFrameData& frame);  // 处理接收到的指令帧


    // Getter
    uint8_t address() const { return m_hostAddress; }

    // 返回分组数据的 QVariantMap（供 QML 绑定）
    QVariantMap viewHardwareInfo() const { return m_hardwareInfo; }
    QVariantMap viewVolumeSettings() const { return m_volumeSettings; }
    QVariantMap viewSpeechSettings() const { return m_speechSettings; }
    QVariantMap viewCameraSettings() const { return m_cameraSettings; }
    QDateTime viewSystemDateTime() const { return m_systemDateTime; }

    // ========== 分组1：硬件信息相关接口 ==========
    Q_INVOKABLE void getHardwareInfo();             // 发送指令获取硬件信息（批量获取版本号和编译日期）

    // ========== 分组2：音量相关接口 ==========
    Q_INVOKABLE void getWirelessVolume();           // 发送指令获取无线音量
    Q_INVOKABLE void setWirelessVolume(uint8_t index);  // 发送指令设置无线音量 (0-8)
    Q_INVOKABLE void getWiredVolume();              // 发送指令获取有线音量
    Q_INVOKABLE void setWiredVolume(uint8_t index);     // 发送指令设置有线音量 (0-6)
    Q_INVOKABLE void getAntennaVolume();            // 发送指令获取天线盒音量
    Q_INVOKABLE void setAntennaVolume(uint8_t index);   // 发送指令设置天线盒音量 (0-8)

    // ========== 分组3：发言相关接口 ==========
    Q_INVOKABLE void getSpeechMode();               // 发送指令获取发言模式
    Q_INVOKABLE void setSpeechMode(uint8_t mode);   // 发送指令设置发言模式 (0=FIFO, 1=SelfLock)
    Q_INVOKABLE void getMaxSpeechCount();           // 发送指令获取最大发言数量
    Q_INVOKABLE void setMaxSpeechCount(uint8_t count);  // 发送指令设置最大发言数量 (1-4)
    Q_INVOKABLE void getMaxUnitCapacity();          // (0x0210) 发送指令获取最大注册单元数量
    Q_INVOKABLE void getCurrentWirelessSpeakerId(); // 发送指令获取当前无线发言ID
    Q_INVOKABLE void getCurrentWiredSpeakerId();    // 发送指令获取当前有线发言ID
    Q_INVOKABLE void getWirelessAudioPath();        // 发送指令获取无线音频路径
    Q_INVOKABLE void setWirelessAudioPath(uint8_t path);    // 发送指令设置无线音频路径 (0=主机, 1=天线盒)

    // ========== 分组4：摄像跟踪相关接口 ==========
    Q_INVOKABLE void getCameraProtocol();           // 发送指令获取摄像跟踪协议
    Q_INVOKABLE void setCameraProtocol(uint8_t protocol);   // 发送指令设置摄像跟踪协议 (0=None, 1=VISCA, 2=PELCO_D, 3=PELCO_P)
    Q_INVOKABLE void getCameraAddress();            // 发送指令获取摄像跟踪地址
    Q_INVOKABLE void setCameraAddress(uint8_t address);     // 发送指令设置摄像跟踪地址
    Q_INVOKABLE void getCameraBaudrate();           // 发送指令获取摄像跟踪波特率
    Q_INVOKABLE void setCameraBaudrate(uint8_t baudrate);   // 发送指令设置摄像跟踪波特率 (0=2400,1=4800,2=9600,3=115200)

    // ========== 分组7：系统时间接口 ==========
    Q_INVOKABLE void getSystemDateTime();                   // 发送指令获取系统时间
    Q_INVOKABLE void setSystemDateTime(uint32_t utcTime);   // 发送指令设置系统时间 (UTC时间戳)

    // ========== 分组8：音频通道信息接口 ==========
    /// 单个无线信道数据
    struct WirelessChannelData {
        quint32 frequency = 0;      // 工作频率 (Hz)
        qint8 rssi = 0;             // 信号强度 (dBm)

        // 辅助方法
        double freqKHz() const { return frequency / 1000.0; }
        QString freqKHzStr() const { return QString::number(freqKHz(), 'f', 1); }

        QVariantMap toVariantMap() const {
            QVariantMap map;
            map["frequency"] = frequency;
            map["freqKHz"] = freqKHzStr();
            map["rssi"] = rssi;
            return map;
        }
    };

    /// 完整的频率点数据
    struct FreqPointData {
        quint8 fid = 0;                     // 频率点序号

        quint32 monitorFreq = 0;            // 监测信道频率 (Hz)
        qint8 monitorRSSI = 0;              // 监测信道信号强度 (dBm)

        // 4个无线信道数据
        WirelessChannelData wireless[4];

        // ---------- 辅助方法 ----------
        double monitorFreqKHz() const { return monitorFreq / 1000.0; }
        QString monitorFreqKHzStr() const { return QString::number(monitorFreqKHz(), 'f', 1); }

        // 获取无线信道（带边界检查）
        const WirelessChannelData& getWireless(int cid) const {
            static const WirelessChannelData empty;
            return (cid >= 0 && cid < 4) ? wireless[cid] : empty;
        }

        // 设置无线信道频率
        void setWirelessFreq(int cid, quint32 freq) {
            if (cid >= 0 && cid < 4) {
                wireless[cid].frequency = freq;
            }
        }

        // 设置无线信道RSSI
        void setWirelessRSSI(int cid, qint8 rssi) {
            if (cid >= 0 && cid < 4) {
                wireless[cid].rssi = rssi;
            }
        }

        // 同时设置频率和RSSI
        void setWirelessData(int cid, quint32 freq, qint8 rssi) {
            if (cid >= 0 && cid < 4) {
                wireless[cid].frequency = freq;
                wireless[cid].rssi = rssi;
            }
        }

        // 重置所有数据
        void reset() {
            fid = 0;
            monitorFreq = 0;
            monitorRSSI = 0;
            for (int i = 0; i < 4; ++i) {
                wireless[i].frequency = 0;
                wireless[i].rssi = 0;
            }
        }

        // 转换为 QVariantMap（供 QML 使用）
        QVariantMap toVariantMap() const {
            QVariantMap map;
            map["fid"] = fid;

            // 监测信道
            map["monitorFreq"] = monitorFreq;
            map["monitorFreqKHz"] = monitorFreqKHzStr();
            map["monitorRSSI"] = monitorRSSI;

            // 无线信道 - 扁平化字段（供 QML 直接访问）
            for (int i = 0; i < 4; ++i) {
                map[QString("wireless%1Freq").arg(i)] = wireless[i].frequency;
                map[QString("wireless%1FreqKHz").arg(i)] = wireless[i].freqKHzStr();
                map[QString("wireless%1RSSI").arg(i)] = wireless[i].rssi;
            }

            // 同时也保留列表形式（供其他用途）
            QVariantList wirelessList;
            QVariantList wirelessFreqList;
            QVariantList wirelessRSSIList;
            for (int i = 0; i < 4; ++i) {
                wirelessList.append(wireless[i].toVariantMap());
                wirelessFreqList.append(wireless[i].frequency);
                wirelessRSSIList.append(wireless[i].rssi);
            }
            map["wireless"] = wirelessList;
            map["wirelessFreq"] = wirelessFreqList;
            map["wirelessRSSI"] = wirelessRSSIList;

            return map;
        }
    };

    // ========== 音频测试公共接口 ==========

    /// 启动完整音频测试（三步流程自动执行）
    Q_INVOKABLE void startAudioTest();
    /// 停止音频测试
    Q_INVOKABLE void stopAudioTest();
    /// 第一步：获取监测信道数量
    Q_INVOKABLE void getMonitorChannelCount();
    /// 第二步：获取监测信道信息
    Q_INVOKABLE void getAllMonitorChannels();
    /// 第三步：测试无线信道
    Q_INVOKABLE void startWirelessChannelTest();

    /// 配置参数：设置信号稳定延时
    Q_INVOKABLE void setStabilizeDelayMs(int delayMs);
    Q_INVOKABLE int getStabilizeDelayMs() const;

    /// 状态查询
    Q_INVOKABLE bool isAudioTesting() const { return m_isAudioTesting; }
    Q_INVOKABLE QString getAudioTestStep() const;
    Q_INVOKABLE int getFrequencyPointCount() const { return m_freqPoints.size(); }
    Q_INVOKABLE int getCurrentFid() const { return m_currentFid; }
    Q_INVOKABLE int getCurrentCid() const { return m_currentCid; }

    /// 数据获取（供 QML 绑定）
    Q_INVOKABLE QVariantList getAllFreqPoints() const;
    Q_INVOKABLE QVariantMap getFreqPoint(int fid) const;
    Q_INVOKABLE QVariantList getUnifiedTableModel() const;

    ///QML 属性绑定 ==========
    Q_PROPERTY(QVariantList unifiedTableModel READ getUnifiedTableModel NOTIFY unifiedTableModelChanged)
    Q_PROPERTY(int freqPointCount READ getFrequencyPointCount NOTIFY freqPointCountChanged)
    Q_PROPERTY(bool isAudioTesting READ isAudioTesting NOTIFY audioTestCompleted)

    // 单元管理
    UnitModel* unitModel() const { return m_unitModel; }  // 获取单元模型
    Q_INVOKABLE void getAllUnits();                 // 查询所有联机单元
    Q_INVOKABLE void refreshSpeakingStatus();       // 刷新发言状态


signals:
    // ========== 操作反馈信号 ==========
    void operationResult(uint8_t address, const QString& operation, bool success, const QString& message);
    void commandFailed(uint16_t command, const QString& error);

    // ========== 批量/状态信号（内部使用）==========
    void dataBatchUpdated(uint8_t address, DataType type, const QVariantMap& data);
    void stateSnapshot(uint8_t address, const QVariantMap& snapshot);

    // ========== 生命周期信号（HostManager 使用）==========
    void aboutToDeactivate(uint8_t address);
    void activated(uint8_t address);

    // ========== 分组数据变化信号（QML 绑定）==========
    ///分组1：硬件信息相关接口
    void hardwareInfoChanged();
    ///分组2：音量相关接口
    void volumeSettingsChanged();
    ///分组3：发言相关接口
    void speechSettingsChanged();
    //分组4：摄像跟踪相关接口
    void cameraSettingsChanged();
    ///分组7：系统时间接口
    void systemDateTimeUpdated(uint8_t address, const QDateTime& dateTime);
    ///分组8：音频测试信号
    void audioTestProgress(int step, int current, int total, const QString& message);    /// 音频测试进度信号
    void audioTestStepChanged(const QString& step);    /// 测试步骤变更信号
    void audioTestCompleted(bool success, const QString& message);    /// 音频测试完成信号
    void monitorChannelCountReceived(int count);    /// 监测信道数量接收信号
    void freqPointCountChanged();    /// 频率点数量变更信号
    void freqPointUpdated(int fid, quint32 monitorFreq, qint8 monitorRSSI);    /// 频率点数据更新信号（监测信道）
    void wirelessChannelUpdated(int fid, int cid, quint32 wirelessFreq, qint8 wirelessRSSI);    /// 无线信道数据更新信号
    void unifiedTableModelChanged();    /// 统一表格模型变更信号

    // 单元检测信号
    void unitListSyncCompleted(int count);      // 查询单元列表，发送信号

private slots:
    // ========== 协议处理器回调（槽函数）==========
    void onCommandResponseCompleted(const ProtocolProcessor::ResponseResult& result);  // 处理请求-响应指令，响应完成处理
    void onCommandFailed(const ProtocolProcessor::PendingRequest& request, const QString& error);  // 请求失败处理
    void onUnsolicitedNotifyReceived(const QByteArray& rawFrame, 
                                     const ProtocolProcessor::ParsedFrameData& parsedData);     // 处理主动上报通知（opCode=0x02）

    // =============== 单元操作槽函数（由 UnitModel 信号触发） ==================
    // XdcController 中用于响应 UnitModel 发出的操作请求的。
    // 它们的作用是：将用户在界面上对单元的操控（如打开、关闭、删除、设置身份、设置别名、刷新无线状态）转换成具体的 RS485 协议指令，并通过主机发送给物理单元。
    void onUnitModelRequestOpenUnit(uint16_t unitId);       // 0x0504（点击"打开发言"按钮）
    void onUnitModelRequestCloseUnit(uint16_t unitId);      // 0x0505（点击"关闭发言"按钮）
    void onUnitModelRequestDeleteUnit(uint16_t unitId);     // 0x0503（点击"删除单元"按钮）
    void onUnitModelRequestSetIdentity(uint16_t unitId, uint8_t identity);  // 0x0501（设置单元信息，修改单元身份（代表/VIP/主席））
    void onUnitModelRequestSetAlias(uint16_t unitId, const QString& alias); // 0x0502（设置,修改单元别名）
    void onUnitModelRequestRefreshWirelessState(uint16_t unitId);       // 0x0606（点击"刷新状态"按钮（仅无线单元））

private:
    // ==================== XDC 协议命令码常量（按功能分组）====================
    /**
     * @brief XDC 协议命令码常量
     * 
     * 命名规范：
     * - GET_XDC_<模块>_<操作>：获取类命令
     * - SET_XDC_<模块>_<操作>：设置类命令
     * 
     * 使用方式：Command::<模块>::<操作>
     * 示例：Command::Volume::GET_XDC_WIRELESS
     */
    struct Command {
        // 系统时间相关 (0x01xx/0x02xx)
        struct SystemTime {
            static constexpr uint16_t GET_XDC_DATETIME     = 0x0204;  ///< 获取系统时间
            static constexpr uint16_t SET_XDC_DATETIME     = 0x0104;  ///< 设置系统时间
        };
        // 音量控制相关 (0x01xx/0x02xx)
        struct Volume {
            static constexpr uint16_t GET_XDC_WIRELESS   = 0x0205;  ///< 获取无线通道音量
            static constexpr uint16_t SET_XDC_WIRELESS   = 0x0105;  ///< 设置无线通道音量
            static constexpr uint16_t GET_XDC_WIRED      = 0x0206;  ///< 获取有线通道音量
            static constexpr uint16_t SET_XDC_WIRED      = 0x0106;  ///< 设置有线通道音量
            static constexpr uint16_t GET_XDC_ANTENNA    = 0x0207;  ///< 获取天线盒通道音量
            static constexpr uint16_t SET_XDC_ANTENNA    = 0x0107;  ///< 设置天线盒通道音量
        };

        // 发言模式相关 (0x01xx/0x02xx)
        struct Speech {
            static constexpr uint16_t GET_XDC_MODE       = 0x0208;  ///< 获取发言模式
            static constexpr uint16_t SET_XDC_MODE       = 0x0108;  ///< 设置发言模式
            static constexpr uint16_t GET_XDC_MAX_COUNT  = 0x0209;  ///< 获取最大发言数量
            static constexpr uint16_t SET_XDC_MAX_COUNT  = 0x0109;  ///< 设置最大发言数量
        };

        // 单元管理相关 (0x02xx/0x05xx/0x06xx)
        struct Unit {
            static constexpr uint16_t GET_XDC_MAX_REGISTER     = 0x0210;  ///< 获取最大注册单元数量
            static constexpr uint16_t GET_XDC_CURRENT_WIRELESS = 0x020E;  ///< 获取当前无线发言ID
            static constexpr uint16_t GET_XDC_CURRENT_WIRED    = 0x020F;  ///< 获取当前有线发言ID
            static constexpr uint16_t GET_XDC_INFO             = 0x0601;  ///< 获取单元信息
            static constexpr uint16_t SET_XDC_OPEN             = 0x0504;  ///< 开启单元
            static constexpr uint16_t SET_XDC_CLOSE            = 0x0505;  ///< 关闭单元
            static constexpr uint16_t SET_XDC_DELETE           = 0x0503;  ///< 删除单元
            static constexpr uint16_t SET_XDC_IDENTITY         = 0x0501;  ///< 设置单元身份
            static constexpr uint16_t SET_XDC_ALIAS            = 0x0502;  ///< 设置单元别名
            static constexpr uint16_t SET_XDC_REFRESH_WIRELESS = 0x0606;  ///< 刷新无线单元状态
        };

        // 音频路径相关 (0x01xx/0x02xx)
        struct AudioPath {
            static constexpr uint16_t GET_XDC_PATH         = 0x020D;  ///< 获取音频输出路径
            static constexpr uint16_t SET_XDC_PATH         = 0x010D;  ///< 设置音频输出路径
        };

        // 摄像跟踪相关 (0x01xx/0x02xx)
        struct Camera {
            static constexpr uint16_t GET_XDC_PROTOCOL     = 0x020A;  ///< 获取摄像机协议
            static constexpr uint16_t SET_XDC_PROTOCOL     = 0x010A;  ///< 设置摄像机协议
            static constexpr uint16_t GET_XDC_ADDRESS      = 0x020B;  ///< 获取摄像机地址
            static constexpr uint16_t SET_XDC_ADDRESS      = 0x010B;  ///< 设置摄像机地址
            static constexpr uint16_t GET_XDC_BAUDRATE     = 0x020C;  ///< 获取摄像机波特率
            static constexpr uint16_t SET_XDC_BAUDRATE     = 0x010C;  ///< 设置摄像机波特率
        };

        // 频率和 RSSI 相关 (0x04xx/0x09xx)
        struct Frequency {
            static constexpr uint16_t SET_XDC_FREQ         = 0x0901;  ///< 设置频点
            static constexpr uint16_t GET_XDC_RSSI         = 0x0404;  ///< 读取无线信道 RSSI
            static constexpr uint16_t GET_XDC_MONITOR_COUNT = 0x0211; ///< 获取监测信道数量
            static constexpr uint16_t GET_XDC_MONITOR_INFO  = 0x0212; ///< 获取监测信道信息

        };
    };

    // ==================== userData Key 前缀常量 ====================
    /**
     * @brief 指令 userData Key 前缀常量
     * 
     * 用于批次管理和请求清除
     * 完整 Key 格式："{prefix}{参数}"
     * 示例："getUnitInfo_0x0001"
     */
    struct KeyPrefix {
        // 单元查询和控制（带参数）
        static constexpr const char* UNIT_INFO        = "getUnitInfo_";
        static constexpr const char* OPEN_UNIT        = "openUnit_";
        static constexpr const char* CLOSE_UNIT       = "closeUnit_";
        static constexpr const char* DELETE_UNIT      = "deleteUnit_";
        static constexpr const char* SET_IDENTITY     = "setIdentity_";
        static constexpr const char* SET_ALIAS        = "setAlias_";
        static constexpr const char* REFRESH_WIRELESS = "refreshWireless_";
        
        // 频率和 RSSI（带参数）
        static constexpr const char* SET_FREQ         = "setFreq_";
        static constexpr const char* READ_RSSI        = "readRSSI_";
        
        // 批次查询前缀
        static constexpr const char* UNIT_QUERY_BATCH = "unitQuery_";
    };

    // 批量发送指令的批次上下文结构体
    struct BatchContext {
        QString batchId;
        int expected = 0;
        int received = 0;
        bool completed = false;
        QString operation;  // 可选，用于调试
    };
    QMap<QString, BatchContext> m_activeBatches;  // 活跃批次映射表

        // 类成员变量
    uint8_t m_hostAddress;                          // 主机地址
    HostManager* m_hostManager;                     // 主机管理器指针
    ProtocolProcessor* m_protocolProcessor;         // 协议处理器指针

    // 私有方法
    void setupProtocolProcessor();                  // 设置协议处理器
    void parseAndDispatch(const ProtocolProcessor::ParsedFrameData& frame);  // 解析并分发响应数据
    void handleSetResponse(const ProtocolProcessor::ParsedFrameData& frame, const QString& operation);  // 通用设置响应处理

    // 批量指令的辅助方法，处理多主机切换时数据保存与加载
    void notifyBatchDataUpdate(DataType type, const QVariantMap& data);  // importState() 中恢复状态后，通知批量数据已恢复
    void notifyStateSnapshot();      // 所有属性加载完成后，通知外部可以保存状态，供 HostManager 切换主机时保存状态

    // ================= 单元管理 ==================
    // 单元指针及模型信号连接函数
    UnitModel* m_unitModel = nullptr;   // 单元指针
    void connectUnitModelSignals();     // 主机模型连接单元

    // 单元遍历状态 ------------------------
    uint16_t m_currentQueryUnitId = 0x0001;     // 当前查询单元Id
    uint16_t m_maxUnitId = 10;      // 最大注册单元数量
    bool m_isQueryingAllUnits = false;
    QString m_currentBatchId;       // 当前查询批次ID（用于清除旧请求）

    // 单元响应处理 ------------------------
    void handleUnitInfoResponse(const ProtocolProcessor::ParsedFrameData& frame);      // 0x0601
    void handleUnitAliasResponse(const ProtocolProcessor::ParsedFrameData& frame);     // 0x0602
    void handleWirelessStateResponse(const ProtocolProcessor::ParsedFrameData& frame); // 0x0606
    void handleUnitOperationResponse(const ProtocolProcessor::ParsedFrameData& frame); // 0x0501/0x0502/0x0503/0x0504/0x0505
    void handleUnitCurrentSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData& frame); // 0x020E/0x020F

    // 单元辅助方法 ------------------------
    void requestNextUnitInfo();

    //========分组1：硬件信息响应处理（不变）
    void handleVersionResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleBuildDateResponse(const ProtocolProcessor::ParsedFrameData& frame);
    /// 用于 QML 绑定的 QVariantMap（直接存储数据，无需额外结构体）
    QVariantMap m_hardwareInfo;     // 硬件信息（版本号、编译日期等）

    //========分组2：音量响应处理
    void handleWirelessVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleWiredVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleAntennaVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame);
    ///用于 QML 绑定的 QVariantMap（直接存储数据，无需额外结构体）
    QVariantMap m_volumeSettings;   // 音量设置（无线、有线、天线盒）

    //========分组3：发言相关响应处理
    void handleSpeechModeResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleMaxSpeechCountResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleMaxUnitCapacityResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleWirelessAudioPathResponse(const ProtocolProcessor::ParsedFrameData& frame);
    // 用于 QML 绑定的 QVariantMap（直接存储数据，无需额外结构体）
    QVariantMap m_speechSettings;   // 发言设置（模式、人数、发言ID等）

    //========分组4：摄像跟踪响应处理
    void handleCameraProtocolResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleCameraAddressResponse(const ProtocolProcessor::ParsedFrameData& frame);
    void handleCameraBaudrateResponse(const ProtocolProcessor::ParsedFrameData& frame);
    // 用于 QML 绑定的 QVariantMap（直接存储数据，无需额外结构体）
    QVariantMap m_cameraSettings;   // 摄像跟踪设置（协议、地址、波特率）

    //========分组7：系统时间响应处理
    void handleSystemDateTimeResponse(const ProtocolProcessor::ParsedFrameData& frame);
    QDateTime m_systemDateTime;     // 系统时间

    //========分组8：音频通道信息响应处理 ===========
    void handleMonitorChannelCountResponse(const ProtocolProcessor::ParsedFrameData& frame);    // 0x0211响应，总频点数
    void handleMonitorChannelResponse(const ProtocolProcessor::ParsedFrameData& frame);         // 0x0212响应，监测通道数据
    void handleSetFrequencyResponse(const ProtocolProcessor::ParsedFrameData& frame);           // 0x0901响应，设置无线通道频率
    void handleReadRSSIResponse(const ProtocolProcessor::ParsedFrameData& frame);               // 0x0404响应，测试无线通道信号

    // 无线音频信道处理 私有方法
    void resetAudioTestState();
    void requestNextMonitorChannel();
    void setNextWirelessFrequency();
    void readWirelessRSSI();
    void advanceToNextTest();
    void finishAudioTest(bool success, const QString& message);
    QString stepToString(int step) const;
    void updateUnifiedTableModel();

    // 无线音频信道处理  成员变量
    bool m_isAudioTesting = false;  // 测试状态
    int m_audioTestStep = 0;        // 0-空闲, 1-获取数量, 2-获取监测信道, 3-测试无线信道

    QVector<FreqPointData> m_freqPoints;    // 统一数据存储（核心数据结构），监测信道和无线信道所有频率点的测试数据
    int m_freqPointCount = 0;               // 频率点数量（0x0211指令查询）

    int m_currentMonitorIndex = 0;          // 测试进度控制，当前请求的监测信道索引
    int m_currentFid = 0;                   // 测试进度控制，当前测试的频率点索引
    int m_currentCid = 1;                   // 测试进度控制，当前测试的无线信道号 (1-4)
    bool m_waitingForRSSI = false;
    bool m_frequencySetPending = false;

    int m_stabilizeDelayMs = 2000;          // 配置参数，发送0x0901设置频率与0x0404读取信号强度指令之间的时间间隔。信号稳定延时（毫秒）

    QVariantList m_unifiedTableModel;       // 表格模型缓存
};

#endif // XDCCONTROLLER_H