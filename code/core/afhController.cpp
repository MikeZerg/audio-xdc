// DS240 跳频系统主机控制器 - 实现文件
#include "AfhController.h"
#include "HostManager.h"
#include <QDebug>
#include <QDateTime>
#include <QtMath>

// ==================== 构造函数/析构函数 ====================

AfhController::AfhController(uint8_t hostAddress, HostManager* parent)
    : QObject(parent)
    , m_hostManager(parent)
    , m_protocolProcessor(new ProtocolProcessor(this))
{
    // 设置主机地址
    m_hostInfo.address = hostAddress;

    // 创建定时器
    m_statusPollTimer = new QTimer(this);
    m_scanTimer = new QTimer(this);
    m_initStepTimer = new QTimer(this);

    // 连接定时器信号
    connect(m_statusPollTimer, &QTimer::timeout, this, &AfhController::onStatusPollTimer);
    connect(m_scanTimer, &QTimer::timeout, this, &AfhController::onScanTimer);
    connect(m_initStepTimer, &QTimer::timeout, this, &AfhController::onInitStepTimer);

    // 初始化协议处理器
    setupProtocolProcessor();

    qDebug().noquote() << QString("AfhController: 构造完成，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));
}

AfhController::~AfhController()
{
    // 停止所有定时器
    if (m_statusPollTimer) {
        m_statusPollTimer->stop();
    }
    if (m_scanTimer) {
        m_scanTimer->stop();
    }
    if (m_initStepTimer) {
        m_initStepTimer->stop();
    }

    // 停止状态轮询
    m_isStatusPolling = false;
    m_scanCache.isScanning = false;

    qDebug().noquote() << QString("AfhController: 析构完成，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));
}


// ==================== 辅助函数 ====================

QString AfhController::formatAddressHex(uint8_t addr) const
{
    // 格式化为 "0x01" 形式
    return QString::asprintf("0x%02X", addr);
}

QString AfhController::addressHex() const
{
    return formatAddressHex(m_hostInfo.address);
}

uint8_t AfhController::address() const
{
    return m_hostInfo.address;
}

bool AfhController::isInitialized() const
{
    return m_hostInfo.isValid() && m_initStep >= 6;
}

bool AfhController::isScanning() const
{
    return m_scanCache.isScanning;
}

int AfhController::initProgress() const
{
    return m_initProgress;
}

QString AfhController::trimNullTerminator(const QByteArray& data) const
{
    // 去除末尾的 '\0' 字符
    QByteArray cleaned = data;
    while (!cleaned.isEmpty() && cleaned.endsWith('\0')) {
        cleaned.chop(1);
    }
    return QString::fromUtf8(cleaned);
}

quint32 AfhController::parseFreqFromPayload(const QByteArray& payload, int offset) const
{
    // 从负载中解析 4 字节大端序频率值（单位：KHz）
    if (payload.size() < offset + 4) {
        return 0;
    }
    return (static_cast<quint32>(static_cast<uint8_t>(payload[offset + 0])) << 24) |
           (static_cast<quint32>(static_cast<uint8_t>(payload[offset + 1])) << 16) |
           (static_cast<quint32>(static_cast<uint8_t>(payload[offset + 2])) << 8) |
           (static_cast<quint32>(static_cast<uint8_t>(payload[offset + 3])));
}

int8_t AfhController::parseRssiFromPayload(const QByteArray& payload, int offset) const
{
    // 从负载中解析 1 字节有符号 RSSI 值（单位：dBm）
    if (payload.size() < offset + 1) {
        return -127;
    }
    return static_cast<int8_t>(payload[offset]);
}


// ==================== 协议处理器初始化 ====================

void AfhController::setupProtocolProcessor()
{
    // 1. 设置发送函数（注入硬件发送能力）
    m_protocolProcessor->setSendFunction([this](const QByteArray& data) {
        sendToHost(data);
    });

    // 2. 配置通信参数
    m_protocolProcessor->setResponseTimeout(500);   // 500ms 超时
    m_protocolProcessor->setMaxRetries(3);          // 最多重试 3 次
    m_protocolProcessor->setInterCommandDelay(50);  // 指令间隔 50ms

    // 3. 连接信号槽
    connect(m_protocolProcessor, &ProtocolProcessor::responseCompleted,
            this, &AfhController::onCommandResponseCompleted);
    connect(m_protocolProcessor, &ProtocolProcessor::requestFailed,
            this, &AfhController::onCommandFailed);
    connect(m_protocolProcessor, &ProtocolProcessor::unsolicitedNotifyReceived,
            this, &AfhController::onUnsolicitedNotifyReceived);

    qDebug().noquote() << QString("AfhController: 协议处理器初始化完成，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));
}


// ==================== 发送指令 ====================

void AfhController::sendToHost(const QByteArray& data)
{
    // 通过 HostManager 发送原始数据
    if (m_hostManager) {
        m_hostManager->sendRawData(data);
    } else {
        qWarning().noquote() << QString("AfhController: HostManager 为空，无法发送数据");
    }
}

void AfhController::sendCommand(uint8_t opCode, uint16_t command,
                                const QByteArray& payload,
                                const QVariant& userData)
{
    // 构建指令配置
    ProtocolProcessor::CommandConfig config(
        m_hostInfo.address,     // 目标地址（主机地址）
        MASTER_ADDR,            // 源地址（中控地址 0x80）
        opCode,                 // 操作码（0x01=请求）
        command,                // 命令码
        payload                 // 负载数据
        );

    // 通过协议处理器发送
    m_protocolProcessor->sendCommand(config, -1, userData);
}

void AfhController::sendCommandQueue(const QVariantList& commands)
{
    // 构建请求列表
    QVector<ProtocolProcessor::PendingRequest> requests;

    for (const auto& cmdVariant : commands) {
        QVariantMap cmd = cmdVariant.toMap();

        uint8_t opCode = cmd["opCode"].toUInt();
        uint16_t command = cmd["command"].toUInt();
        QByteArray payload = cmd["payload"].toByteArray();
        QString userData = cmd["userData"].toString();

        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostInfo.address, MASTER_ADDR, opCode, command, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;
        req.userData = QVariant(userData);

        requests.append(req);
    }

    m_protocolProcessor->sendCommandQueue(requests);
}

QVariantList AfhController::buildCommandQueue(const QVariantList& commands)
{
    // 构建批量获取指令队列（供初始化使用）
    QVariantList queue;

    for (const auto& cmdVariant : commands) {
        QVariantMap cmd = cmdVariant.toMap();

        if (!cmd.contains("command")) {
            qWarning() << "AfhController: buildCommandQueue 命令缺少 command 字段";
            continue;
        }

        uint16_t command = cmd["command"].toUInt();
        QByteArray payload = cmd.value("payload").toByteArray();
        QString operation = cmd.value("operation",
                                      QString("get_0x%1").arg(command, 4, 16, QChar('0'))).toString();

        QVariantMap request;
        request["opCode"] = static_cast<uint8_t>(0x01);  // 请求操作码
        request["command"] = command;
        request["payload"] = payload;
        request["userData"] = operation;

        queue.append(request);
    }

    return queue;
}


// ==================== 接收指令分发 ====================

void AfhController::handleIncomingFrame(const ProtocolProcessor::ParsedFrameData& frame)
{
    // 由 HostManager 调用，将接收到的帧交给 ProtocolProcessor 处理
    qDebug().noquote() << QString("AfhController: 收到 HostManager 转发数据，地址：%1，CMD：0x%2")
                              .arg(formatAddressHex(m_hostInfo.address))
                              .arg(frame.command, 4, 16, QChar('0'));

    // 构建原始帧交给 ProtocolProcessor 处理
    QByteArray rawFrame = ProtocolProcessor::buildFrame(
        frame.destAddr,
        frame.srcAddr,
        frame.opCode,
        frame.command,
        frame.payload
        );

    m_protocolProcessor->handleIncomingFrame(rawFrame);
}


// ==================== 协议处理器回调 ====================

void AfhController::onCommandResponseCompleted(const ProtocolProcessor::ResponseResult& result)
{
    if (!result.success) {
        qWarning().noquote() << QString("AfhController: 响应失败：%1").arg(result.errorMessage);
        return;
    }

    // 1. 解析 userData，提取批次ID和操作名
    QString userDataStr = result.pendingRequest.userData.toString();
    QString batchId;
    QString operation;

    if (userDataStr.contains('|')) {
        QStringList parts = userDataStr.split('|');
        batchId = parts[0];
        operation = parts[1];
    } else {
        operation = userDataStr;
    }

    qDebug().noquote() << QString("AfhController: 收到响应 - 地址：%1, CMD: 0x%2, 批次：%3")
                              .arg(formatAddressHex(m_hostInfo.address))
                              .arg(result.parsedData.command, 4, 16, QChar('0'))
                              .arg(batchId.isEmpty() ? "无" : batchId);

    // 2. 处理批次计数（核销）
    if (!batchId.isEmpty() && m_activeBatches.contains(batchId)) {
        auto& ctx = m_activeBatches[batchId];
        ctx.received++;

        qDebug().noquote() << QString("AfhController: 批次进度 %1/%2")
                                  .arg(ctx.received).arg(ctx.expected);

        // 检查是否完成
        if (ctx.received >= ctx.expected && !ctx.completed) {
            ctx.completed = true;

            qDebug().noquote() << QString("AfhController: 批次完成！批次ID：%1，操作：%2")
                                      .arg(batchId).arg(ctx.operation);

            // 根据操作类型发射对应信号
            if (ctx.operation == "fetchHostHardwareInfo") {
                emit hostInfoChanged();
            } else if (ctx.operation == "fetchApInfo") {
                emit apInfoChanged();
            } else if (ctx.operation == "fetchSystemConfig") {
                emit systemConfigChanged();
            } else if (ctx.operation == "loadPresetFreqPool" || ctx.operation == "loadCustomFreqPool") {
                emit freqPoolChanged();
            } else if (ctx.operation == "fetchAllPortConfig") {
                emit portRangeChanged();
                emit portFreqChanged();
            }

            notifyStateSnapshot();
            m_activeBatches.remove(batchId);
        }
    }

    // 3. 解析具体数据并更新成员变量
    parseAndDispatch(result.parsedData);
}

void AfhController::onCommandFailed(const ProtocolProcessor::PendingRequest& request,
                                    const QString& error)
{
    qWarning().noquote() << QString("AfhController: 指令执行失败：%1，CMD：0x%2")
                                .arg(error)
                                .arg(request.commandConfig.command, 4, 16, QChar('0'));

    emit commandFailed(request.commandConfig.command, error);

    // 解析操作名
    QString userDataStr = request.userData.toString();
    QString operation;
    QString batchId;

    if (userDataStr.contains('|')) {
        QStringList parts = userDataStr.split('|');
        batchId = parts[0];
        operation = parts[1];
    } else {
        operation = userDataStr;
    }

    emit operationResult(m_hostInfo.address, operation, false, error);

    // 如果失败请求属于某个批次，也需要计数（失败也算完成，避免永久等待）
    if (!batchId.isEmpty() && m_activeBatches.contains(batchId)) {
        auto& ctx = m_activeBatches[batchId];
        ctx.received++;

        qDebug().noquote() << QString("AfhController: 批次中请求失败，进度 %1/%2")
                                  .arg(ctx.received).arg(ctx.expected);

        if (ctx.received >= ctx.expected && !ctx.completed) {
            ctx.completed = true;
            qWarning().noquote() << QString("AfhController: 批次完成（含失败），批次ID：%1")
                                        .arg(batchId);
            m_activeBatches.remove(batchId);
        }
    }
}

void AfhController::onUnsolicitedNotifyReceived(const QByteArray& rawFrame,
                                                const ProtocolProcessor::ParsedFrameData& parsedData)
{
    qDebug().noquote() << QString("AfhController: 收到主动上报 - 地址：%1, CMD: 0x%2")
                              .arg(formatAddressHex(m_hostInfo.address))
                              .arg(parsedData.command, 4, 16, QChar('0'));

    // 直接复用 parseAndDispatch，与 Response 处理逻辑完全一致
    parseAndDispatch(parsedData);
}


// ==================== 响应解析与分发 ====================

void AfhController::parseAndDispatch(const ProtocolProcessor::ParsedFrameData& frame)
{
    // 根据命令码分发到对应的处理函数
    switch (frame.command) {
    // ========== 主机硬件信息 ==========
    case Command::Host::GET_VERSION:
        handleVersionResponse(frame.payload);
        break;
    case Command::Host::GET_BUILD_DATE:
        handleBuildDateResponse(frame.payload);
        break;
    case Command::Host::GET_PORT_COUNT:
        handlePortCountResponse(frame.payload);
        break;
    case Command::Host::GET_AP_COUNT:
        handleApCountResponse(frame.payload);
        break;

    // ========== 天线盒信息 ==========
    case Command::Ap::GET_STATUS:
        handleApStatusResponse(frame.payload);
        break;
    case Command::Ap::GET_TYPE:
        handleApTypeResponse(frame.payload);
        break;
    case Command::Ap::GET_VERSION:
        handleApVersionResponse(frame.payload);
        break;
    case Command::Ap::GET_DATE:
        handleApDateResponse(frame.payload);
        break;
    case Command::Ap::GET_CONFIG:
        handleApConfigResponse(frame.payload);
        break;

    // ========== 系统配置 ==========
    case Command::Config::GET_WORK_MODE:
        handleWorkModeResponse(frame.payload);
        break;
    case Command::Config::GET_FREQ_POOL:
        handleFreqPoolResponse(frame.payload);
        break;
    case Command::Config::GET_SENSITIVITY:
        handleSensitivityResponse(frame.payload);
        break;
    case Command::Config::GET_VOLUME:
        handleVolumeResponse(frame.payload);
        break;

    // ========== 频点池 ==========
    case Command::FreqPool::GET_PRESET_NS:
        handlePresetNsResponse(frame.payload);
        break;
    case Command::FreqPool::GET_PRESET_FREQ:
        handlePresetFreqResponse(frame.payload);
        break;
    case Command::FreqPool::GET_CUSTOM_NS:
        handleCustomNsResponse(frame.payload);
        break;
    case Command::FreqPool::GET_CUSTOM_FREQ:
        handleCustomFreqResponse(frame.payload);
        break;

    // ========== 端口配置 ==========
    case Command::PortConfig::GET_FREQ_ID:
        handlePortFreqResponse(frame.payload);
        break;
    case Command::PortConfig::GET_RANGE:
        handlePortRangeResponse(frame.payload);
        break;

    // ========== 端口状态 ==========
    case Command::PortStatus::GET_FREQ:
        handlePortFreqInfoResponse(frame.payload);
        break;
    case Command::PortStatus::GET_USERDATA:
        handlePortUserDataResponse(frame.payload);
        break;
    case Command::PortStatus::GET_PHASELOCK:
        handlePortPhaseLockResponse(frame.payload);
        break;
    case Command::PortStatus::GET_RSSI:
        handlePortRssiResponse(frame.payload);
        break;
    case Command::PortStatus::GET_AUDIO_RSSI:
        handlePortAudioRssiResponse(frame.payload);
        break;

    // ========== 统计 ==========
    case Command::Stats::GET_RF_STATS:
        handleRfStatsResponse(frame.payload);
        break;

    // ========== 设置命令响应 ==========
    case Command::Config::SET_VOLUME:
        handleSetResponse(frame, "setVolume");
        break;
    case Command::Ap::SET_CONFIG:
        handleSetResponse(frame, "setApConfigType");
        break;
    case Command::Config::SET_WORK_MODE:
        handleSetResponse(frame, "setWorkMode");
        break;
    case Command::Config::SET_FREQ_POOL:
        handleSetResponse(frame, "setFreqPool");
        break;
    case Command::Config::SET_SENSITIVITY:
        handleSetResponse(frame, "setSensitivity");
        break;
    case Command::FreqPool::SET_CUSTOM_NS:
        handleSetResponse(frame, "setCustomFreqCount");
        break;
    case Command::FreqPool::SET_CUSTOM_FREQ:
        handleSetResponse(frame, "setCustomFreq");
        break;
    case Command::PortConfig::SET_FREQ_ID:
        handleSetResponse(frame, "setPortFreq");
        break;
    case Command::PortConfig::SET_RANGE:
        handleSetResponse(frame, "setPortRange");
        break;

    default:
        qWarning().noquote() << QString("AfhController: 未知命令：0x%1")
                                    .arg(frame.command, 4, 16, QChar('0'));
        break;
    }
}

void AfhController::handleSetResponse(const ProtocolProcessor::ParsedFrameData& frame,
                                      const QString& operation)
{
    // 设置命令的响应，通常表示操作成功
    qDebug().noquote() << QString("AfhController: 设置命令响应，CMD：0x%1，操作：%2")
                              .arg(frame.command, 4, 16, QChar('0'))
                              .arg(operation);
    emit operationResult(m_hostInfo.address, operation, true, "操作成功");
}


// ==================== 响应处理函数 - 主机硬件信息 ====================

void AfhController::handleVersionResponse(const QByteArray& payload)
{
    // 0x0202 响应：版本号（ASCII 字符串）
    QString version = trimNullTerminator(payload);
    m_hostInfo.version = version;

    qDebug().noquote() << QString("AfhController: 主机版本：%1").arg(version);
}

void AfhController::handleBuildDateResponse(const QByteArray& payload)
{
    // 0x0203 响应：编译日期（ASCII 字符串）
    QString buildDate = trimNullTerminator(payload);
    m_hostInfo.buildDate = buildDate;

    qDebug().noquote() << QString("AfhController: 编译日期：%1").arg(buildDate);
}

void AfhController::handlePortCountResponse(const QByteArray& payload)
{
    // 0x0204 响应：端口数量（1 字节）
    if (payload.isEmpty()) return;

    uint8_t portCount = static_cast<uint8_t>(payload[0]);
    m_hostInfo.portCount = portCount;

    // 初始化端口相关容器
    m_portConfig.initPorts(portCount);
    m_portStatus.initPorts(portCount);

    qDebug().noquote() << QString("AfhController: 端口数量：%1").arg(portCount);
}

void AfhController::handleApCountResponse(const QByteArray& payload)
{
    // 0x0205 响应：天线盒数量（1 字节）
    if (payload.isEmpty()) return;

    uint8_t apCount = static_cast<uint8_t>(payload[0]);
    m_hostInfo.apCount = apCount;

    qDebug().noquote() << QString("AfhController: 天线盒数量：%1").arg(apCount);
}


// ==================== 响应处理函数 - 天线盒信息 ====================

void AfhController::handleApStatusResponse(const QByteArray& payload)
{
    // 0x0206 响应：apId(1) + status(1)
    if (payload.size() < 2) return;

    uint8_t apId = static_cast<uint8_t>(payload[0]);
    bool online = (payload[1] != 0);

    AfhApInfo info;
    info.apId = apId;
    info.online = online ? AfhApStatus::Online : AfhApStatus::Offline;
    m_apTable.updateApInfo(info);

    // 检查状态是否变化
    static QMap<uint8_t, bool> lastOnlineState;
    if (!lastOnlineState.contains(apId) || lastOnlineState[apId] != online) {
        lastOnlineState[apId] = online;
        emit apOnlineChanged(apId, online);
    }

    qDebug().noquote() << QString("AfhController: AP%1 状态：%2")
                              .arg(apId).arg(online ? "在线" : "离线");
}

void AfhController::handleApTypeResponse(const QByteArray& payload)
{
    // 0x0207 响应：apId(1) + type(1)
    if (payload.size() < 2) return;

    uint8_t apId = static_cast<uint8_t>(payload[0]);
    AfhApType actualType = static_cast<AfhApType>(payload[1]);

    AfhApInfo info;
    info.apId = apId;
    info.actualType = actualType;
    m_apTable.updateApInfo(info);

    qDebug().noquote() << QString("AfhController: AP%1 实际类型：%2")
                              .arg(apId).arg(AfhApInfo::apTypeToString(actualType));
}

void AfhController::handleApVersionResponse(const QByteArray& payload)
{
    // 0x0208 响应：apId(1) + version(ASCII)
    if (payload.size() < 2) return;

    uint8_t apId = static_cast<uint8_t>(payload[0]);
    QString version = trimNullTerminator(payload.mid(1));

    AfhApInfo info;
    info.apId = apId;
    info.version = version;
    m_apTable.updateApInfo(info);

    qDebug().noquote() << QString("AfhController: AP%1 版本：%2").arg(apId).arg(version);
}

void AfhController::handleApDateResponse(const QByteArray& payload)
{
    // 0x0209 响应：apId(1) + date(ASCII)
    if (payload.size() < 2) return;

    uint8_t apId = static_cast<uint8_t>(payload[0]);
    QString buildDate = trimNullTerminator(payload.mid(1));

    AfhApInfo info;
    info.apId = apId;
    info.buildDate = buildDate;
    m_apTable.updateApInfo(info);

    qDebug().noquote() << QString("AfhController: AP%1 编译日期：%2").arg(apId).arg(buildDate);
}

void AfhController::handleApConfigResponse(const QByteArray& payload)
{
    // 0x0212 响应：apId(1) + configType(1)
    if (payload.size() < 2) return;

    uint8_t apId = static_cast<uint8_t>(payload[0]);
    AfhApType configType = static_cast<AfhApType>(payload[1]);

    AfhApInfo info;
    info.apId = apId;
    info.configType = configType;
    m_apTable.updateApInfo(info);

    qDebug().noquote() << QString("AfhController: AP%1 配置类型：%2")
                              .arg(apId).arg(AfhApInfo::apTypeToString(configType));
}


// ==================== 响应处理函数 - 系统配置 ====================

void AfhController::handleWorkModeResponse(const QByteArray& payload)
{
    // 0x0213 响应：mode(1)
    if (payload.isEmpty()) return;

    AfhWorkMode mode = static_cast<AfhWorkMode>(payload[0]);
    m_sysConfig.workMode = mode;

    qDebug().noquote() << QString("AfhController: 工作模式：%1").arg(static_cast<int>(mode));
}

void AfhController::handleFreqPoolResponse(const QByteArray& payload)
{
    // 0x0214 响应：pool(1)
    if (payload.isEmpty()) return;

    AfhFreqPool pool = static_cast<AfhFreqPool>(payload[0]);
    m_sysConfig.freqPool = pool;
    m_freqPool.activePool = pool;

    qDebug().noquote() << QString("AfhController: 频点池类型：%1")
                              .arg(pool == AfhFreqPool::Preset ? "预置" : "客制化");
}

void AfhController::handleSensitivityResponse(const QByteArray& payload)
{
    // 0x0219 响应：sensitivity(1)
    if (payload.isEmpty()) return;

    AfhSensitivity sens = static_cast<AfhSensitivity>(payload[0]);
    m_sysConfig.sensitivity = sens;

    qDebug().noquote() << QString("AfhController: 跳频灵敏度：%1")
                              .arg(sens == AfhSensitivity::High ? "高" : "低");
}

void AfhController::handleVolumeResponse(const QByteArray& payload)
{
    // 0x0210 响应：currentIndex(1) + maxIndex(1)
    if (payload.size() < 2) return;

    uint8_t currentIndex = static_cast<uint8_t>(payload[0]);
    uint8_t maxIndex = static_cast<uint8_t>(payload[1]);

    m_sysConfig.volumeIndex = currentIndex;
    m_sysConfig.maxVolume = maxIndex;

    qDebug().noquote() << QString("AfhController: 无线音量：%1/%2").arg(currentIndex).arg(maxIndex);
}


// ==================== 响应处理函数 - 频点池 ====================

void AfhController::handlePresetNsResponse(const QByteArray& payload)
{
    // 0x0215 响应：count(1)，固定为 64
    if (payload.isEmpty()) return;

    int count = static_cast<uint8_t>(payload[0]);
    m_freqPool.initPresetPool(count);

    qDebug().noquote() << QString("AfhController: 预置频点数量：%1").arg(count);
}

void AfhController::handlePresetFreqResponse(const QByteArray& payload)
{
    // 0x0216 响应：chId(1) + freq(4)
    if (payload.size() < 5) return;

    uint8_t chId = static_cast<uint8_t>(payload[0]);
    quint32 freqKhz = parseFreqFromPayload(payload, 1);

    m_freqPool.updatePresetFreq(chId, freqKhz);
}

void AfhController::handleCustomNsResponse(const QByteArray& payload)
{
    // 0x0217 响应：count(1)
    if (payload.isEmpty()) return;

    int count = static_cast<uint8_t>(payload[0]);
    m_freqPool.initCustomPool(count);

    qDebug().noquote() << QString("AfhController: 客制化频点数量：%1").arg(count);
}

void AfhController::handleCustomFreqResponse(const QByteArray& payload)
{
    // 0x0218 响应：chId(1) + freq(4)
    if (payload.size() < 5) return;

    uint8_t chId = static_cast<uint8_t>(payload[0]);
    quint32 freqKhz = parseFreqFromPayload(payload, 1);

    m_freqPool.updateCustomFreq(chId, freqKhz);
}


// ==================== 响应处理函数 - 端口配置 ====================

void AfhController::handlePortFreqResponse(const QByteArray& payload)
{
    // 0x021A 响应：portId(1) + chId(1)
    if (payload.size() < 2) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    uint8_t chId = static_cast<uint8_t>(payload[1]);

    m_portConfig.updateFreqId(portId, chId);
}

void AfhController::handlePortRangeResponse(const QByteArray& payload)
{
    // 0x021B 响应：portId(1) + startFreq(4) + endFreq(4)
    if (payload.size() < 9) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    quint32 startFreq = parseFreqFromPayload(payload, 1);
    quint32 endFreq = parseFreqFromPayload(payload, 5);

    m_portConfig.updateRange(portId, startFreq, endFreq);
}


// ==================== 响应处理函数 - 端口状态 ====================

void AfhController::handlePortPhaseLockResponse(const QByteArray& payload)
{
    // 0x0604 响应：portId(1) + locked(1)
    if (payload.size() < 2) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    bool locked = (payload[1] != 0);

    bool wasLocked = m_portStatus.isPhaseLocked(portId);
    m_portStatus.updatePhaseLock(portId, locked);

    // 状态变化时发射主动上报信号
    if (wasLocked != locked) {
        emit portPhaseLockChanged(portId, locked);
    }

    emit portStatusChanged();
}

void AfhController::handlePortRssiResponse(const QByteArray& payload)
{
    // 0x0605 响应：portId(1) + rssi(1)
    if (payload.size() < 2) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    qint8 rssi = parseRssiFromPayload(payload, 1);

    qint8 oldRssi = m_portStatus.getRssi(portId);
    m_portStatus.updateRssi(portId, rssi);

    // RSSI 变化超过 6dB 时发射信号
    if (qAbs(oldRssi - rssi) >= 6) {
        emit portRssiChanged(portId, rssi);
    }

    emit portStatusChanged();
}

void AfhController::handlePortAudioRssiResponse(const QByteArray& payload)
{
    // 0x0606 响应：portId(1) + audioRssi(2)
    if (payload.size() < 3) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    quint16 audioRssi = (static_cast<uint8_t>(payload[1]) << 8) |
                        static_cast<uint8_t>(payload[2]);

    uint8_t oldPercent = m_portStatus.getAfPercent(portId);
    m_portStatus.updateAudioRssi(portId, audioRssi);
    uint8_t newPercent = m_portStatus.getAfPercent(portId);

    // 音频电平变化超过 10% 时发射信号
    if (qAbs(static_cast<int>(oldPercent) - static_cast<int>(newPercent)) >= 10) {
        emit portAudioLevelChanged(portId, newPercent);
    }

    emit portStatusChanged();
}

void AfhController::handlePortUserDataResponse(const QByteArray& payload)
{
    // 0x0603 响应：portId(1) + userData(1)
    if (payload.size() < 2) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    uint8_t userData = static_cast<uint8_t>(payload[1]);

    // 解析电量百分比：低6位 × 2，最大100
    uint8_t batteryPercent = (userData & 0x3F) * 2;
    if (batteryPercent > 100) {
        batteryPercent = 100;
    }

    uint8_t oldPercent = m_portStatus.getBatteryPercent(portId);
    m_portStatus.updateUserData(portId, userData, batteryPercent);

    // 电量变化超过 10% 时发射信号
    if (qAbs(static_cast<int>(oldPercent) - static_cast<int>(batteryPercent)) >= 10) {
        emit portBatteryChanged(portId, batteryPercent);
    }

    emit portStatusChanged();
}

void AfhController::handlePortFreqInfoResponse(const QByteArray& payload)
{
    // 0x0601 响应：portId(1) + freq(4)
    if (payload.size() < 5) return;

    uint8_t portId = static_cast<uint8_t>(payload[0]);
    quint32 freqKhz = parseFreqFromPayload(payload, 1);

    // 同时需要 0x0602 获取 chId，这里暂不更新 chId
    m_portStatus.updateFreq(portId, freqKhz, 0);

    emit portStatusChanged();
}


// ==================== 响应处理函数 - 统计 ====================

void AfhController::handleRfStatsResponse(const QByteArray& payload)
{
    // 0x0401 响应：apId(1) + txCount(4) + rxError(4) + rxOther(4) + rxSelf(4) + lastRssi(1)
    // 总计 1 + 4*4 + 1 = 18 字节
    if (payload.size() < 18) return;

    AfhRfStats stats;
    int offset = 0;

    stats.apId = static_cast<uint8_t>(payload[offset++]);
    stats.txPacketCount = (static_cast<uint8_t>(payload[offset+0]) << 24) |
                          (static_cast<uint8_t>(payload[offset+1]) << 16) |
                          (static_cast<uint8_t>(payload[offset+2]) << 8) |
                          static_cast<uint8_t>(payload[offset+3]);
    offset += 4;

    stats.rxErrorCount = (static_cast<uint8_t>(payload[offset+0]) << 24) |
                         (static_cast<uint8_t>(payload[offset+1]) << 16) |
                         (static_cast<uint8_t>(payload[offset+2]) << 8) |
                         static_cast<uint8_t>(payload[offset+3]);
    offset += 4;

    stats.rxOtherCount = (static_cast<uint8_t>(payload[offset+0]) << 24) |
                         (static_cast<uint8_t>(payload[offset+1]) << 16) |
                         (static_cast<uint8_t>(payload[offset+2]) << 8) |
                         static_cast<uint8_t>(payload[offset+3]);
    offset += 4;

    stats.rxSelfCount = (static_cast<uint8_t>(payload[offset+0]) << 24) |
                        (static_cast<uint8_t>(payload[offset+1]) << 16) |
                        (static_cast<uint8_t>(payload[offset+2]) << 8) |
                        static_cast<uint8_t>(payload[offset+3]);
    offset += 4;

    stats.lastRssi = static_cast<uint8_t>(payload[offset]);

    m_rfStats.updateStats(stats);
    emit rfStatsReceived(stats.apId, stats);

    qDebug().noquote() << QString("AfhController: AP%1 统计 - TX:%2, RX:%3, 成功率:%4%")
                              .arg(stats.apId)
                              .arg(stats.txPacketCount)
                              .arg(stats.rxSelfCount)
                              .arg(stats.successRate() * 100, 0, 'f', 1);
}


// ==================== 状态导出/导入 ====================

QVariantMap AfhController::exportState() const
{
    QVariantMap state;

    state["hostInfo"] = m_hostInfo.toVariantMap();
    state["systemConfig"] = m_sysConfig.toVariantMap();
    state["apInfoList"] = m_apTable.toVariantList();
    state["portRangeList"] = m_portConfig.getRangeList();
    state["portFreqList"] = m_portConfig.getFreqIdList();
    state["customFreqList"] = m_freqPool.getCustomList();
    state["activeFreqPool"] = static_cast<int>(m_freqPool.activePool);

    state["_address"] = m_hostInfo.address;
    state["_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    state["_version"] = "1.0";

    return state;
}

void AfhController::importState(const QVariantMap& state)
{
    // 验证地址
    if (state.contains("_address")) {
        uint8_t savedAddress = state["_address"].toUInt();
        if (savedAddress != m_hostInfo.address) {
            qWarning().noquote() << QString("AfhController: 状态地址不匹配！期望：%1，实际：%2")
                                        .arg(formatAddressHex(m_hostInfo.address))
                                        .arg(formatAddressHex(savedAddress));
        }
    }

    // 恢复系统配置
    if (state.contains("systemConfig")) {
        QVariantMap config = state["systemConfig"].toMap();
        m_sysConfig.workMode = static_cast<AfhWorkMode>(config.value("workMode", 1).toInt());
        m_sysConfig.freqPool = static_cast<AfhFreqPool>(config.value("freqPool", 0).toInt());
        m_sysConfig.sensitivity = static_cast<AfhSensitivity>(config.value("sensitivity", 1).toInt());
        m_sysConfig.volumeIndex = config.value("volumeIndex", 6).toUInt();
        emit systemConfigChanged();
    }

    // 恢复端口配置
    if (state.contains("portRangeList")) {
        // 需要逐条解析并调用 updateRange
    }

    if (state.contains("portFreqList")) {
        // 需要逐条解析并调用 updateFreqId
    }

    // 恢复客制化频点池
    if (state.contains("customFreqList")) {
        // 需要逐条解析并调用 updateCustomFreq
    }

    // 恢复激活的频点池
    if (state.contains("activeFreqPool")) {
        m_freqPool.activePool = static_cast<AfhFreqPool>(state["activeFreqPool"].toInt());
    }

    notifyStateSnapshot();

    qDebug().noquote() << QString("AfhController: 状态恢复完成，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));
}

void AfhController::notifyStateSnapshot()
{
    // 通知 HostManager 可以保存状态
    // 此信号在 XdcController 中有定义，这里暂不实现
}


// ==================== Getter（供 QML 属性绑定）====================

QVariantMap AfhController::viewHostInfo() const
{
    return m_hostInfo.toVariantMap();
}

QVariantList AfhController::viewApInfoList() const
{
    return m_apTable.toVariantList();
}

QVariantMap AfhController::viewSystemConfig() const
{
    return m_sysConfig.toVariantMap();
}

QVariantList AfhController::viewPresetFreqList() const
{
    return m_freqPool.getPresetList();
}

QVariantList AfhController::viewCustomFreqList() const
{
    return m_freqPool.getCustomList();
}

int AfhController::presetFreqCount() const
{
    return m_freqPool.presetPool.size();
}

int AfhController::customFreqCount() const
{
    return m_freqPool.customPool.size();
}

QVariantList AfhController::viewPortStatusList() const
{
    return m_portStatus.toVariantList();
}

QVariantList AfhController::viewPortRangeList() const
{
    return m_portConfig.getRangeList();
}

QVariantList AfhController::viewPortFreqList() const
{
    return m_portConfig.getFreqIdList();
}

QVariantList AfhController::viewScanDataList() const
{
    return m_scanCache.toVariantList();
}

int AfhController::scanProgress() const
{
    return m_scanCache.progress;
}

int AfhController::scanApId() const
{
    return m_scanCache.apId;
}

QString AfhController::scanApName() const
{
    return QString("AP%1").arg(m_scanCache.apId + 1);
}


// ==================== 分组1：初始化与主机信息 ====================

void AfhController::initialize()
{
    qDebug().noquote() << QString("AfhController: 开始初始化，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    // 清空所有数据容器
    m_hostInfo.clear();
    m_sysConfig.clear();
    m_apTable.clear();
    m_freqPool.clear();
    m_portConfig.clear();
    m_portStatus.clear();
    m_scanCache.clear();
    m_rfStats.clear();

    // 启动初始化序列
    startInitSequence();
}

void AfhController::fetchHostHardwareInfo()
{
    qDebug().noquote() << QString("AfhController: 批量获取主机硬件信息，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    // 1. 生成唯一批次ID
    QString batchId = QString("host_hardware_%1_%2")
                          .arg(m_hostInfo.address)
                          .arg(QDateTime::currentMSecsSinceEpoch());

    // 2. 构建4个请求
    QVector<ProtocolProcessor::PendingRequest> requests;

    auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                             const QString& operation) -> ProtocolProcessor::PendingRequest {
        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostInfo.address, MASTER_ADDR, 0x01, cmd, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;
        req.userData = QVariant(batchId + "|" + operation);
        return req;
    };

    // 0x0202: 获取主机版本号
    requests.append(createRequest(Command::Host::GET_VERSION, QByteArray(), "host_version"));
    // 0x0203: 获取主机编译日期
    requests.append(createRequest(Command::Host::GET_BUILD_DATE, QByteArray(), "host_build_date"));
    // 0x0204: 获取端口数量
    requests.append(createRequest(Command::Host::GET_PORT_COUNT, QByteArray(), "host_port_count"));
    // 0x0205: 获取天线盒数量
    requests.append(createRequest(Command::Host::GET_AP_COUNT, QByteArray(), "host_ap_count"));

    // 3. 记录批次上下文（硬编码预期响应数 = 4）
    BatchContext ctx;
    ctx.batchId = batchId;
    ctx.expected = 4;
    ctx.received = 0;
    ctx.completed = false;
    ctx.operation = "fetchHostHardwareInfo";
    m_activeBatches[batchId] = ctx;

    qDebug().noquote() << QString("AfhController: 批次ID：%1，期望响应数：%2")
                              .arg(batchId).arg(ctx.expected);

    // 4. 发送所有请求
    m_protocolProcessor->sendCommandQueue(requests);
}


// ==================== 初始化序列 ====================

void AfhController::startInitSequence()
{
    m_initStep = 1;
    m_initProgress = 0;

    qDebug().noquote() << QString("AfhController: 开始初始化序列，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    // 第一步：批量获取主机硬件信息
    fetchHostHardwareInfo();

    // 启动定时器等待批次完成
    m_initStepTimer->start(100);
}

void AfhController::onInitStepTimer()
{
    // 检查当前步骤的批次是否完成
    bool stepCompleted = false;

    switch (m_initStep) {
    case 1:
        // 等待 fetchHostHardwareInfo 完成
        stepCompleted = !hasActiveBatch("fetchHostHardwareInfo");
        if (stepCompleted) {
            m_initProgress = 20;
            emit initProgressChanged();
            m_initStep = 2;
            // 下一步：批量获取天线盒信息
            fetchApInfo();
        }
        break;

    case 2:
        stepCompleted = !hasActiveBatch("fetchApInfo");
        if (stepCompleted) {
            m_initProgress = 40;
            emit initProgressChanged();
            m_initStep = 3;
            // 下一步：加载频点池
            loadActiveFreqPool();
        }
        break;

    case 3:
        stepCompleted = !hasActiveBatch("loadPresetFreqPool") &&
                        !hasActiveBatch("loadCustomFreqPool");
        if (stepCompleted) {
            m_initProgress = 60;
            emit initProgressChanged();
            m_initStep = 4;
            // 下一步：获取系统配置
            fetchSystemConfig();
        }
        break;

    case 4:
        stepCompleted = !hasActiveBatch("fetchSystemConfig");
        if (stepCompleted) {
            m_initProgress = 80;
            emit initProgressChanged();
            m_initStep = 5;
            // 下一步：获取端口配置
            fetchAllPortConfig();
        }
        break;

    case 5:
        stepCompleted = !hasActiveBatch("fetchAllPortConfig");
        if (stepCompleted) {
            m_initProgress = 100;
            emit initProgressChanged();
            m_initStep = 6;

            // 初始化完成
            emit initializedChanged();

            // 开始状态轮询
            setStatusPolling(true, 500);

            m_initStepTimer->stop();

            qDebug().noquote() << QString("AfhController: 初始化完成，地址：%1")
                                      .arg(formatAddressHex(m_hostInfo.address));
        }
        break;

    default:
        break;
    }
}

void AfhController::processInitStep()
{
    // 由 onInitStepTimer 调用，这里保留空实现
}

bool AfhController::hasActiveBatch(const QString& operation) const
{
    for (auto it = m_activeBatches.constBegin(); it != m_activeBatches.constEnd(); ++it) {
        if (it.value().operation == operation && !it.value().completed) {
            return true;
        }
    }
    return false;
}


// ==================== 分组2：天线盒信息 ====================

void AfhController::fetchApInfo(uint8_t apId)
{
    qDebug().noquote() << QString("AfhController: 批量获取天线盒信息，地址：%1，apId：%2")
                              .arg(formatAddressHex(m_hostInfo.address))
                              .arg(apId == 0xFF ? "全部" : QString::number(apId));

    // 确定要查询的 AP 列表
    QList<uint8_t> apList;
    if (apId == 0xFF) {
        for (uint8_t i = 0; i < m_hostInfo.apCount; ++i) {
            apList.append(i);
        }
    } else {
        apList.append(apId);
    }

    if (apList.isEmpty()) {
        qWarning() << "AfhController: 没有需要查询的天线盒";
        return;
    }

    // 生成批次ID
    QString batchId = QString("ap_info_%1_%2")
                          .arg(m_hostInfo.address)
                          .arg(QDateTime::currentMSecsSinceEpoch());

    QVector<ProtocolProcessor::PendingRequest> requests;

    auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                             const QString& operation) -> ProtocolProcessor::PendingRequest {
        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostInfo.address, MASTER_ADDR, 0x01, cmd, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;
        req.userData = QVariant(batchId + "|" + operation);
        return req;
    };

    // 对每个 AP 发送 4 个请求
    for (uint8_t id : apList) {
        QByteArray apIdPayload(1, static_cast<char>(id));

        // 0x0206: 获取在线状态
        requests.append(createRequest(Command::Ap::GET_STATUS, apIdPayload,
                                      QString("ap%1_status").arg(id)));
        // 0x0207: 获取实际类型
        requests.append(createRequest(Command::Ap::GET_TYPE, apIdPayload,
                                      QString("ap%1_type").arg(id)));
        // 0x0208: 获取版本号
        requests.append(createRequest(Command::Ap::GET_VERSION, apIdPayload,
                                      QString("ap%1_version").arg(id)));
        // 0x0209: 获取编译日期
        requests.append(createRequest(Command::Ap::GET_DATE, apIdPayload,
                                      QString("ap%1_date").arg(id)));
    }

    // 记录批次上下文（硬编码预期响应数 = apList.size() * 4）
    BatchContext ctx;
    ctx.batchId = batchId;
    ctx.expected = apList.size() * 4;
    ctx.received = 0;
    ctx.completed = false;
    ctx.operation = "fetchApInfo";
    m_activeBatches[batchId] = ctx;

    qDebug().noquote() << QString("AfhController: 批次ID：%1，期望响应数：%2")
                              .arg(batchId).arg(ctx.expected);

    m_protocolProcessor->sendCommandQueue(requests);
}

void AfhController::setApConfigType(uint8_t apId, uint8_t type)
{
    // 0x0112: 设置天线盒配置类型
    qDebug().noquote() << QString("AfhController: 设置 AP%1 配置类型：%2")
                              .arg(apId).arg(type);

    QByteArray payload;
    payload.append(static_cast<char>(apId));
    payload.append(static_cast<char>(type));

    sendCommand(0x01, Command::Ap::SET_CONFIG, payload, "setApConfigType");
}


// ==================== 分组3：系统配置 ====================

void AfhController::fetchSystemConfig()
{
    qDebug().noquote() << QString("AfhController: 批量获取系统配置，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    QString batchId = QString("sys_config_%1_%2")
                          .arg(m_hostInfo.address)
                          .arg(QDateTime::currentMSecsSinceEpoch());

    QVector<ProtocolProcessor::PendingRequest> requests;

    auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                             const QString& operation) -> ProtocolProcessor::PendingRequest {
        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostInfo.address, MASTER_ADDR, 0x01, cmd, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;
        req.userData = QVariant(batchId + "|" + operation);
        return req;
    };

    // 0x0210: 获取无线音量
    requests.append(createRequest(Command::Config::GET_VOLUME, QByteArray(), "volume"));
    // 0x0213: 获取工作模式
    requests.append(createRequest(Command::Config::GET_WORK_MODE, QByteArray(), "work_mode"));
    // 0x0214: 获取频点池类型
    requests.append(createRequest(Command::Config::GET_FREQ_POOL, QByteArray(), "freq_pool"));
    // 0x0219: 获取跳频灵敏度
    requests.append(createRequest(Command::Config::GET_SENSITIVITY, QByteArray(), "sensitivity"));

    // 对每个 AP 获取配置类型
    for (uint8_t i = 0; i < m_hostInfo.apCount; ++i) {
        QByteArray apIdPayload(1, static_cast<char>(i));
        requests.append(createRequest(Command::Ap::GET_CONFIG, apIdPayload,
                                      QString("ap%1_config").arg(i)));
    }

    // 预期响应数 = 4 + apCount
    int expected = 4 + m_hostInfo.apCount;

    BatchContext ctx;
    ctx.batchId = batchId;
    ctx.expected = expected;
    ctx.received = 0;
    ctx.completed = false;
    ctx.operation = "fetchSystemConfig";
    m_activeBatches[batchId] = ctx;

    m_protocolProcessor->sendCommandQueue(requests);
}

void AfhController::setWorkMode(uint8_t mode)
{
    // 0x0113: 设置工作模式
    QByteArray payload(1, static_cast<char>(mode));
    sendCommand(0x01, Command::Config::SET_WORK_MODE, payload, "setWorkMode");
}

void AfhController::setFreqPool(uint8_t pool)
{
    // 0x0114: 设置频点池类型
    QByteArray payload(1, static_cast<char>(pool));
    sendCommand(0x01, Command::Config::SET_FREQ_POOL, payload, "setFreqPool");
}

void AfhController::setSensitivity(uint8_t sens)
{
    // 0x0119: 设置跳频灵敏度
    QByteArray payload(1, static_cast<char>(sens));
    sendCommand(0x01, Command::Config::SET_SENSITIVITY, payload, "setSensitivity");
}

void AfhController::setVolume(uint8_t index)
{
    // 0x0110: 设置无线音量
    if (index > m_sysConfig.maxVolume) {
        qWarning() << "音量索引超出范围:" << index;
        return;
    }
    QByteArray payload(1, static_cast<char>(index));
    sendCommand(0x01, Command::Config::SET_VOLUME, payload, "setVolume");
}

void AfhController::getWorkMode()
{
    sendCommand(0x01, Command::Config::GET_WORK_MODE, QByteArray(), "getWorkMode");
}

void AfhController::getFreqPool()
{
    sendCommand(0x01, Command::Config::GET_FREQ_POOL, QByteArray(), "getFreqPool");
}

void AfhController::getSensitivity()
{
    sendCommand(0x01, Command::Config::GET_SENSITIVITY, QByteArray(), "getSensitivity");
}

void AfhController::getVolume()
{
    sendCommand(0x01, Command::Config::GET_VOLUME, QByteArray(), "getVolume");
}

void AfhController::getApConfigType(uint8_t apId)
{
    QByteArray payload(1, static_cast<char>(apId));
    sendCommand(0x01, Command::Ap::GET_CONFIG, payload,
                QString("getApConfig_%1").arg(apId));
}


// ==================== 分组4：频点池管理 ====================

void AfhController::loadActiveFreqPool()
{
    if (m_freqPool.activePool == AfhFreqPool::Preset) {
        // 加载预置频点池
        qDebug().noquote() << QString("AfhController: 加载预置频点池");

        QString batchId = QString("preset_pool_%1_%2")
                              .arg(m_hostInfo.address)
                              .arg(QDateTime::currentMSecsSinceEpoch());

        QVector<ProtocolProcessor::PendingRequest> requests;

        auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                                 const QString& operation) -> ProtocolProcessor::PendingRequest {
            ProtocolProcessor::PendingRequest req;
            req.commandConfig = ProtocolProcessor::CommandConfig(
                m_hostInfo.address, MASTER_ADDR, 0x01, cmd, payload
                );
            req.rawFrame = ProtocolProcessor::buildFrame(
                req.commandConfig.destAddr, req.commandConfig.srcAddr,
                req.commandConfig.opCode, req.commandConfig.command,
                req.commandConfig.payload
                );
            req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
            req.maxRetries = 3;
            req.userData = QVariant(batchId + "|" + operation);
            return req;
        };

        // 0x0215: 获取预置频点数量
        requests.append(createRequest(Command::FreqPool::GET_PRESET_NS, QByteArray(), "preset_ns"));

        // 预设预置频点数量为 64，先初始化容器
        m_freqPool.initPresetPool(PRESET_FREQ_COUNT);

        // 0x0216: 获取每个预置频点值
        for (int i = 0; i < PRESET_FREQ_COUNT; ++i) {
            QByteArray payload(1, static_cast<char>(i));
            requests.append(createRequest(Command::FreqPool::GET_PRESET_FREQ, payload,
                                          QString("preset_freq_%1").arg(i)));
        }

        BatchContext ctx;
        ctx.batchId = batchId;
        ctx.expected = 1 + PRESET_FREQ_COUNT;  // 1个数量响应 + 64个频点响应
        ctx.received = 0;
        ctx.completed = false;
        ctx.operation = "loadPresetFreqPool";
        m_activeBatches[batchId] = ctx;

        m_protocolProcessor->sendCommandQueue(requests);

    } else {
        // 加载客制化频点池
        qDebug().noquote() << QString("AfhController: 加载客制化频点池");

        // 先获取数量
        getCustomFreqCount();
    }
}

void AfhController::setCustomFreqCount(uint8_t count)
{
    // 0x0117: 设置客制化频点数量
    if (count < MIN_CUSTOM_FREQ_COUNT || count > MAX_CUSTOM_FREQ_COUNT) {
        qWarning() << "客制化频点数量超出范围:" << count;
        return;
    }
    QByteArray payload(1, static_cast<char>(count));
    sendCommand(0x01, Command::FreqPool::SET_CUSTOM_NS, payload, "setCustomFreqCount");
}

void AfhController::setCustomFreq(uint8_t chId, quint32 freqKhz)
{
    // 0x0118: 设置客制化频点值
    QByteArray payload;
    payload.append(static_cast<char>(chId));
    payload.append(static_cast<char>((freqKhz >> 24) & 0xFF));
    payload.append(static_cast<char>((freqKhz >> 16) & 0xFF));
    payload.append(static_cast<char>((freqKhz >> 8) & 0xFF));
    payload.append(static_cast<char>(freqKhz & 0xFF));

    sendCommand(0x01, Command::FreqPool::SET_CUSTOM_FREQ, payload,
                QString("setCustomFreq_%1").arg(chId));
}

void AfhController::getCustomFreqCount()
{
    // 0x0217: 获取客制化频点数量
    sendCommand(0x01, Command::FreqPool::GET_CUSTOM_NS, QByteArray(), "getCustomFreqCount");
}

void AfhController::getCustomFreq(uint8_t chId)
{
    // 0x0218: 获取客制化频点值
    QByteArray payload(1, static_cast<char>(chId));
    sendCommand(0x01, Command::FreqPool::GET_CUSTOM_FREQ, payload,
                QString("getCustomFreq_%1").arg(chId));
}


// ==================== 分组5：端口配置 ====================

void AfhController::fetchAllPortConfig()
{
    qDebug().noquote() << QString("AfhController: 批量获取端口配置，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    QString batchId = QString("port_config_%1_%2")
                          .arg(m_hostInfo.address)
                          .arg(QDateTime::currentMSecsSinceEpoch());

    QVector<ProtocolProcessor::PendingRequest> requests;

    auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                             const QString& operation) -> ProtocolProcessor::PendingRequest {
        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostInfo.address, MASTER_ADDR, 0x01, cmd, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;
        req.userData = QVariant(batchId + "|" + operation);
        return req;
    };

    // 对每个端口发送 2 个请求
    for (uint8_t portId = 0; portId < m_hostInfo.portCount; ++portId) {
        QByteArray portPayload(1, static_cast<char>(portId));

        // 0x021A: 获取端口频点ID
        requests.append(createRequest(Command::PortConfig::GET_FREQ_ID, portPayload,
                                      QString("port%1_freq").arg(portId)));
        // 0x021B: 获取端口频率范围
        requests.append(createRequest(Command::PortConfig::GET_RANGE, portPayload,
                                      QString("port%1_range").arg(portId)));
    }

    // 预期响应数 = portCount * 2
    int expected = m_hostInfo.portCount * 2;

    BatchContext ctx;
    ctx.batchId = batchId;
    ctx.expected = expected;
    ctx.received = 0;
    ctx.completed = false;
    ctx.operation = "fetchAllPortConfig";
    m_activeBatches[batchId] = ctx;

    m_protocolProcessor->sendCommandQueue(requests);
}

void AfhController::setPortFreq(uint8_t portId, uint8_t chId)
{
    // 0x011A: 设置端口频点ID
    QByteArray payload;
    payload.append(static_cast<char>(portId));
    payload.append(static_cast<char>(chId));

    sendCommand(0x01, Command::PortConfig::SET_FREQ_ID, payload,
                QString("setPortFreq_%1").arg(portId));
}

void AfhController::setPortRange(uint8_t portId, quint32 startFreq, quint32 endFreq)
{
    // 0x011B: 设置端口频率范围
    QByteArray payload;
    payload.append(static_cast<char>(portId));
    payload.append(static_cast<char>((startFreq >> 24) & 0xFF));
    payload.append(static_cast<char>((startFreq >> 16) & 0xFF));
    payload.append(static_cast<char>((startFreq >> 8) & 0xFF));
    payload.append(static_cast<char>(startFreq & 0xFF));
    payload.append(static_cast<char>((endFreq >> 24) & 0xFF));
    payload.append(static_cast<char>((endFreq >> 16) & 0xFF));
    payload.append(static_cast<char>((endFreq >> 8) & 0xFF));
    payload.append(static_cast<char>(endFreq & 0xFF));

    sendCommand(0x01, Command::PortConfig::SET_RANGE, payload,
                QString("setPortRange_%1").arg(portId));
}

void AfhController::getPortFreq(uint8_t portId)
{
    QByteArray payload(1, static_cast<char>(portId));
    sendCommand(0x01, Command::PortConfig::GET_FREQ_ID, payload,
                QString("getPortFreq_%1").arg(portId));
}

void AfhController::getPortRange(uint8_t portId)
{
    QByteArray payload(1, static_cast<char>(portId));
    sendCommand(0x01, Command::PortConfig::GET_RANGE, payload,
                QString("getPortRange_%1").arg(portId));
}


// ==================== 分组6：端口状态 ====================

void AfhController::setStatusPolling(bool enabled, int intervalMs)
{
    if (enabled == m_isStatusPolling) {
        return;
    }

    m_isStatusPolling = enabled;

    if (enabled) {
        m_statusPollIndex = 0;
        m_statusPollTimer->start(intervalMs);
        qDebug().noquote() << QString("AfhController: 开始状态轮询，间隔：%1ms").arg(intervalMs);
    } else {
        m_statusPollTimer->stop();
        qDebug().noquote() << QString("AfhController: 停止状态轮询");
    }
}

void AfhController::onStatusPollTimer()
{
    if (!m_isStatusPolling || m_hostInfo.portCount == 0) {
        return;
    }

    uint8_t portId = m_statusPollIndex;

    QByteArray portPayload(1, static_cast<char>(portId));

    // 发送4个状态查询请求
    sendCommand(0x01, Command::PortStatus::GET_PHASELOCK, portPayload,
                QString("poll_phase_%1").arg(portId));
    sendCommand(0x01, Command::PortStatus::GET_RSSI, portPayload,
                QString("poll_rssi_%1").arg(portId));
    sendCommand(0x01, Command::PortStatus::GET_AUDIO_RSSI, portPayload,
                QString("poll_audio_%1").arg(portId));
    sendCommand(0x01, Command::PortStatus::GET_USERDATA, portPayload,
                QString("poll_user_%1").arg(portId));

    // 移动到下一个端口
    m_statusPollIndex = (portId + 1) % m_hostInfo.portCount;
}

void AfhController::refreshPortStatus(uint8_t portId)
{
    if (portId == 0xFF) {
        // 刷新所有端口
        for (uint8_t i = 0; i < m_hostInfo.portCount; ++i) {
            QByteArray payload(1, static_cast<char>(i));
            sendCommand(0x01, Command::PortStatus::GET_PHASELOCK, payload);
            sendCommand(0x01, Command::PortStatus::GET_RSSI, payload);
            sendCommand(0x01, Command::PortStatus::GET_AUDIO_RSSI, payload);
            sendCommand(0x01, Command::PortStatus::GET_USERDATA, payload);
        }
    } else {
        QByteArray payload(1, static_cast<char>(portId));
        sendCommand(0x01, Command::PortStatus::GET_PHASELOCK, payload);
        sendCommand(0x01, Command::PortStatus::GET_RSSI, payload);
        sendCommand(0x01, Command::PortStatus::GET_AUDIO_RSSI, payload);
        sendCommand(0x01, Command::PortStatus::GET_USERDATA, payload);
    }
}


// ==================== 分组7：频谱扫描 ====================

void AfhController::startScan(uint8_t apId, int intervalMs)
{
    if (m_scanCache.isScanning) {
        qWarning() << "AfhController: 扫描已在进行中";
        return;
    }

    qDebug().noquote() << QString("AfhController: 开始频谱扫描，AP%1，间隔：%2ms")
                              .arg(apId).arg(intervalMs);

    m_scanCache.clear();

    // 初始化缓存大小为当前激活频点池的大小
    int poolSize = m_freqPool.getActivePoolSize();
    m_scanCache.initCache(poolSize);
    m_scanCache.apId = apId;
    m_scanCache.isScanning = true;
    m_scanCache.currentChId = 0;

    emit scanApIdChanged();
    emit scanningChanged();

    m_scanTimer->start(intervalMs);
}

void AfhController::stopScan()
{
    if (!m_scanCache.isScanning) {
        return;
    }

    qDebug().noquote() << QString("AfhController: 停止频谱扫描");

    m_scanTimer->stop();
    m_scanCache.isScanning = false;

    emit scanningChanged();
    emit scanCompleted(m_scanCache.apId, true, "扫描已停止");
}

void AfhController::scanOnce(uint8_t apId)
{
    qDebug().noquote() << QString("AfhController: 执行单次扫描，AP%1").arg(apId);

    // 单次扫描：发送所有频点的请求，但不启动定时器
    m_scanCache.clear();
    int poolSize = m_freqPool.getActivePoolSize();
    m_scanCache.initCache(poolSize);
    m_scanCache.apId = apId;
    m_scanCache.currentChId = 0;

    emit scanApIdChanged();

    // 请求所有频点的 RSSI
    for (int chId = 0; chId < poolSize; ++chId) {
        requestNextScanFreq();
    }
}

void AfhController::refreshScanProgress()
{
    // 可以查询主机当前的扫描进度（如果有对应协议）
    // 暂不实现
}

void AfhController::onScanTimer()
{
    if (!m_scanCache.isScanning) {
        return;
    }

    requestNextScanFreq();
}

void AfhController::requestNextScanFreq()
{
    int poolSize = m_freqPool.getActivePoolSize();
    if (poolSize == 0) {
        return;
    }

    uint8_t chId = m_scanCache.currentChId;

    // 根据当前频点池类型选择命令
    uint16_t cmd = (m_freqPool.activePool == AfhFreqPool::Preset)
                       ? Command::FreqPool::GET_PRESET_FREQ
                       : Command::FreqPool::GET_CUSTOM_FREQ;

    QByteArray payload(1, static_cast<char>(chId));
    sendCommand(0x01, cmd, payload, QString("scan_ch_%1").arg(chId));

    // 更新进度
    m_scanCache.currentChId = (chId + 1) % poolSize;
    m_scanCache.setProgress(m_scanCache.currentChId, poolSize);

    emit scanProgressChanged();
    emit scanDataChanged();
}


// ==================== 分组8：统计与高级功能 ====================

void AfhController::getRfStats(uint8_t apId)
{
    // 0x0401: 获取无线报文统计
    QByteArray payload(1, static_cast<char>(apId));
    sendCommand(0x01, Command::Stats::GET_RF_STATS, payload,
                QString("getRfStats_%1").arg(apId));
}

void AfhController::autoAssignFrequencies()
{
    qDebug().noquote() << QString("AfhController: 自动分配频点，地址：%1")
                              .arg(formatAddressHex(m_hostInfo.address));

    // 对每个端口执行自动分配
    for (uint8_t portId = 0; portId < m_hostInfo.portCount; ++portId) {
        uint8_t bestChId = findBestChannelForPort(portId);
        if (bestChId < m_freqPool.getActivePoolSize()) {
            setPortFreq(portId, bestChId);
            qDebug().noquote() << QString("AfhController: 端口 %1 自动分配到 CH%2")
                                      .arg(portId + 1).arg(bestChId + 1);
        }
    }
}

uint8_t AfhController::findBestChannelForPort(uint8_t portId) const
{
    // 简化算法：找 RSSI 最低（干扰最小）且未被占用的频点
    int poolSize = m_freqPool.getActivePoolSize();
    uint8_t bestChId = m_portConfig.getFreqId(portId);  // 默认保持原频点
    qint8 bestRssi = 127;

    for (uint8_t chId = 0; chId < poolSize; ++chId) {
        if (!isChannelAvailable(chId, portId)) {
            continue;
        }

        qint8 rssi = m_scanCache.getRssi(chId);
        if (rssi < bestRssi) {
            bestRssi = rssi;
            bestChId = chId;
        }
    }

    return bestChId;
}

bool AfhController::isChannelAvailable(uint8_t chId, uint8_t excludePortId) const
{
    // 检查频点是否在端口频率范围内
    AfhPortRange range = m_portConfig.getRange(excludePortId);
    AfhFreqInfo freqInfo = m_freqPool.getFreqInfo(chId);

    if (freqInfo.freqKhz < range.startFreqKhz || freqInfo.freqKhz > range.endFreqKhz) {
        return false;
    }

    // 检查是否被其他端口占用
    for (uint8_t portId = 0; portId < m_hostInfo.portCount; ++portId) {
        if (portId == excludePortId) {
            continue;
        }
        if (m_portConfig.getFreqId(portId) == chId) {
            return false;  // 已被占用
        }
    }

    return true;
}