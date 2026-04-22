// code/core/ProtocolProcessor.cpp
#include "ProtocolProcessor.h"
#include "../library/LogManager.h"
#include <QDebug>
#include <QtGlobal>

// 构造函数
ProtocolProcessor::ProtocolProcessor(QObject *parent)
    : QObject(parent)
    , m_responseTimer(new QTimer(this))
{
    connect(m_responseTimer, &QTimer::timeout, 
            this, &ProtocolProcessor::onResponseTimeout);
}

// 析构函数
ProtocolProcessor::~ProtocolProcessor()
{
    m_responseTimer->stop();
}

// ======== 发送指令功能 ========
// 构建帧函数
QByteArray ProtocolProcessor::buildFrame(uint8_t destAddr, uint8_t srcAddr, uint8_t opCode, uint16_t command, const QByteArray &payload)
{
    QByteArray frame;

    // 帧头(1字节帧头)
    frame.append(static_cast<char>(FRAME_HEADER));

    // 长度字段（1字节长度，1字节长度反码）
    const int payloadSize = payload.size();
    const int totalLength = MIN_FRAME_SIZE + payloadSize;
    frame.append(static_cast<char>(totalLength));
    frame.append(static_cast<char>(~totalLength)); // 长度反码

    // 地址和操作码
    frame.append(static_cast<char>(destAddr));
    frame.append(static_cast<char>(srcAddr));
    frame.append(static_cast<char>(opCode));

    // 命令码（2字节操作指令）
    frame.append(static_cast<char>((command >> 8) & 0xFF)); // 高字节
    frame.append(static_cast<char>(command & 0xFF));        // 低字节

    // 负载数据（n字节参数负载）
    if (!payload.isEmpty()) {
        frame.append(payload);
    }

    // 计算校验和 (从destAddr开始到payload结束)
    uint8_t checksum = calculateChecksum(frame, POS_DEST_ADDR);
    frame.append(static_cast<char>(checksum));

    // 帧尾
    frame.append(static_cast<char>(FRAME_FOOTER));

    return frame;
}

// ======== 接收指令功能 ========
// 从缓冲区提取完整帧
QList<QByteArray> ProtocolProcessor::extractFramesFromBuffer(QByteArray &buffer)
{
    QList<QByteArray> frames;

    while (true) {
        // 查找帧头0x55的位置
        int headerPos = buffer.indexOf(static_cast<char>(FRAME_HEADER));

        // 如果未找到帧头，或剩余数据不足以包含长度字段，退出循环
        if (headerPos == -1 || buffer.size() - headerPos < 3) break;

        // 从帧头开始截取数据，丢弃帧头之前的无效数据
        buffer = buffer.mid(headerPos);

        // 读取长度字段和长度反码
        uint8_t len = static_cast<uint8_t>(buffer[POS_LENGTH]);
        uint8_t complement = static_cast<uint8_t>(buffer[POS_LEN_COMPLEMENT]);

        // 校验长度字段是否正确（长度 + 反码 == 0xFF）
        if ((len + complement) != 0xFF) {
            QByteArray dropped = buffer.left(3);
             LOG_WARNING(QString("长度校验错误，移除错误数据: %1").arg(dropped.toHex(' ').toUpper()));
            buffer.remove(0, 1); // 移除当前帧头，继续查找下一个帧头
            continue;
        }

        // 如果当前缓存不足以构成完整帧，退出等待下一次数据补充
        if (buffer.size() < len) break;

        // 提取完整帧（从帧头到帧尾）
        QByteArray frame = buffer.left(len);
        frames.append(frame);

        LOG_RX(QString("RX Raw: %1").arg(frame.toHex(' ').toUpper()));

        // 从缓存中移除已处理的帧
        buffer.remove(0, len);
    }
    // 返回所有成功提取的完整帧
    return frames;
}

// 校验帧函数
bool ProtocolProcessor::validateFrame(const QByteArray &frame)
{
    // 基本长度检查
    if (frame.size() < MIN_FRAME_SIZE) {
        return false;
    }

    // 头尾检查
    if (static_cast<uint8_t>(frame[POS_HEADER]) != FRAME_HEADER ||
        static_cast<uint8_t>(frame[frame.size()-1]) != FRAME_FOOTER) {
        return false;
    }

    // 长度验证
    uint8_t len = static_cast<uint8_t>(frame[POS_LENGTH]);
    uint8_t lenComplement = static_cast<uint8_t>(frame[POS_LEN_COMPLEMENT]);
    if ((len + lenComplement) != 0xFF) {
        return false;
    }

    // 实际长度验证
    if (frame.size() < len) {
        return false;
    }

    // 计算校验和
    int checksumStart = POS_DEST_ADDR;
    int checksumEnd = len - 2; // 减去校验和和帧尾
    uint8_t calculatedChecksum = calculateChecksum(frame, checksumStart, checksumEnd - checksumStart);
    uint8_t expectedChecksum = static_cast<uint8_t>(frame[len - 2]);

    return calculatedChecksum == expectedChecksum;
}

// 计算校验和
uint8_t ProtocolProcessor::calculateChecksum(const QByteArray &data, int start, int length)
{
    if (start < 0 || start >= data.size()) {
        return 0;
    }

    int end = (length == -1) ? data.size() : (start + length);
    if (end > data.size()) {
        end = data.size();
    }

    uint8_t checksum = 0;
    for (int i = start; i < end; ++i) {
        checksum ^= static_cast<uint8_t>(data[i]);
    }
    return checksum;
}

// 解析帧函数
bool ProtocolProcessor::parseFrame(const QByteArray &frame, ParsedFrameData &parsedData)
{
    if (!validateFrame(frame)) {
        // LOG_ERROR(QString("帧校验失败: %1").arg(frame.toHex(' ').toUpper()));     // 注释掉，原因是任何调用解析帧函数，出现验证错误都会记录日志
        return false;
    }

    parsedData.destAddr = static_cast<uint8_t>(frame[POS_DEST_ADDR]);
    parsedData.srcAddr = static_cast<uint8_t>(frame[POS_SRC_ADDR]);
    parsedData.opCode = static_cast<uint8_t>(frame[POS_OPCODE]);
    parsedData.command = (static_cast<uint8_t>(frame[POS_COMMAND_HIGH_]) << 8) |
                         static_cast<uint8_t>(frame[POS_COMMAND_LOW]);

    // 确定帧类型
    switch (parsedData.opCode) {
    case 0x01: parsedData.frameType = Request; break;       // 请求（前端→主机）
    case 0x02: parsedData.frameType = Notification; break;  // 通知（主机主动上报）
    case 0x04: parsedData.frameType = Response; break;      // 响应（主机回应请求）
    default: parsedData.frameType = Unknown;
    }

    // 提取负载数据
    int totalLength = static_cast<uint8_t>(frame[POS_LENGTH]);
    int payloadLength = totalLength - MIN_FRAME_SIZE;
    if (payloadLength > 0) {
        parsedData.payload = frame.mid(POS_PAYLOAD_START, payloadLength);
    } else {
        parsedData.payload.clear();
    }

    bool toDebug = true;   // 是否调试解析校验结果

    if (toDebug) {
        return true;
    } else {

        // 🔥 ========== 详细打印所有解析的帧元素 ==========
        qDebug().noquote() << QString("===== ProtocolProcessor: 帧解析详情 =====");

        // 1. 原始帧数据
        qDebug().noquote() << QString("原始帧 HEX : %1").arg(frame.toHex(' ').toUpper());
        qDebug().noquote() << QString("帧总长度   : %1 字节").arg(frame.size());
        // 2. 帧头帧尾
        qDebug().noquote() << QString("帧头       : 0x%1").arg(static_cast<uint8_t>(frame[POS_HEADER]), 2, 16, QChar('0')).toUpper();
        qDebug().noquote() << QString("帧尾       : 0x%1").arg(static_cast<uint8_t>(frame[frame.size()-1]), 2, 16, QChar('0')).toUpper();
        // 3. 长度字段
        uint8_t len = static_cast<uint8_t>(frame[POS_LENGTH]);
        uint8_t complement = static_cast<uint8_t>(frame[POS_LEN_COMPLEMENT]);
        qDebug().noquote() << QString("长度字段   : 0x%1 (%2 字节)").arg(len, 2, 16, QChar('0')).toUpper().arg(len);
        qDebug().noquote() << QString("长度反码   : 0x%1").arg(complement, 2, 16, QChar('0')).toUpper();
        qDebug().noquote() << QString("长度校验   : %1 (0x%2 + 0x%3 = 0xFF)").arg((len + complement) == 0xFF ? "正确" : "错误").arg(len, 2, 16, QChar('0')).toUpper().arg(complement, 2, 16, QChar('0')).toUpper();
        // 4. 地址字段
        qDebug().noquote() << QString("目标地址   : 0x%1").arg(parsedData.destAddr, 2, 16, QChar('0')).toUpper();
        qDebug().noquote() << QString("源地址     : 0x%1").arg(parsedData.srcAddr, 2, 16, QChar('0')).toUpper();
        // 5. 操作码和帧类型
        qDebug().noquote() << QString("操作码     : 0x%1 (%2)").arg(parsedData.opCode, 2, 16, QChar('0')).toUpper().arg(frameTypeToString(parsedData.frameType));
        // 6. 命令码
        qDebug().noquote() << QString("命令码     : 0x%1 (%2)").arg(parsedData.command, 4, 16, QChar('0')).toUpper().arg(parsedData.command);
        // 7. 负载数据
        if (payloadLength > 0) {
            qDebug().noquote() << QString("负载长度   : %1 字节").arg(payloadLength);
            qDebug().noquote() << QString("负载 HEX   : %1").arg(parsedData.payload.toHex(' ').toUpper());
            // 尝试解析负载中的具体参数（如果有的话）
            if (payloadLength >= 1) {
                qDebug().noquote() << QString("负载字节 [0]: 0x%1").arg(static_cast<uint8_t>(parsedData.payload[0]), 2, 16, QChar('0')).toUpper();
            }
            if (payloadLength >= 2) {
                qDebug().noquote() << QString("负载字节 [1]: 0x%1").arg(static_cast<uint8_t>(parsedData.payload[1]), 2, 16, QChar('0')).toUpper();
            }
            if (payloadLength > 2) {
                qDebug().noquote() << QString("剩余负载   : %1 字节").arg(payloadLength - 2);
            }
        } else {
            qDebug().noquote() << QString("负载数据   : 无");
        }
        // 8. 校验和
        uint8_t checksum = static_cast<uint8_t>(frame[len - 2]);
        uint8_t calculatedChecksum = calculateChecksum(frame, POS_DEST_ADDR, len - 2 - POS_DEST_ADDR);
        qDebug().noquote() << QString("接收校验和 : 0x%1").arg(checksum, 2, 16, QChar('0')).toUpper();
        qDebug().noquote() << QString("计算校验和 : 0x%1 (%2)").arg(calculatedChecksum, 2, 16, QChar('0')).toUpper().arg(checksum == calculatedChecksum ? "匹配" : "不匹配");

        qDebug().noquote() << QString("========================================");

        return true;
    }
}

// 匹配响应命令与请求命令
bool ProtocolProcessor::isMatchingResponse(const ParsedFrameData &requestFrame, const ParsedFrameData &responseFrame)
{
    // 匹配规则检查：
    // 1. 地址互换（发送帧的目标地址 = 响应帧的源地址）
    // 2. 地址互换（发送帧的源地址 = 响应帧的目标地址）
    // 3. 操作类型转换（请求帧0x01 → 响应帧0x04）
    // 4. 命令码完全一致
    // 地址互换匹配
    bool addressMatch = (requestFrame.destAddr == responseFrame.srcAddr) && (requestFrame.srcAddr == responseFrame.destAddr);

    // 操作类型转换匹配（请求 -> 响应）
    bool opcodeMatch = (requestFrame.opCode == 0x01) && (responseFrame.opCode == 0x04);

    // 命令码匹配
    bool commandMatch = (requestFrame.command == responseFrame.command);

    return addressMatch && opcodeMatch && commandMatch;
}

// 获取帧类型描述
QString ProtocolProcessor::frameTypeToString(FrameType type)
{
    switch (type) {
    case Request: return "Request";
    case Response: return "Response";
    case Notification: return "Notification";
    default: return "Unknown";
    }
}


// ==================== 实例方法：请求响应管理（新增功能）====================
// 设置响应超时时间
void ProtocolProcessor::setResponseTimeout(int timeoutMs)
{
    m_responseTimeoutMs = qMax(100, timeoutMs);
}

// 设置响应重发次数
void ProtocolProcessor::setMaxRetries(int maxRetries)
{
    m_maxRetries = qMax(0, maxRetries);
}

// 设置队列内延时（发送指令间隔）
void ProtocolProcessor::setInterCommandDelay(int delayMs)
{
    m_interCommandDelayMs = qMax(0, delayMs);
}

// 发送单个指令
void ProtocolProcessor::sendCommand(const CommandConfig &config,
                                     int maxRetries,
                                     const QVariant &userData)
{
    // 构建完整帧
    QByteArray frame = buildFrame(
        config.destAddr,
        config.srcAddr,
        config.opCode,
        config.command,
        config.payload
    );
    
    if (frame.isEmpty()) {
        LOG_ERROR("构建指令帧失败");
        return;
    }
    
    // 创建请求项
    PendingRequest request;
    request.commandConfig = config;
    request.rawFrame = frame;
    request.matchRule = MatchRule::fromCommandConfig(config);
    request.maxRetries = (maxRetries >= 0) ? maxRetries : m_maxRetries;
    request.userData = userData;
    
    m_requestQueue.enqueue(request);
    
    if (m_state == CommState::Idle) {
        processNextRequest();
    }
}

// 发送批量指令（所有指令一次性加入队列，按顺序逐个发送和等待响应）
void ProtocolProcessor::sendCommandQueue(const QVector<PendingRequest> &requests)
{
    // 将所有请求加入队列
    for (const auto& req : requests) {
        m_requestQueue.enqueue(req);
    }
    
    // 如果空闲，开始处理第一个请求
    if (m_state == CommState::Idle) {
        processNextRequest();
    }
}

// 清理队列成员变量
void ProtocolProcessor::clearQueue()
{
    m_requestQueue.clear();
    m_responseTimer->stop();
    switchState(CommState::Idle);
}

// 按 userData 前缀清除请求（精细化清除,）
int ProtocolProcessor::clearCommandsByUserDataPrefix(const QString& prefix)
{
    if (prefix.isEmpty()) {
        LOG_WARNING("clearCommandsByUserDataPrefix: 前缀为空，跳过清除");
        return 0;
    }

    int removedCount = 0;
    QQueue<PendingRequest> newQueue;

    // 遍历当前队列，分离需要清除和保留的请求
    while (!m_requestQueue.isEmpty()) {
        PendingRequest req = m_requestQueue.dequeue();

        // 检查 userData 是否以指定前缀开头
        QString userDataStr = req.userData.toString();
        if (userDataStr.startsWith(prefix)) {
            removedCount++;
            LOG_INFO(QString("清除队列中的请求 - UserData前缀: %1, Command: 0x%2")
                         .arg(prefix)
                         .arg(req.commandConfig.command, 4, 16, QChar('0')));
        } else {
            newQueue.enqueue(req);  // 保留其他请求
        }
    }

    // 更新队列
    m_requestQueue = newQueue;

    // 如果队列为空，重置状态
    if (m_requestQueue.isEmpty()) {
        m_responseTimer->stop();
        switchState(CommState::Idle);
    } else if (m_state != CommState::Idle) {
        // 继续处理队列中的下一个请求
        processNextRequest();
    }

    LOG_INFO(QString("按前缀清除完成 - 前缀: %1, 清除数量: %2, 剩余数量: %3")
                 .arg(prefix).arg(removedCount).arg(m_requestQueue.size()));

    return removedCount;
}

// 用于处理非 request-response 模式的指令（主机主动上报的数据）
void ProtocolProcessor::registerNotificationHandler(uint8_t opCode, uint16_t command, 
                                                     NotificationHandler callback)
{
    // 创建一个唯一的 key：opCode << 16 | command
    quint32 key = (static_cast<quint32>(opCode) << 16) | command;
    // 将回调函数存储到 QMap 中
    m_notificationHandlers[key] = callback;
}

// 移除之前注册的通知处理器（避免内存泄漏或重复处理）
void ProtocolProcessor::unregisterNotificationHandler(uint8_t opCode, uint16_t command)
{
    // 使用相同的算法计算 key
    quint32 key = (static_cast<quint32>(opCode) << 16) | command;
    // 从 QMap 中移除
    m_notificationHandlers.remove(key);
}

// 注入实际的硬件发送逻辑（让 ProtocolProcessor 知道如何发送数据）
void ProtocolProcessor::setSendFunction(SendFunction func)
{
    // 保存发送函数的 lambda 表达式
    m_sendFunction = func;

    // 关键点，需要这个lambda来发送数据
    //     1. m_sendFunction 是一个函数指针/lambda
    //        - 在 setupProtocolProcessor() 时注入
    //        - 捕获了 this 指针（指向 XdcController）
    //     2. ProtocolProcessor 不直接知道 HostManager
    //        - 它只知道调用 m_sendFunction(data)
    //        - 具体怎么发送，由注入的 lambda 决定
    //     3. 这是依赖注入模式
    //        - ProtocolProcessor 不依赖具体的发送实现
    //        - 可以灵活替换发送方式（串口、TCP、UDP 等）
    //     4. 数据流仍然经过 Controller
    //        - ProtocolProcessor → lambda → XdcController::sendToHost() → HostManager
    //        - 但这是通过函数调用，不是通过对象引用
}

// 核心匹配引擎：判断收到的响应帧是哪个请求的响应，并分发处理
/** 对于控制器会收到两类指令，其一是主机主动上报通知（opCode=0x02）是前端请求-主机响应*/
bool ProtocolProcessor::handleIncomingFrame(const QByteArray &rawFrame)
{
    // 解析帧
    ParsedFrameData parsedData;
    if (!parseFrame(rawFrame, parsedData)) {
        LOG_WARNING("解析帧失败，丢弃该帧");
        return false;
    }

    // 第一类指令：opCode=0x02 的主动上报通知
    if (parsedData.opCode == 0x02) {
        LOG_RX(QString("收到主机主动上报通知 - "
                       "OpCode: 0x%1, Command: 0x%2, SrcAddr: 0x%3, Payload: %4")
                  .arg(parsedData.opCode, 2, 16, QChar('0'))
                  .arg(parsedData.command, 4, 16, QChar('0'))
                  .arg(parsedData.srcAddr, 2, 16, QChar('0'))
                  .arg(parsedData.payload.toHex(' ').toUpper()));

        // 发射信号，由 Controller 处理业务逻辑
        emit unsolicitedNotifyReceived(rawFrame, parsedData);
        return true;  // 已处理，不再执行后续逻辑
    }
    
    // 第二类指令：等待响应，检查是否匹配当前请求
    // 情况 1：已经在等待或者重试的请求-响应匹配
    if (m_state == CommState::Waiting || m_state == CommState::Retrying) {
        // 关键：三元组精确匹配
        if (m_currentRequest.matchesResponse(parsedData)) {
            // 成功匹配Response （0x04）
            m_responseTimer->stop();

            LOG_RX(QString("收到匹配的响应 - opCode: 0x%1, CMD: 0x%2, PAYLOAD: 0x%3")
                      .arg(parsedData.opCode, 2, 16, QChar('0'))
                      .arg(parsedData.command, 4, 16, QChar('0'))
                      .arg(parsedData.payload.toHex(' ').toUpper()));
            
            // 构造响应结果
            ResponseResult result;
            result.success = true;
            result.commandConfig = m_currentRequest.commandConfig;
            result.requestFrame = m_currentRequest.rawFrame;
            result.responseFrame = rawFrame;
            result.parsedData = parsedData;
            result.pendingRequest = m_currentRequest;
            
            // 发射信号：通知 Controller 响应已完成
            emit responseCompleted(result);
            
            // 切换到空闲状态
            switchState(CommState::Idle);
            
            QTimer::singleShot(m_interCommandDelayMs, this, [this]() {
                processNextRequest();
            });
            
            return true;    //匹配完成
        }
    }
    
    // 情况 2： 非等待和重试的请求-响应匹配
    quint32 key = (static_cast<quint32>(parsedData.opCode) << 16) | parsedData.command;
    if (m_notificationHandlers.contains(key)) {
        // 找到注册的通知处理器，（opCode + command）匹配，调用回调
        auto handler = m_notificationHandlers.value(key);
        if (handler) {
            handler(rawFrame, parsedData);  // 调用回调
        }
        return true;
    }

    return false;   // 都不匹配
}


// ==================== 私有槽函数实现 ====================

void ProtocolProcessor::processNextRequest()
{
    if (m_state != CommState::Idle) {
        return;
    }
    
    if (m_requestQueue.isEmpty()) {
        emit queueProcessed();
        return;
    }

    // 取出队列头部的请求
    m_currentRequest = m_requestQueue.dequeue();
    m_retryCount = 0;
    
    if (!sendCurrentRequest()) {
        LOG_ERROR("发送指令失败");
        switchState(CommState::Error);

        // 🔥 失败时也清空当前请求
        m_currentRequest = PendingRequest();

        processNextRequest();
        return;
    }
    
    switchState(CommState::Waiting);
    m_responseTimer->start(m_responseTimeoutMs);
}

bool ProtocolProcessor::sendCurrentRequest()
{
    if (!m_sendFunction) {
        LOG_ERROR("发送函数未设置");
        return false;
    }
    
    m_currentRequest.sendTime = QDateTime::currentDateTime();
    m_currentRequest.isWaitingResponse = true;

    // 记录发送的原始数据
    LOG_TX(QString("TX Raw: %1").arg(m_currentRequest.rawFrame.toHex(' ').toUpper()));
    
    m_sendFunction(m_currentRequest.rawFrame);
    
    emit requestSent(m_currentRequest);
    
    return true;
}

void ProtocolProcessor::onResponseTimeout()
{
    // 防御性检查，如果指令队列为空，停止超时定时器
    if (m_currentRequest.commandConfig.command == 0) {
        LOG_WARNING("onResponseTimeout: 当前请求已清空，停止定时器");
        m_responseTimer->stop();
        return;
    }

    m_retryCount++;
    
    if (m_retryCount <= m_currentRequest.maxRetries) {
        switchState(CommState::Retrying);
        
        int backoffDelay = calculateBackoffDelay(m_retryCount);
        
        LOG_WARNING(QString("第%1次重试，延迟%2ms").arg(m_retryCount).arg(backoffDelay));
        
        // 复用同一个定时器，设置新的超时时间
        m_responseTimer->start(backoffDelay);
        // 延迟 backoffDelay 毫秒后，会自动触发 onResponseTimeout()
    } else {
        // 超过最大重试次数，要跳过当前失败指令，执行下一个指令
        LOG_ERROR(QString("指令执行失败 - 超过最大重试次数 (Cmd: 0x%1)").arg(m_currentRequest.commandConfig.command, 4, 16, QChar('0')).toUpper());
        
        
        ResponseResult result;
        result.success = false;
        result.commandConfig = m_currentRequest.commandConfig;
        result.requestFrame = m_currentRequest.rawFrame;
        result.errorMessage = "响应超时";
        result.pendingRequest = m_currentRequest;
        
        emit requestFailed(m_currentRequest, "响应超时");

        // 停止定时器，跳入“执行下一指令”前停止当前定时器，并设置状态为空闲
        m_responseTimer->stop();
        switchState(CommState::Idle);

        // 重要：清空当前请求，避免重复处理
        m_currentRequest = PendingRequest();

        // 开始执行下一指令
        processNextRequest();
    }
}

void ProtocolProcessor::resendCurrentRequest()
{
    // 防御性检查
    if (m_currentRequest.commandConfig.command == 0) {
        LOG_WARNING("resendCurrentRequest: 当前请求为空，取消重发");
        return;
    }

    if (sendCurrentRequest()) {
        // 发送成功，启动超时定时器等待响应
        switchState(CommState::Waiting);
        m_responseTimer->start(m_responseTimeoutMs);
    } else {
        // 🔥 发送失败
        m_retryCount++;  // ← 增加重试计数

        LOG_WARNING(QString("发送失败 - 已重试次数: %1/%2")
                        .arg(m_retryCount)
                        .arg(m_currentRequest.maxRetries));

        // 检查是否超过最大重试
        if (m_retryCount > m_currentRequest.maxRetries) {
            // 超过最大重试，立即处理失败
            onResponseTimeout();
        } else {
            // 还可以继续重试，计算退避延迟
            int backoffDelay = calculateBackoffDelay(m_retryCount);

            LOG_WARNING(QString("第%1次重试，延迟%2ms后再次尝试发送")
                            .arg(m_retryCount).arg(backoffDelay));

            // 延迟后再次调用 onResponseTimeout() → resendCurrentRequest()
            m_responseTimer->start(backoffDelay);
        }
    }
}

void ProtocolProcessor::switchState(CommState newState)
{
    CommState oldState = m_state;
    m_state = newState;
    emit stateChanged(newState, oldState);
}

int ProtocolProcessor::calculateBackoffDelay(int retryCount)
{
    return m_responseTimeoutMs * (1 << retryCount);
}
