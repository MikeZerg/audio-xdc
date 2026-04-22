// code/core/ProtocolProcessor.h
#ifndef PROTOCOLPROCESSOR_H
#define PROTOCOLPROCESSOR_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QQueue>
#include <QTimer>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <functional>

/**
 * @brief 协议处理器 - 集成协议编解码和请求 - 响应管理
 * 
 * 功能模块：
 * 1. 协议帧构建、解析、验证（静态方法）
 * 2. 请求 - 响应匹配（实例方法）
 * 3. 指令队列管理（实例方法）
 * 4. 超时重试（实例方法）
 * 5. 通知处理（实例方法）
 */
class ProtocolProcessor : public QObject  // 👈 必须继承 QObject
{
    Q_OBJECT

public:
    // ==================== 帧类型枚举 ====================
    enum FrameType {
        Unknown,       ///< 未知帧类型
        Request,       ///< 请求帧，从主设备发送到从设备
        Response,      ///< 响应帧，从设备对请求的回复
        Notification   ///< 通知帧，单向通知不需要回复
    };
    Q_ENUM(FrameType)

    // ==================== 解析后的帧数据结构 ====================
    struct ParsedFrameData {
        uint8_t destAddr;       ///< 目标地址
        uint8_t srcAddr;        ///< 源地址
        uint8_t opCode;         ///< 操作码
        uint16_t command;       ///< 命令码
        QByteArray payload;     ///< 负载数据
        FrameType frameType;    ///< 帧类型
        
        ParsedFrameData() 
            : destAddr(0), srcAddr(0), opCode(0), command(0)
            , frameType(Unknown) {}
    };

    // ==================== 静态方法：协议编解码（原有功能保持不变）====================
    
    /**
     * @brief 构建协议帧
     * @param destAddr 目标地址
     * @param srcAddr 源地址
     * @param opCode 操作码
     * @param command 命令码
     * @param payload 负载数据
     * @return 完整的协议帧
     */
    static QByteArray buildFrame(uint8_t destAddr, uint8_t srcAddr,
                                 uint8_t opCode, uint16_t command,
                                 const QByteArray &payload = QByteArray());
    
    /**
     * @brief 解析协议帧
     * @param frame 原始帧数据
     * @param parsedData 输出：解析后的数据
     * @return 是否解析成功
     */
    static bool parseFrame(const QByteArray &frame, ParsedFrameData &parsedData);
    
    /**
     * @brief 验证协议帧格式和校验和
     * @param frame 帧数据
     * @return 是否验证通过
     */
    static bool validateFrame(const QByteArray &frame);
    
    /**
     * @brief 从缓冲区提取完整帧
     * @param buffer 接收缓冲区（会修改）
     * @return 提取到的完整帧列表
     */
    static QList<QByteArray> extractFramesFromBuffer(QByteArray &buffer);
    
    /**
     * @brief 判断响应帧是否匹配请求帧
     * @param requestFrame 请求帧数据
     * @param responseFrame 响应帧数据
     * @return 是否匹配
     */
    static bool isMatchingResponse(const ParsedFrameData &requestFrame,
                                   const ParsedFrameData &responseFrame);
    
    /**
     * @brief 帧类型转字符串
     * @param type 帧类型
     * @return 类型描述
     */
    static QString frameTypeToString(FrameType type);
    
    /**
     * @brief 计算校验和
     * @param data 数据
     * @param start 起始位置
     * @param length 长度（-1 表示到末尾）
     * @return 校验和
     */
    static uint8_t calculateChecksum(const QByteArray &data, int start = 0, int length = -1);


    // ==================== request-response 请求响应管理相关结构 ====================
    /**
     * @brief 指令配置信息
     */
    struct CommandConfig {
        uint8_t destAddr;         ///< 目标地址（接收方）
        uint8_t srcAddr;          ///< 源地址（发送方）
        uint8_t opCode;           ///< 操作码
        uint16_t command;         ///< 命令码
        QByteArray payload;       ///< 负载数据
        
        CommandConfig() 
            : destAddr(0), srcAddr(0x80), opCode(0), command(0) {}
        
        CommandConfig(uint8_t dest, uint8_t src, uint8_t op, uint16_t cmd, 
                      const QByteArray &data = QByteArray())
            : destAddr(dest), srcAddr(src), opCode(op), command(cmd), payload(data) {}
    };

    /**
     * @brief 匹配规则（用于识别响应）
     */
    struct MatchRule {
        uint8_t expectedSrcAddr;      ///< 期望的响应源地址
        uint8_t expectedOpCode;       ///< 期望的响应操作码
        uint16_t expectedCommand;     ///< 期望的响应命令码
        
        MatchRule() : expectedSrcAddr(0), expectedOpCode(0), expectedCommand(0) {}
        
        MatchRule(uint8_t src, uint8_t op, uint16_t cmd)
            : expectedSrcAddr(src), expectedOpCode(op), expectedCommand(cmd) {}
        
        /**
         * @brief 从指令配置创建匹配规则
         */
        static MatchRule fromCommandConfig(const CommandConfig &config) {
            return MatchRule(config.destAddr, 0x04, config.command);
        }
        
        /**
         * @brief 检查是否匹配响应帧
         */
        bool matches(const ParsedFrameData& frame) const {
            return frame.srcAddr == expectedSrcAddr &&
                   frame.opCode == expectedOpCode &&
                   frame.command == expectedCommand;
        }
    };

    /**
     * @brief 待发送的请求项
     */
    struct PendingRequest {
        CommandConfig commandConfig;    ///< 指令配置
        QByteArray rawFrame;            ///< 构建好的完整帧
        MatchRule matchRule;            ///< 匹配规则
        QDateTime sendTime;             ///< 发送时间戳
        int retryCount = 0;             ///< 已重试次数
        int maxRetries = 3;             ///< 最大重试次数
        bool isWaitingResponse = false; ///< 是否正在等待响应
        QVariant userData;              ///< 用户关联数据
        
        /**
         * @brief 检查是否匹配响应
         */
        bool matchesResponse(const ParsedFrameData& frame) const {
            return matchRule.matches(frame);
        }
    };

    /**
     * @brief 响应结果封装（返回给 Controller）
     */
    struct ResponseResult {
        bool success;                               ///< 是否成功
        CommandConfig commandConfig;                ///< 原始指令配置
        QByteArray requestFrame;                    ///< 原始请求帧
        QByteArray responseFrame;                   ///< 原始响应帧
        ParsedFrameData parsedData;                 ///< 解析后的响应数据
        PendingRequest pendingRequest;              ///< 关联的请求信息
        QString errorMessage;                       ///< 错误信息（如果失败）
        
        ResponseResult() : success(false) {}
    };

    /**
     * @brief 通信状态机状态
     */
    enum class CommState {
        Idle,       ///< 空闲：可以发送新指令
        Waiting,    ///< 等待响应
        Retrying,   ///< 重试中
        Error       ///< 错误
    };
    Q_ENUM(CommState)

    // ==================== 构造函数和析构函数 ====================
    explicit ProtocolProcessor(QObject *parent = nullptr);
    ~ProtocolProcessor();

    // ==================== 配置参数 ====================
    
    /**
     * @brief 设置响应超时时间（毫秒）
     */
    void setResponseTimeout(int timeoutMs);
    
    /**
     * @brief 设置最大重试次数
     */
    void setMaxRetries(int maxRetries);
    
    /**
     * @brief 设置指令间延迟（毫秒）
     */
    void setInterCommandDelay(int delayMs);
    
    // Getter
    int getResponseTimeout() const { return m_responseTimeoutMs; }
    int getMaxRetries() const { return m_maxRetries; }
    int getInterCommandDelay() const { return m_interCommandDelayMs; }

    // ==================== 核心功能：发送指令 ====================
    
    /**
     * @brief 发送指令（自动加入队列）
     * @param config 指令配置
     * @param maxRetries 最大重试次数（-1 使用默认值）
     * @param userData 用户关联数据（用于区分请求）
     */
    void sendCommand(const CommandConfig &config,
                     int maxRetries = -1,
                     const QVariant &userData = QVariant());
    
    /**
     * @brief 批量发送指令
     * @param requests 请求列表
     */
    void sendCommandQueue(const QVector<PendingRequest> &requests);
    
    /**
     * @brief 清空队列
     */
    void clearQueue();
    
    /**
     * @brief 按 userData 前缀清除请求（精细化清除）
     * @param prefix userData 的前缀字符串
     * @return 清除的请求数量
     *
     * 用途：清除特定批次的请求，不影响其他功能的指令
     * 示例：clearCommandsByUserDataPrefix("unitQuery_") 清除所有单元查询请求
     */
    int clearCommandsByUserDataPrefix(const QString& prefix);

    /**
     * @brief 获取队列中待处理的请求数量
     */
    int pendingCount() const { return m_requestQueue.size(); }
    
    /**
     * @brief 当前是否空闲状态
     */
    bool isIdle() const { return m_state == CommState::Idle; }
    
    /**
     * @brief 获取当前状态
     */
    CommState currentState() const { return m_state; }

    // ==================== 通知处理 ====================
    
    using NotificationHandler = std::function<void(const QByteArray&, const ParsedFrameData&)>;
    
    /**
     * @brief 注册通知处理器（用于非 request-response 指令）
     * @param opCode 操作码
     * @param command 命令码
     * @param callback 处理回调
     */
    void registerNotificationHandler(uint8_t opCode, uint16_t command, 
                                     NotificationHandler callback);
    
    /**
     * @brief 注销通知处理器
     */
    void unregisterNotificationHandler(uint8_t opCode, uint16_t command);

    // ==================== 外部接口 ====================
    
    using SendFunction = std::function<void(const QByteArray&)>;
    
    /**
     * @brief 设置发送函数（由外部提供，用于发送原始帧）
     */
    void setSendFunction(SendFunction func);
    
    /**
     * @brief 处理接收到的响应帧
     * @param rawFrame 完整的原始响应帧
     * @return 是否成功匹配和处理
     */
    bool handleIncomingFrame(const QByteArray &rawFrame);

signals:
    /**
     * @brief 未注册的主动上报通知（opCode=0x02）
     */
    void unsolicitedNotifyReceived(const QByteArray& rawFrame, const ProtocolProcessor::ParsedFrameData& parsedData);

    /**
     * @brief 响应完成信号（通知 Controller：某个请求已成功收到响应并完成匹配）
     */
    void responseCompleted(const ProtocolProcessor::ResponseResult &result);

    /**
     * @brief 请求失败信号（通知 Controller：某个请求超时或失败，超过最大重试次数）
     */
    void requestFailed(const ProtocolProcessor::PendingRequest &request, const QString &error);

    /**
     * @brief 状态变化信号（通知外部当前通信状态机的状态变化（用于调试或 UI 状态指示））
     */
    void stateChanged(ProtocolProcessor::CommState newState, ProtocolProcessor::CommState oldState);

    /**
     * @brief 请求发送信号（通知外部：某个请求已经实际发送到硬件）
     */
    void requestSent(const ProtocolProcessor::PendingRequest &request);

    /**
     * @brief 队列处理完成信号
     */
    void queueProcessed();

private slots:
    // ==================== 私有槽函数 ====================
    void processNextRequest();      ///< 处理下一个请求
    void onResponseTimeout();       ///< 响应超时处理
    void resendCurrentRequest();    ///< 重发当前请求

private:
    // ==================== 内部方法 ====================
    bool sendCurrentRequest();                  ///< 发送当前请求
    void switchState(CommState newState);       ///< 切换状态
    int calculateBackoffDelay(int retryCount);  ///< 计算退避延迟

    // ==================== 成员变量 ====================
    
    // 协议处理相关（原有）
    QByteArray m_receivedBuffer;                ///< 接收缓冲区
    
    // 请求响应管理相关（新增）
    CommState m_state = CommState::Idle;        ///< 当前状态
    QQueue<PendingRequest> m_requestQueue;      ///< 请求队列
    PendingRequest m_currentRequest;            ///< 当前正在处理的请求
    
    QTimer* m_responseTimer;                    ///< 响应超时定时器
    int m_responseTimeoutMs = 500;              ///< 响应超时时间
    int m_maxRetries = 3;                       ///< 最大重试次数
    int m_interCommandDelayMs = 50;             ///< 指令间延迟
    int m_retryCount = 0;                       ///< 当前重试次数
    
    SendFunction m_sendFunction;                ///< 发送函数
    
    QMap<quint32, NotificationHandler> m_notificationHandlers;  ///< 通知处理器映射

    // ==================== 私有常量（静态方法使用）====================
private:
    static constexpr uint8_t FRAME_HEADER = 0x55;         ///< 帧头标识
    static constexpr uint8_t FRAME_FOOTER = 0xAA;         ///< 帧尾标识
    
    static constexpr int MIN_FRAME_SIZE = 10;             ///< 最小帧长度（无负载）
    static constexpr int POS_HEADER = 0;                  ///< 帧头位置
    static constexpr int POS_LENGTH = 1;                  ///< 长度位置
    static constexpr int POS_LEN_COMPLEMENT = 2;          ///< 长度反码位置
    static constexpr int POS_DEST_ADDR = 3;               ///< 目标地址位置
    static constexpr int POS_SRC_ADDR = 4;                ///< 源地址位置
    static constexpr int POS_OPCODE = 5;                  ///< 操作码位置
    static constexpr int POS_COMMAND_HIGH_ = 6;           ///< 命令码高字节位置
    static constexpr int POS_COMMAND_LOW = 7;             ///< 命令码低字节位置
    static constexpr int POS_PAYLOAD_START = 8;           ///< 负载起始位置
};

#endif // PROTOCOLPROCESSOR_H