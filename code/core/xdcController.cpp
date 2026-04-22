// XDC236 有线无线融合会议系统
#include "xdcController.h"
#include "HostManager.h"
#include "ProtocolProcessor.h"
#include "../library/LogManager.h"

#include <QDebug>
#include <QDateTime>


// ==================== 构造函数/析构函数 ====================
XdcController::XdcController(uint8_t hostAddress, HostManager* parent)
    : QObject(parent)
    , m_hostAddress(hostAddress)
    , m_hostManager(parent)
    , m_protocolProcessor(new ProtocolProcessor(this))
    , m_isAudioTesting(false)
{
    LOG_INFO(QString("0x%1").arg(m_hostAddress, 2, 16, QChar('0')).toUpper());

    // 创建该主机独立的 UnitModel
    m_unitModel = new UnitModel(this);
    setupProtocolProcessor();   // 初始化协议处理器连接
    connectUnitModelSignals();  // 初始化单元模型连接
}

XdcController::~XdcController()
{
    qDebug() << "XdcController：析构地址："
             << QString("0x%1").arg(m_hostAddress, 2, 16, QChar('0')).toUpper();
}


// ==================== 初始化协议处理器 ====================
void XdcController::setupProtocolProcessor()
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
    // 请求-响应模式中连接信号槽
    connect(m_protocolProcessor, &ProtocolProcessor::responseCompleted,
            this, &XdcController::onCommandResponseCompleted);
    connect(m_protocolProcessor, &ProtocolProcessor::requestFailed,
            this, &XdcController::onCommandFailed);

    // 主动上报通知（opCode=0x02）中连接信号槽
    connect(m_protocolProcessor, &ProtocolProcessor::unsolicitedNotifyReceived,
            this, &XdcController::onUnsolicitedNotifyReceived);
}

// ==================== 切换数据实现 ====================
QVariantMap XdcController::exportState() const
{
    QVariantMap state;

    // 分组1：硬件信息
    state["hardwareInfo"] = m_hardwareInfo;

    // 分组2：音量设置
    state["volumeSettings"] = m_volumeSettings;

    // 分组3：发言设置
    state["speechSettings"] = m_speechSettings;

    // 分组4：摄像设置
    state["cameraSettings"] = m_cameraSettings;

    // 分组7：系统时间
    state["systemDateTime"] = m_systemDateTime.toString(Qt::ISODate);

    // 添加元数据
    state["_address"] = m_hostAddress;
    state["_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    state["_version"] = "1.0";

    return state;
}

void XdcController::importState(const QVariantMap& state)
{
    // 验证地址
    if (state.contains("_address")) {
        uint8_t savedAddress = state["_address"].toUInt();
        if (savedAddress != m_hostAddress) {
            qWarning().noquote() << QString("XdcController：状态地址不匹配！期望：%1，实际：%2")
                                        .arg(formatAddressHex(m_hostAddress), formatAddressHex(savedAddress));
        }
    }

    // 恢复音量设置
    if (state.contains("volumeSettings")) {
        m_volumeSettings = state["volumeSettings"].toMap();
        emit volumeSettingsChanged();
    }

    // 恢复发言设置
    if (state.contains("speechSettings")) {
        m_speechSettings = state["speechSettings"].toMap();
        emit speechSettingsChanged();
    }

    // 恢复摄像设置
    if (state.contains("cameraSettings")) {
        m_cameraSettings = state["cameraSettings"].toMap();
        emit cameraSettingsChanged();
    }

    // 恢复系统时间
    if (state.contains("systemDateTime")) {
        m_systemDateTime = QDateTime::fromString(state["systemDateTime"].toString(), Qt::ISODate);
        emit systemDateTimeUpdated(m_hostAddress, m_systemDateTime);
    }

    // 发射批量更新信号
    QVariantMap allProps;
    allProps["hardwareInfo"] = m_hardwareInfo;
    allProps["volumeSettings"] = m_volumeSettings;
    allProps["speechSettings"] = m_speechSettings;
    allProps["cameraSettings"] = m_cameraSettings;
    notifyBatchDataUpdate(DataType::Property, allProps);
    notifyStateSnapshot();

    qDebug().noquote() << QString("XdcController：状态恢复完成，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
}

// ==================== 通用发送功能 ====================
QVariantList XdcController::buildCommandQueue(const QVariantList& commands)
{
    QVariantList queue;

    for (const auto& cmdVariant : commands) {
        QVariantMap cmd = cmdVariant.toMap();

        // 获取命令码
        if (!cmd.contains("command")) {
            qWarning() << "buildCommandQueue: 命令缺少 command 字段";
            continue;
        }

        uint16_t command = cmd["command"].toUInt();
        QByteArray payload;

        // 如果有 payload 字段则使用
        if (cmd.contains("payload")) {
            payload = cmd["payload"].toByteArray();
        }

        // 操作名称，用于回调标识
        QString operation = cmd.value("operation", QString("get_0x%1").arg(command, 4, 16, QChar('0'))).toString();

        QVariantMap request;
        request["opCode"] = static_cast<uint8_t>(0x01);  // 请求操作码
        request["command"] = command;
        request["payload"] = payload;
        request["userData"] = operation;

        queue.append(request);
    }

    qDebug().noquote() << QString("XdcController: 生成获取指令队列，地址：%1，数量：%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(queue.size());

    return queue;
}

void XdcController::sendToHost(const QByteArray& data)
{
    if (m_hostManager) {
        m_hostManager->sendRawData(data);
    } else {
        qWarning().noquote() << QString("XdcController：HostManager为空，无法发送数据");
    }
}

void XdcController::sendCommand(uint8_t opCode, uint16_t command,
                                const QByteArray& payload,
                                const QVariant& userData)
{
    QString opStr = QString("0x%1").arg(opCode, 2, 16, QChar('0'));
    QString cmdStr = QString("0x%1").arg(command, 4, 16, QChar('0'));
    QString userDataStr = userData.toString();
    QString payloadStr = payload.toHex(' ').toUpper();

    // 详细日志：记录所有发送的指令
    qDebug().noquote() << QString("XdcController: 发送指令 - Op:0x%1 Cmd:0x%2 UserData:%3 Payload:%4")
               .arg(opStr, cmdStr, userDataStr, payloadStr);

    LOG_TX(QString("Op:%1 Cmd:%2 UserData:%3 Payload:%4")
               .arg(opStr, cmdStr, userDataStr, payloadStr));

    ProtocolProcessor::CommandConfig config(
        m_hostAddress,      // 目标地址
        0x80,               // 源地址
        opCode,             // 操作码
        command,            // 命令码
        payload             // 负载
        );

    m_protocolProcessor->sendCommand(config, -1, userData);
}

void XdcController::sendCommandQueue(const QVariantList& commandQueue)
{
    QVector<ProtocolProcessor::PendingRequest> requests;

    for (const auto& cmdVariant : commandQueue) {
        QVariantMap cmd = cmdVariant.toMap();

        uint8_t opCode = cmd["opCode"].toUInt();
        uint16_t command = cmd["command"].toUInt();
        QByteArray payload = cmd["payload"].toByteArray();
        QString userData = cmd["userData"].toString();

        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostAddress, 0x80, opCode, command, payload
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

// ==================== 通用回调处理 ====================
// 收到主机返回的指令帧，第一种类型，请求-响应模式处理的响应指令帧
void XdcController::onCommandResponseCompleted(const ProtocolProcessor::ResponseResult& result)
{
    if (!result.success) {
        qWarning().noquote() << QString("XdcController：响应失败：%1").arg(result.errorMessage);
        return;
    }

    // 1. 解析 userData，提取批次ID，提取批次ID和原始操作名
    QString userDataStr = result.pendingRequest.userData.toString();
    QString batchId;
    QString originalOperation;

    if (userDataStr.contains('|')) {
        QStringList parts = userDataStr.split('|');
        batchId = parts[0];     // "hardware_0x01_xxx"
        originalOperation = parts[1];   // "version_kernel"
    } else {
        // 兼容旧代码：没有批次ID的情况，直接使用原逻辑
        batchId.clear();
        originalOperation = userDataStr;
    }

    qDebug().noquote() << QString(
                              "XdcController: 收到请求响应 - 地址：%1, OpCode: 0x%2, Command: 0x%3, 批次ID：%4"
                              ).arg(formatAddressHex(m_hostAddress))
                              .arg(result.parsedData.opCode, 2, 16, QChar('0'))
                              .arg(result.parsedData.command, 4, 16, QChar('0'))
                              .arg(batchId.isEmpty() ? "无" : batchId);

    // 2. 处理批次计数（核销）
    if (!batchId.isEmpty() && m_activeBatches.contains(batchId)) {
        auto& ctx = m_activeBatches[batchId];
        ctx.received++; // 收到一个响应，计数+1

        qDebug().noquote() << QString("XdcController：批次进度 %1/%2")
                                  .arg(ctx.received).arg(ctx.expected);

        // 3. 检查是否完成
        if (ctx.received >= ctx.expected && !ctx.completed) {
            ctx.completed = true;
            qDebug().noquote() << QString("XdcController：批次完成！批次ID：%1")
                                      .arg(batchId);
            notifyStateSnapshot();  // 所有xx个请求都完成了, 通知外部
            m_activeBatches.remove(batchId);    // 清理批次
        }
    }

    // 4. 解析具体数据并更新成员变量
    parseAndDispatch(result.parsedData);
}

// xdcController.cpp

void XdcController::onCommandFailed(const ProtocolProcessor::PendingRequest& request,
                                    const QString& error)
{
    qWarning().noquote() << QString("XdcController：指令执行失败：%1，用户数据：%2，CMD：0x%3")
                                .arg(error, request.userData.toString(),
                                     QString::number(request.commandConfig.command, 16));

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

    emit operationResult(m_hostAddress, operation, false, error);

    // 如果失败请求属于某个批次，也需要计数（失败也算完成，避免永久等待）
    if (!batchId.isEmpty() && m_activeBatches.contains(batchId)) {
        auto& ctx = m_activeBatches[batchId];
        ctx.received++;

        qDebug().noquote() << QString("XdcController：批次中请求失败，进度 %1/%2")
                                  .arg(ctx.received).arg(ctx.expected);

        if (ctx.received >= ctx.expected && !ctx.completed) {
            ctx.completed = true;
            qWarning().noquote() << QString("XdcController：批次完成（含失败），批次ID：%1")
                                        .arg(batchId);
            notifyStateSnapshot();
            m_activeBatches.remove(batchId);
        }
    }
}

// 收到主机返回的指令帧，第一种类型OpCode=0x02，主动上报通知处理
void XdcController::onUnsolicitedNotifyReceived(const QByteArray& rawFrame,
                                               const ProtocolProcessor::ParsedFrameData& parsedData)
{
    qDebug().noquote() << QString(
        "XdcController: 收到主动上报 - 地址：%1, OpCode: 0x%2, Command: 0x%3"
    ).arg(formatAddressHex(m_hostAddress))
     .arg(parsedData.opCode, 2, 16, QChar('0'))
     .arg(parsedData.command, 4, 16, QChar('0'));
    
    // 直接复用 parseAndDispatch()，与 Response 处理逻辑完全一致
    parseAndDispatch(parsedData);
}

// ==================== 数据接收、解析和分发 ====================
void XdcController::handleIncomingFrame(const ProtocolProcessor::ParsedFrameData& frame)
{
    qDebug().noquote() << QString("XdcController：收到HostManager转发数据，地址：%1，oPCode：0x%2，CMD：0x%3，负载：%4")
                              .arg(formatAddressHex(m_hostAddress),
                                   QString::number(frame.opCode, 16).toUpper().rightJustified(2, '0'),
                                   QString::number(frame.command, 16).toUpper().rightJustified(4, '0'),
                                   QString(frame.payload.toHex(' ').toUpper()));

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

// 收到主机返回的指令帧，第一种类型，请求-响应模式处理的响应指令帧  
void XdcController::parseAndDispatch(const ProtocolProcessor::ParsedFrameData& frame)
{
    switch (frame.command) {
    // 分组1：硬件信息
    case 0x0202: handleVersionResponse(frame); break;               // 0x04,0x0202
    case 0x0203: handleBuildDateResponse(frame); break;             // 0x04,0x0203

    // 分组2：音量响应
    case 0x0205: handleWirelessVolumeResponse(frame); break;        // 0x04,0x0205
    case 0x0206: handleWiredVolumeResponse(frame); break;           // 0x04,0x0206
    case 0x0207: handleAntennaVolumeResponse(frame); break;         // 0x04,0x0207

    // 分组3：发言相关响应
    case 0x0208: handleSpeechModeResponse(frame); break;            // 0x04,0x0208
    case 0x0209: handleMaxSpeechCountResponse(frame); break;        // 0x04,0x0209
    case 0x0210: handleMaxUnitCapacityResponse(frame); break;       // 0x04,0x0210
    case 0x020D: handleWirelessAudioPathResponse(frame); break;     // 0x04,0x020D

    // 分组4：摄像跟踪响应
    case 0x020A: handleCameraProtocolResponse(frame); break;        // 0x04,0x020A
    case 0x020B: handleCameraAddressResponse(frame); break;         // 0x04,0x020B
    case 0x020C: handleCameraBaudrateResponse(frame); break;        // 0x04,0x020C

    // 分组7：系统时间响应
    case 0x0204: handleSystemDateTimeResponse(frame); break;        // 0x04,0x0204

    // 分组8：音频通道信息响应
    case 0x0211: handleMonitorChannelCountResponse(frame); break;   // 0x04,0x0211
    case 0x0212: handleMonitorChannelResponse(frame); break;        // 0x04,0x0212
    case 0x0901: handleSetFrequencyResponse(frame); break;          // 0x04,0x0901
    case 0x0404: handleReadRSSIResponse(frame); break;              // 0x04,0x0404

    // 单元相关命令的响应
    case 0x0601: handleUnitInfoResponse(frame); break;              // 0x04,0x0601
    case 0x0602: handleUnitAliasResponse(frame); break;             // 0x04,0x0602
    case 0x0606: handleWirelessStateResponse(frame); break;         // 0x04,0x0606
    case 0x0501: break;                                             // 0x0501, 设置单元信息(身份，类型)
    case 0x0502: break;                                             // 0x0502, 设置单元别名
    case 0x0503:
    case 0x0504:
    case 0x0505: handleUnitOperationResponse(frame); break;         // 0x04,0x0503/0x0504/0x0505
    case 0x020E:
    case 0x020F: handleUnitCurrentSpeakerIdResponse(frame); break;  // 0x04,0x020F


    // 设置命令的响应(！！！目前同意发送信号operationResult(), 要检查处理结果！！！)
    case 0x0105: handleSetResponse(frame, "setWirelessVolume"); break;      // 0x04,0x0105
    case 0x0106: handleSetResponse(frame, "setWiredVolume"); break;         // 0x04,0x0106
    case 0x0107: handleSetResponse(frame, "setAntennaVolume"); break;       // 0x04,0x0107
    case 0x0108: handleSetResponse(frame, "setSpeechMode"); break;          // 0x04,0x0108
    case 0x0109: handleSetResponse(frame, "setMaxSpeechCount"); break;      // 0x04,0x0109
    case 0x010D: handleSetResponse(frame, "setWirelessAudioPath"); break;   // 0x04,0x010D
    case 0x010A: handleSetResponse(frame, "setCameraProtocol"); break;      // 0x04,0x010A
    case 0x010B: handleSetResponse(frame, "setCameraAddress"); break;       // 0x04,0x010B
    case 0x010C: handleSetResponse(frame, "setCameraBaudrate"); break;      // 0x04,0x010C
    case 0x0104: handleSetResponse(frame, "setSystemDateTime"); break;      // 0x04,0x0104

    default:
        qWarning().noquote() << QString("XdcController：未知命令：0x%1")
                                    .arg(QString::number(frame.command, 16));
    }
}

// ==================== 通用设置响应处理 ====================
void XdcController::handleSetResponse(const ProtocolProcessor::ParsedFrameData& frame, const QString& operation)
{
    // 设置命令的响应，通常表示操作成功
    qDebug().noquote() << QString("XdcController：设置命令响应，CMD：0x%1，操作：%2")
                              .arg(QString::number(frame.command, 16).toUpper(), operation);
    emit operationResult(m_hostAddress, operation, true, "操作成功");
}

// ==================== 辅助方法 ====================
void XdcController::notifyBatchDataUpdate(DataType type, const QVariantMap& data)
{
    emit dataBatchUpdated(m_hostAddress, type, data);
}

void XdcController::notifyStateSnapshot()
{
    emit stateSnapshot(m_hostAddress, exportState());
}

QString XdcController::trimNullTerminator(const QByteArray& data)   // 参数数据末尾去空格
{
    QByteArray cleaned = data;
    while (!cleaned.isEmpty() && cleaned.endsWith('\0')) {
        cleaned.chop(1);
    }
    return QString::fromUtf8(cleaned);
}

// ==================== 分组1：硬件信息相关功能（批量指令集）====================
void XdcController::getHardwareInfo()       // 获取硬件信息，软件启动查询（版本，时间，音量，等21个指令）信息
{
    qDebug().noquote() << QString("XdcController: 开始批量获取硬件信息，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));

    // 1.1 生成唯一批次ID（地址 + 毫秒时间戳）
    QString batchId = QString("hardware_%1_%2")
                          .arg(m_hostAddress)
                          .arg(QDateTime::currentMSecsSinceEpoch());

    // 1.2 构建22个请求，每个请求的userData都携带批次ID
    QVector<ProtocolProcessor::PendingRequest> requests;

    auto createRequest = [&](uint16_t cmd, const QByteArray& payload,
                             const QString& userData) -> ProtocolProcessor::PendingRequest {
        ProtocolProcessor::PendingRequest req;
        req.commandConfig = ProtocolProcessor::CommandConfig(
            m_hostAddress, 0x80, 0x01, cmd, payload
            );
        req.rawFrame = ProtocolProcessor::buildFrame(
            req.commandConfig.destAddr, req.commandConfig.srcAddr,
            req.commandConfig.opCode, req.commandConfig.command,
            req.commandConfig.payload
            );
        req.matchRule = ProtocolProcessor::MatchRule::fromCommandConfig(req.commandConfig);
        req.maxRetries = 3;

        // 将批次ID和原始操作名一起存入 userData
        // 拼接批次ID：批次ID|操作名（便于调试和后续扩展）
        QString combinedUserData = batchId + "|" + userData;
        req.userData = QVariant(combinedUserData);

        return req;
    };

    // ===== 主机系统时间查询 =====
    requests.append(createRequest(Command::SystemTime::GET_XDC_DATETIME, QByteArray(), "host_datetime"));  // 0x0204 获取系统时间

    // ===== 第一行：硬件版本/日期查询 =====
    requests.append(createRequest(0x0202, QByteArray(1, '\x00'), "version_kernel"));  // 0x0202 获取内核版本
    requests.append(createRequest(0x0203, QByteArray(1, '\x00'), "date_kernel"));     // 0x0203 获取内核编译日期
    requests.append(createRequest(0x0202, QByteArray(1, '\x01'), "version_link"));    // 0x0202 获取LINK版本
    requests.append(createRequest(0x0203, QByteArray(1, '\x01'), "date_link"));       // 0x0203 获取LINK编译日期
    requests.append(createRequest(0x0202, QByteArray(1, '\x02'), "version_anta"));    // 0x0202 获取天线盒版本
    requests.append(createRequest(0x0203, QByteArray(1, '\x02'), "date_anta"));       // 0x0203 获取天线盒编译日期

    // ===== 第二行：发言模式与数量 =====
    requests.append(createRequest(Command::Speech::GET_XDC_MODE, QByteArray(), "speech_mode"));           // 0x0208 获取发言模式
    requests.append(createRequest(Command::Speech::GET_XDC_MAX_COUNT, QByteArray(), "max_speech_count")); // 0x0209 获取最大发言数量

    // ===== 第三行：音量设置 =====
    requests.append(createRequest(Command::Volume::GET_XDC_WIRELESS, QByteArray(), "wireless_volume")); // 0x0205 获取无线音量
    requests.append(createRequest(Command::Volume::GET_XDC_WIRED, QByteArray(), "wired_volume"));       // 0x0206 获取有线音量
    requests.append(createRequest(Command::Volume::GET_XDC_ANTENNA, QByteArray(), "antenna_volume"));   // 0x0207 获取天线盒音量

    // ===== 第四行：音频路径与发言ID =====
    requests.append(createRequest(Command::AudioPath::GET_XDC_PATH, QByteArray(), "audio_path"));                     // 0x020D 获取无线音频路径
    requests.append(createRequest(Command::Unit::GET_XDC_CURRENT_WIRELESS, QByteArray(), "current_wireless_speaker")); // 0x020E 获取当前无线发言ID
    requests.append(createRequest(Command::Unit::GET_XDC_CURRENT_WIRED, QByteArray(), "current_wired_speaker"));       // 0x020F 获取当前有线发言ID

    // ===== 第五行：单元容量 =====
    requests.append(createRequest(Command::Unit::GET_XDC_MAX_REGISTER, QByteArray(), "max_unit_capacity")); // 0x0210 获取最大注册单元数量

    // ===== 第六行：摄像跟踪参数 =====
    requests.append(createRequest(Command::Camera::GET_XDC_PROTOCOL, QByteArray(), "camera_protocol")); // 0x020A 获取摄像跟踪协议
    requests.append(createRequest(Command::Camera::GET_XDC_ADDRESS, QByteArray(), "camera_address"));   // 0x020B 获取摄像跟踪地址
    requests.append(createRequest(Command::Camera::GET_XDC_BAUDRATE, QByteArray(), "camera_baudrate")); // 0x020C 获取摄像跟踪波特率

    // ===== 报文信息查询（0x0401-0x0403）=====
    // requests.append(createRequest(0x0401, QByteArray(), "msg_uplink"));
    // requests.append(createRequest(0x0402, QByteArray(), "msg_uplink"));
    // requests.append(createRequest(0x0403, QByteArray(), "msg_uplink"));

    //  1.3 记录批次上下文
    BatchContext ctx;   // 定义结构体ctx，并对其项目赋值
    ctx.batchId = batchId;
    ctx.expected = requests.size();     // = 18
    ctx.received = 0;
    ctx.completed = false;
    ctx.operation = "getHardwareInfo";
    m_activeBatches[batchId] = ctx;

    qDebug().noquote() << QString("XdcController：批量获取硬件信息，批次ID：%1，期望响应数：%2")
                              .arg(batchId).arg(ctx.expected);

    // 1.4 发送所有请求
    m_protocolProcessor->sendCommandQueue(requests);
}

void XdcController::handleVersionResponse(const ProtocolProcessor::ParsedFrameData& frame)  // 处理版本响应指令0x0202
{
    if (frame.payload.isEmpty()) return;

    quint8 subCmd = static_cast<quint8>(frame.payload.at(0));
    QString version = trimNullTerminator(frame.payload.mid(1));    // 去除末尾空白格

    if (subCmd == 0x00) {
        m_hardwareInfo["kernelVersion"] = version;
        qDebug().noquote() << QString("XdcController：内核版本：%1").arg(version);
    }
    else if (subCmd == 0x01) {
        m_hardwareInfo["linkVersion"] = version;
        qDebug().noquote() << QString("XdcController：链接版本：%1").arg(version);
    }
    else if (subCmd == 0x02) {
        m_hardwareInfo["antaVersion"] = version;
        qDebug().noquote() << QString("XdcController：天线版本：%1").arg(version);
    }

    emit hardwareInfoChanged();

    qDebug().noquote() << QString("XdcController：[版本属性更新] 内核：%1 | Link：%2 | Anta：%3")
                              .arg(m_hardwareInfo["kernelVersion"].toString(),
                                   m_hardwareInfo["linkVersion"].toString(),
                                   m_hardwareInfo["antaVersion"].toString());
}

void XdcController::handleBuildDateResponse(const ProtocolProcessor::ParsedFrameData& frame)    // 处理编译时间响应指令// 处理“版本”响应指令0x0203
{
    if (frame.payload.isEmpty()) return;

    quint8 subCmd = static_cast<quint8>(frame.payload.at(0));
    QString date = trimNullTerminator(frame.payload.mid(1));    // 去除末尾空白格

    if (subCmd == 0x00) {
        m_hardwareInfo["kernelDate"] = date;
        qDebug().noquote() << QString("XdcController：内核编译日期：%1").arg(date);
    }
    else if (subCmd == 0x01) {
        m_hardwareInfo["linkDate"] = date;
        qDebug().noquote() << QString("XdcController：链接编译日期：%1").arg(date);
    }
    else if (subCmd == 0x02) {
        m_hardwareInfo["antaDate"] = date;
        qDebug().noquote() << QString("XdcController：天线编译日期：%1").arg(date);
    }

    emit hardwareInfoChanged();

    qDebug().noquote() << QString("XdcController：[日期属性更新] 内核：%1 | Link：%2 | Anta：%3")
                              .arg(m_hardwareInfo["kernelDate"].toString(),
                                   m_hardwareInfo["linkDate"].toString(),
                                   m_hardwareInfo["antaDate"].toString());
}

// ==================== 分组2：音量相关功能 ====================
void XdcController::getWirelessVolume() // 0x0205, 查询无线音量
{
    qDebug().noquote() << QString("XdcController: 获取无线音量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Volume::GET_XDC_WIRELESS, QByteArray(), "getWirelessVolume");
}

void XdcController::setWirelessVolume(uint8_t index)    // 0x0105，设置无线音量
{
    uint8_t maxVolume = m_volumeSettings.value("wirelessMaxVolume", 9).toUInt();
    if (index >= maxVolume) {
        qWarning() << "无线音量索引超出范围，最大值:" << maxVolume - 1;
        emit operationResult(m_hostAddress, "setWirelessVolume", false, "音量索引超出范围");
        return;
    }

    qDebug().noquote() << QString("XdcController: 设置无线音量，地址：%1，索引：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(index);
    QByteArray payload;
    payload.append(static_cast<char>(index));
    sendCommand(0x01, Command::Volume::SET_XDC_WIRELESS, payload, "setWirelessVolume");
}

void XdcController::handleWirelessVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x0205, 处理“无线音量”响应
{
    if (frame.payload.size() < 2) {
        qWarning() << "无线音量响应数据长度不足";
        return;
    }

    uint8_t currentIndex = static_cast<uint8_t>(frame.payload.at(0));
    uint8_t maxCount = static_cast<uint8_t>(frame.payload.at(1));

    m_volumeSettings["wirelessVolume"] = currentIndex;
    m_volumeSettings["wirelessMaxVolume"] = maxCount;

    emit volumeSettingsChanged();

    qDebug().noquote() << QString("XdcController：无线音量：当前索引=%1，最大档位=%2")
                              .arg(currentIndex).arg(maxCount);
}

void XdcController::getWiredVolume()    // 0x0206, 查询无线音量
{
    qDebug().noquote() << QString("XdcController: 获取有线音量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Volume::GET_XDC_WIRED, QByteArray(), "getWiredVolume");
}

void XdcController::setWiredVolume(uint8_t index)   // 0x0106, 设置有线音量
{
    uint8_t maxVolume = m_volumeSettings.value("wiredMaxVolume", 7).toUInt();
    if (index >= maxVolume) {
        qWarning() << "有线音量索引超出范围，最大值:" << maxVolume - 1;
        emit operationResult(m_hostAddress, "setWiredVolume", false, "音量索引超出范围");
        return;
    }

    qDebug().noquote() << QString("XdcController: 设置有线音量，地址：%1，索引：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(index);
    QByteArray payload;
    payload.append(static_cast<char>(index));
    sendCommand(0x01, Command::Volume::SET_XDC_WIRED, payload, "setWiredVolume");
}

void XdcController::handleWiredVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame)  // 0x02/0x04, 0x0206, 处理有线音量响应
{
    if (frame.payload.size() < 2) {
        qWarning() << "有线音量响应数据长度不足";
        return;
    }

    uint8_t currentIndex = static_cast<uint8_t>(frame.payload.at(0));
    uint8_t maxCount = static_cast<uint8_t>(frame.payload.at(1));

    m_volumeSettings["wiredVolume"] = currentIndex;
    m_volumeSettings["wiredMaxVolume"] = maxCount;

    emit volumeSettingsChanged();

    qDebug().noquote() << QString("XdcController：有线音量：当前索引=%1，最大档位=%2")
                              .arg(currentIndex).arg(maxCount);
}

void XdcController::getAntennaVolume()  // 0x0207, 查询天线盒音量
{
    qDebug().noquote() << QString("XdcController: 获取天线盒音量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Volume::GET_XDC_ANTENNA, QByteArray(), "getAntennaVolume");
}

void XdcController::setAntennaVolume(uint8_t index) // 0x0107, 设置天线盒音量
{
    uint8_t maxVolume = m_volumeSettings.value("antennaMaxVolume", 9).toUInt();
    if (index >= maxVolume) {
        qWarning() << "天线盒音量索引超出范围，最大值:" << maxVolume - 1;
        emit operationResult(m_hostAddress, "setAntennaVolume", false, "音量索引超出范围");
        return;
    }

    qDebug().noquote() << QString("XdcController: 设置天线盒音量，地址：%1，索引：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(index);
    QByteArray payload;
    payload.append(static_cast<char>(index));
    sendCommand(0x01, Command::Volume::SET_XDC_ANTENNA, payload, "setAntennaVolume");
}

void XdcController::handleAntennaVolumeResponse(const ProtocolProcessor::ParsedFrameData& frame)    // 0x02/0x04, 0x0207, 处理“天线盒音量”响应
{
    if (frame.payload.size() < 2) {
        qWarning() << "天线盒音量响应数据长度不足";
        return;
    }

    uint8_t currentIndex = static_cast<uint8_t>(frame.payload.at(0));
    uint8_t maxCount = static_cast<uint8_t>(frame.payload.at(1));

    m_volumeSettings["antennaVolume"] = currentIndex;
    m_volumeSettings["antennaMaxVolume"] = maxCount;

    emit volumeSettingsChanged();

    qDebug().noquote() << QString("XdcController：天线盒音量：当前索引=%1，最大档位=%2")
                              .arg(currentIndex).arg(maxCount);
}

// ==================== 分组3：发言相关功能 ====================
void XdcController::getSpeechMode() // 0x0208, 查询发言模式
{
    qDebug().noquote() << QString("XdcController: 获取发言模式，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Speech::GET_XDC_MODE, QByteArray(), "getSpeechMode");
}

void XdcController::setSpeechMode(uint8_t mode) // 0x0108, 设置发言模式
{
    qDebug().noquote() << QString("XdcController: 设置发言模式，地址：%1，模式：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(mode);
    QByteArray payload;
    payload.append(static_cast<char>(mode));
    sendCommand(0x01, Command::Speech::SET_XDC_MODE, payload, "setSpeechMode");
}

void XdcController::handleSpeechModeResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x0208, 处理“发言模式”响应
{
    if (frame.payload.isEmpty()) return;

    uint8_t mode = static_cast<uint8_t>(frame.payload.at(0));
    m_speechSettings["speechMode"] = mode;

    emit speechSettingsChanged();

    QString modeStr = (mode == 0) ? "FIFO" : "SelfLock";
    qDebug().noquote() << QString("XdcController：发言模式：%1").arg(modeStr);
}

void XdcController::getMaxSpeechCount()     // 0x0209, 查询最大同时发言数量，须小于等于硬件允许数量（DS236为4个）
{
    qDebug().noquote() << QString("XdcController: 获取最大发言数量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Speech::GET_XDC_MAX_COUNT, QByteArray(), "getMaxSpeechCount");
}

void XdcController::setMaxSpeechCount(uint8_t count)    // 0x0109, 设置最大同时发言数量量，须小于等于硬件允许数量（DS236为4个）
{
    uint8_t maxCount = m_speechSettings.value("hardwareMaxSpeech", 4).toUInt();
    if (count < 1 || count > maxCount) {
        qWarning() << "发言数量超出范围，有效范围: 1-" << maxCount;
        emit operationResult(m_hostAddress, "setMaxSpeechCount", false, "发言数量超出范围");
        return;
    }

    qDebug().noquote() << QString("XdcController: 设置最大发言数量，地址：%1，数量：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(count);
    QByteArray payload;
    payload.append(static_cast<char>(count));
    sendCommand(0x01, Command::Speech::SET_XDC_MAX_COUNT, payload, "setMaxSpeechCount");
}

void XdcController::handleMaxSpeechCountResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x0209, 处理“最大同时发言数量”响应
{
    if (frame.payload.size() < 2) {
        qWarning() << "最大发言数量响应数据长度不足";
        return;
    }

    uint8_t currentCount = static_cast<uint8_t>(frame.payload.at(0));
    uint8_t maxCount = static_cast<uint8_t>(frame.payload.at(1));

    m_speechSettings["maxSpeechCount"] = currentCount;
    m_speechSettings["hardwareMaxSpeech"] = maxCount;

    emit speechSettingsChanged();

    qDebug().noquote() << QString("XdcController：最大发言数量：当前=%1，硬件最大=%2")
                              .arg(currentCount).arg(maxCount);
}

void XdcController::getMaxUnitCapacity()        // 0x0210, 查询最大可注册单元容量（不可设置）
{
    qDebug().noquote() << QString("XdcController: 获取最大注册单元数量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Unit::GET_XDC_MAX_REGISTER, QByteArray(), "getMaxUnitCapacity");
}

void XdcController::handleMaxUnitCapacityResponse(const ProtocolProcessor::ParsedFrameData& frame)  // 0x0210, 处理“最大可注册单元容量”响应
{
    // 协议：返回2个字节，表示最大注册单元数量
    if (frame.payload.size() < 2) {
        qWarning() << "XdcController: 0x0210 响应数据长度不足，期望>=2，实际：" << frame.payload.size();
        return;
    }

    uint16_t maxUnitCount = (static_cast<uint8_t>(frame.payload.at(0)) << 8) |
                            static_cast<uint8_t>(frame.payload.at(1));

    m_speechSettings["maxUnitCapacity"] = maxUnitCount;    // 将最大单元容量值存入结构变量
    m_maxUnitId = maxUnitCount; // 同时赋值给最大单元Id值， 用于单元模型处理
    qDebug() << QString("maxUnitCapacity = ").arg(m_maxUnitId);

    emit speechSettingsChanged();
}

void XdcController::getCurrentWirelessSpeakerId()   // 0x020E, 查询“当前正在发言的无线单元ID”
{
    qDebug().noquote() << QString("XdcController: 获取当前无线发言ID，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
     sendCommand(0x01, Command::Unit::GET_XDC_CURRENT_WIRELESS, QByteArray(), "getCurrentWirelessSpeakerId");
}

void XdcController::getCurrentWiredSpeakerId()      // 0x020F, 查询“当前正在发言的有线单元ID”
{
    qDebug().noquote() << QString("XdcController: 获取当前有线发言ID，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Unit::GET_XDC_CURRENT_WIRED, QByteArray(), "getCurrentWiredSpeakerId");
}

void XdcController::getWirelessAudioPath()  // 0x020D, 查询“音频输出路径”
{
    qDebug().noquote() << QString("XdcController: 获取无线音频路径，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::AudioPath::GET_XDC_PATH, QByteArray(), "getWirelessAudioPath");
}

void XdcController::setWirelessAudioPath(uint8_t path)  // 0x010D, 设置“音频输出路径”
{
    qDebug().noquote() << QString("XdcController: 设置无线音频路径，地址：%1，路径：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(path);
    QByteArray payload;
    payload.append(static_cast<char>(path));
    sendCommand(0x01, Command::AudioPath::SET_XDC_PATH, payload, "setWirelessAudioPath");
}

void XdcController::handleWirelessAudioPathResponse(const ProtocolProcessor::ParsedFrameData& frame)    // 0x02/0x04, 0x020D, 处理“音频输出路径”响应
{
    if (frame.payload.isEmpty()) return;

    uint8_t path = static_cast<uint8_t>(frame.payload.at(0));
    m_speechSettings["wirelessAudioPath"] = path;

    emit speechSettingsChanged();

    QString pathStr = (path == 0) ? "主机" : "天线盒";
    qDebug().noquote() << QString("XdcController：无线音频路径：%1").arg(pathStr);
}

// ==================== 分组4：摄像跟踪相关功能 ====================
// 0x020A, 查询“摄像机协议”
void XdcController::getCameraProtocol()
{
    qDebug().noquote() << QString("XdcController: 查询摄像跟踪协议 getCameraProtocol()");
    sendCommand(0x01, Command::Camera::GET_XDC_PROTOCOL, QByteArray(), "getCameraProtocol");
}

void XdcController::setCameraProtocol(uint8_t protocol) // 0x010A, 设置“摄像机协议”
{
    qDebug().noquote() << QString("XdcController: 设置摄像跟踪协议，地址：%1，协议：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(protocol);
    QByteArray payload;
    payload.append(static_cast<char>(protocol));
    sendCommand(0x01, Command::Camera::SET_XDC_PROTOCOL, payload, "setCameraProtocol");
}

void XdcController::handleCameraProtocolResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x020A, 处理“摄像机协议”响应
{
    if (frame.payload.isEmpty()) return;

    uint8_t protocol = static_cast<uint8_t>(frame.payload.at(0));
    m_cameraSettings["protocol"] = protocol;

    emit cameraSettingsChanged();

    QString protocolStr;
    switch (protocol) {
    case 0: protocolStr = "None"; break;
    case 1: protocolStr = "VISCA"; break;
    case 2: protocolStr = "PELCO_D"; break;
    case 3: protocolStr = "PELCO_P"; break;
    default: protocolStr = "Unknown";
    }
    qDebug().noquote() << QString("XdcController：摄像跟踪协议：%1").arg(protocolStr);
}

void XdcController::getCameraAddress()  // 0x020B, 查询“摄像机地址”
{
    qDebug().noquote() << QString("XdcController: 获取摄像跟踪地址，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Camera::GET_XDC_ADDRESS, QByteArray(), "getCameraAddress");
}

void XdcController::setCameraAddress(uint8_t address)   // 0x010A, 设置“摄像机地址”
{
    qDebug().noquote() << QString("XdcController: 设置摄像跟踪地址，地址：%1，摄像机地址：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(address);
    QByteArray payload;
    payload.append(static_cast<char>(address));
    sendCommand(0x01, Command::Camera::SET_XDC_ADDRESS, payload, "setCameraAddress");
}

void XdcController::handleCameraAddressResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x020B, 处理“摄像机地址”响应
{
    if (frame.payload.isEmpty()) return;

    uint8_t address = static_cast<uint8_t>(frame.payload.at(0));
    m_cameraSettings["address"] = address;

    emit cameraSettingsChanged();

    qDebug().noquote() << QString("XdcController：摄像跟踪地址：%1").arg(address);
}

void XdcController::getCameraBaudrate()     // 0x020C, 查询“摄像机波特率”
{
    qDebug().noquote() << QString("XdcController: 获取摄像跟踪波特率，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::Camera::GET_XDC_BAUDRATE, QByteArray(), "getCameraBaudrate");
}

void XdcController::setCameraBaudrate(uint8_t baudrate) // 0x010C, 设置“摄像机波特率”
{
    qDebug().noquote() << QString("XdcController: 设置摄像跟踪波特率，地址：%1，波特率：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(baudrate);
    QByteArray payload;
    payload.append(static_cast<char>(baudrate));
    sendCommand(0x01, Command::Camera::SET_XDC_BAUDRATE, payload, "setCameraBaudrate");
}

void XdcController::handleCameraBaudrateResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x020C, 处理“摄像机波特率”响应
{
    if (frame.payload.isEmpty()) return;

    uint8_t baudrate = static_cast<uint8_t>(frame.payload.at(0));
    m_cameraSettings["baudrate"] = baudrate;

    emit cameraSettingsChanged();

    QString baudrateStr;
    switch (baudrate) {
    case 0: baudrateStr = "2400bps"; break;
    case 1: baudrateStr = "4800bps"; break;
    case 2: baudrateStr = "9600bps"; break;
    case 3: baudrateStr = "115200bps"; break;
    default: baudrateStr = "Unknown";
    }
    qDebug().noquote() << QString("XdcController：摄像跟踪波特率：%1").arg(baudrateStr);
}

// ==================== 分组7：系统时间相关功能 ====================
void XdcController::getSystemDateTime()     // 0x0204, 查询“系统时间”
{
    qDebug().noquote() << QString("XdcController: 获取系统时间，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
    sendCommand(0x01, Command::SystemTime::GET_XDC_DATETIME, QByteArray(), "getSystemDateTime");
}

void XdcController::setSystemDateTime(uint32_t utcTime) // 0x0204, 设置“系统时间”
{
    qDebug().noquote() << QString("XdcController: 设置系统时间，地址：%1，UTC时间戳：%2")
                              .arg(formatAddressHex(m_hostAddress)).arg(utcTime);
    QByteArray payload;
    // UTC时间戳，大端模式
    payload.append(static_cast<char>((utcTime >> 24) & 0xFF));
    payload.append(static_cast<char>((utcTime >> 16) & 0xFF));
    payload.append(static_cast<char>((utcTime >> 8) & 0xFF));
    payload.append(static_cast<char>(utcTime & 0xFF));
    sendCommand(0x01, Command::SystemTime::SET_XDC_DATETIME, payload, "setSystemDateTime");
}

void XdcController::handleSystemDateTimeResponse(const ProtocolProcessor::ParsedFrameData& frame)   // 0x02/0x04, 0x0204, 处理“系统时间”响应
{
    if (frame.payload.size() < 4) {
        qWarning() << "系统时间响应数据长度不足";
        return;
    }

    uint32_t utcTime = (static_cast<uint8_t>(frame.payload.at(0)) << 24) |
                       (static_cast<uint8_t>(frame.payload.at(1)) << 16) |
                       (static_cast<uint8_t>(frame.payload.at(2)) << 8) |
                       static_cast<uint8_t>(frame.payload.at(3));

    m_systemDateTime = QDateTime::fromSecsSinceEpoch(utcTime);
    emit systemDateTimeUpdated(m_hostAddress, m_systemDateTime);

    qDebug().noquote() << QString("XdcController：系统时间：%1")
                              .arg(m_systemDateTime.toString("yyyy-MM-dd hh:mm:ss"));
}

// ==================== 分组8：音频通道信息相关功能 ====================
/* XDC236主机有一个音频监测通道，四个无线输出音频通道。
 * 监测音频音频监测通道的设置为硬件内置通道，为单通道，包括20个（0x0211查询获得）内置工作频率（0x0212查询获得）
 * 无线输出音频通道有四个输出通道，可以指定每个通道的工作频率。本测试功能即测试这四个无线音频输出通道，在指定频率（音频监测通道的频率，或者完全手工指定频率）下的信号强度
 * 0x0901可设置一个无线音频通道的工作频率，0x0404可以读取四个无线音频通道的信号强度（但是只有对应通道号的值有效）。测试时要设置一个通道频率，等待稳定后读取一个信号强度。
 */
// 第一部分：公共接口实现 ----------------------------------------
// 启动完整音频测试（一个函数入口，三步流程自动执行）
void XdcController::startAudioTest()
{
    if (m_isAudioTesting) {
        qWarning().noquote() << QString("XdcController: 音频测试正在进行中，地址：%1")
                                    .arg(formatAddressHex(m_hostAddress));
        return;
    }

    qDebug().noquote() << QString("XdcController: 开始音频测试，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));

    resetAudioTestState();
    m_isAudioTesting = true;
    m_audioTestStep = 1;

    emit audioTestStepChanged(stepToString(m_audioTestStep));
    emit audioTestProgress(1, 0, 0, "正在获取监测信道数量...");

    // 第一步：获取监测音频信道数量
    getMonitorChannelCount();
}

void XdcController::stopAudioTest()
{
    if (!m_isAudioTesting) return;

    qDebug().noquote() << QString("XdcController: 停止音频测试，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));

    finishAudioTest(false, "用户停止测试");
}

void XdcController::getMonitorChannelCount()    // 0x0211, 查询音频监测通道的内置频点个数
{
    qDebug().noquote() << QString("XdcController: 发送 0x0211 获取监测信道数量，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));

    sendCommand(0x01, Command::Frequency::GET_XDC_MONITOR_COUNT, QByteArray(), "getMonitorChannelCount");
}

void XdcController::getAllMonitorChannels()     // 0x0212, 查询音频监测通道的内置频率及信号强度
{
    if (m_freqPointCount <= 0) {
        qWarning() << "XdcController: 频率点数量无效，请先调用 getMonitorChannelCount()";
        return;
    }

    qDebug().noquote() << QString("XdcController: 开始批量获取监测信道信息，数量：%1，地址：%2")
                              .arg(m_freqPointCount)
                              .arg(formatAddressHex(m_hostAddress));

    // 初始化统一数据数组
    m_freqPoints.clear();
    m_freqPoints.resize(m_freqPointCount);
    for (int i = 0; i < m_freqPointCount; ++i) {
        m_freqPoints[i].fid = i;
    }

    m_currentMonitorIndex = 0;
    requestNextMonitorChannel();
}

void XdcController::startWirelessChannelTest()
{
    if (m_freqPoints.isEmpty()) {
        qWarning() << "XdcController: 频率点数据为空，请先调用 getAllMonitorChannels()";
        finishAudioTest(false, "频率点数据为空");
        return;
    }

    qDebug().noquote() << QString("XdcController: 开始无线信道测试，频率点数：%1，地址：%2")
                              .arg(m_freqPoints.size())
                              .arg(formatAddressHex(m_hostAddress));

    m_currentFid = 0;
    m_currentCid = 0;
    m_waitingForRSSI = false;
    m_frequencySetPending = false;

    int totalSteps = m_freqPoints.size() * 4;
    emit audioTestProgress(3, 0, totalSteps,
                           QString("开始测试无线信道 (共 %1 个频率点 × 4 个信道)")
                               .arg(m_freqPoints.size()));

    setNextWirelessFrequency();
}

void XdcController::setStabilizeDelayMs(int delayMs)
{
    if (delayMs >= 100 && delayMs <= 10000) {
        m_stabilizeDelayMs = delayMs;
        qDebug() << "XdcController: 信号稳定延时设置为:" << m_stabilizeDelayMs << "ms";
    } else {
        qWarning() << "XdcController: 延时参数超出范围(100-10000ms):" << delayMs;
    }
}

int XdcController::getStabilizeDelayMs() const
{
    return m_stabilizeDelayMs;
}

QString XdcController::getAudioTestStep() const
{
    return stepToString(m_audioTestStep);
}

QVariantList XdcController::getAllFreqPoints() const
{
    QVariantList result;
    for (const auto& point : m_freqPoints) {
        result.append(point.toVariantMap());
    }
    return result;
}

QVariantMap XdcController::getFreqPoint(int fid) const
{
    if (fid >= 0 && fid < m_freqPoints.size()) {
        return m_freqPoints[fid].toVariantMap();
    }
    return QVariantMap();
}

QVariantList XdcController::getUnifiedTableModel() const
{
    return m_unifiedTableModel;
}

// 第二部分：响应处理函数实现 ----------------------------------------
void XdcController::handleMonitorChannelCountResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    if (frame.payload.isEmpty()) {
        qWarning() << "XdcController: 0x0211 响应负载为空";
        if (m_isAudioTesting) {
            finishAudioTest(false, "获取信道数量失败：响应负载为空");
        }
        return;
    }

    m_freqPointCount = static_cast<quint8>(frame.payload.at(0));

    qDebug().noquote() << QString("XdcController: 监测信道数量：%1")
                              .arg(m_freqPointCount);

    emit monitorChannelCountReceived(m_freqPointCount);
    emit freqPointCountChanged();

    if (m_isAudioTesting && m_audioTestStep == 1) {
        if (m_freqPointCount > 0) {
            m_audioTestStep = 2;
            emit audioTestStepChanged(stepToString(m_audioTestStep));
            emit audioTestProgress(2, 0, m_freqPointCount,
                                   QString("正在获取 %1 个监测信道信息...")
                                       .arg(m_freqPointCount));

            // 自动开始获取监测信道信息
            getAllMonitorChannels();
        } else {
            finishAudioTest(false, "监测信道数量为0");
        }
    }
}

void XdcController::handleMonitorChannelResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    // 响应格式：fid(1) + freq(4) + rssi(1)
    if (frame.payload.size() < 6) {
        qWarning() << "XdcController: 0x0212 响应负载长度不足，实际："
                   << frame.payload.size() << "，期望：>=6";
        return;
    }

    quint8 fid = static_cast<quint8>(frame.payload.at(0));

    // 解析频率（大端序，4字节）
    quint32 frequency = (static_cast<quint32>(static_cast<quint8>(frame.payload.at(1))) << 24) |
                        (static_cast<quint32>(static_cast<quint8>(frame.payload.at(2))) << 16) |
                        (static_cast<quint32>(static_cast<quint8>(frame.payload.at(3))) << 8) |
                        static_cast<quint32>(static_cast<quint8>(frame.payload.at(4)));

    // 解析 RSSI（有符号字节）
    qint8 rssi = static_cast<qint8>(frame.payload.at(5));

    qDebug().noquote() << QString("XdcController: 监测信道 %1 - 频率: %2 Hz, RSSI: %3 dBm")
                              .arg(fid).arg(frequency).arg(rssi);

    // 存储到统一数据结构
    if (fid < static_cast<quint8>(m_freqPoints.size())) {
        m_freqPoints[fid].fid = fid;
        m_freqPoints[fid].monitorFreq = frequency;
        m_freqPoints[fid].monitorRSSI = rssi;

        // 初始化无线信道频率为监测频率（默认值）
        for (int cid = 0; cid < 4; ++cid) {
            m_freqPoints[fid].wireless[cid].frequency = frequency;
        }
    }

    emit freqPointUpdated(fid, frequency, rssi);

    // 每个监测信道响应后更新表格
    updateUnifiedTableModel();

    if (m_isAudioTesting && m_audioTestStep == 2) {
        m_currentMonitorIndex++;
        emit audioTestProgress(2, m_currentMonitorIndex, m_freqPointCount,
                               QString("已获取 %1/%2 个监测信道")
                                   .arg(m_currentMonitorIndex)
                                   .arg(m_freqPointCount));

        // 延时 20ms 后继续下一个请求
        QTimer::singleShot(20, this, [this]() {
            requestNextMonitorChannel();
        });
    }
}

void XdcController::handleSetFrequencyResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    if (!m_frequencySetPending) return;

    m_frequencySetPending = false;

    // 解析响应，获取实际设置的频率
    if (frame.payload.size() >= 8) {
        quint8 cid = static_cast<quint8>(frame.payload.at(2));
        quint8 fid = static_cast<quint8>(frame.payload.at(3));

        // 解析设置的频率（大端序，4字节）
        quint32 freq = (static_cast<quint32>(static_cast<quint8>(frame.payload.at(4))) << 24) |
                       (static_cast<quint32>(static_cast<quint8>(frame.payload.at(5))) << 16) |
                       (static_cast<quint32>(static_cast<quint8>(frame.payload.at(6))) << 8) |
                       static_cast<quint32>(static_cast<quint8>(frame.payload.at(7)));

        qDebug().noquote() << QString("XdcController: 频率设置成功 - FID:%1, CID:%2, 频率:%3 Hz")
                                  .arg(fid).arg(cid).arg(freq);

        // 存储无线信道的工作频率
        if (fid < m_freqPoints.size() && cid >= 1 && cid <= 4) {
            int index = cid - 1;  // 转换为 0-3
            m_freqPoints[fid].setWirelessFreq(index, freq);
        }
    }

    // 等待信号稳定后读取 RSSI
    QTimer::singleShot(m_stabilizeDelayMs, this, [this]() {
        if (m_isAudioTesting && !m_waitingForRSSI) {
            readWirelessRSSI();
        }
    });
}

void XdcController::handleReadRSSIResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    if (!m_waitingForRSSI) return;

    m_waitingForRSSI = false;

    // 响应格式：count(1) + rssi[4](4)
    if (frame.payload.size() < 5) {
        qWarning() << "XdcController: 0x0404 响应负载长度不足，实际："
                   << frame.payload.size() << "，期望：>=5";
        advanceToNextTest();
        return;
    }

    quint8 count = static_cast<quint8>(frame.payload.at(0));

    qDebug().noquote() << QString("XdcController: 收到 RSSI 响应，通道数：%1").arg(count);

    // 查找有效的 RSSI 值（非零）
    qint8 validRSSI = 0;
    int validCid = -1;

    for (int i = 0; i < qMin(static_cast<int>(count), 4); ++i) {
        qint8 rssi = static_cast<qint8>(frame.payload.at(1 + i));
        if (rssi != 0) {
            validRSSI = rssi;
            validCid = i;
            qDebug().noquote() << QString("  信道 %1: RSSI = %2 dBm (有效)").arg(i).arg(rssi);
            break;  // 只有一个有效值
        } else {
            qDebug().noquote() << QString("  信道 %1: RSSI = 0 (无效)").arg(i);
        }
    }

    // 存储有效数据（只有 rssi != 0 才存储）
    // 注意：validCid 是数组索引 0-3，对应 CID 值 1-4
    if (m_currentFid < m_freqPoints.size() && validCid >= 0 && validCid < 4 && validRSSI != 0) {
        m_freqPoints[m_currentFid].setWirelessRSSI(validCid, validRSSI);

        // 发射信号时，将数组索引转换为 CID 值（+1）
        int newCidValue = validCid + 1;
        emit wirelessChannelUpdated(m_currentFid, newCidValue,
                                    m_freqPoints[m_currentFid].wireless[validCid].frequency,
                                    validRSSI);

        qDebug().noquote() << QString("XdcController: 存储无线数据 - FID:%1, CID:%2, RSSI:%3 dBm")
                                  .arg(m_currentFid).arg(newCidValue).arg(validRSSI);
    } else {
        qDebug().noquote() << QString("XdcController: 未找到有效RSSI - FID:%1, CID:%2")
                                  .arg(m_currentFid).arg(m_currentCid);
    }

    // 每个 RSSI 响应后更新表格
    updateUnifiedTableModel();

    // 推进到下一个测试
    advanceToNextTest();
}

// 第三部分：私有方法实现 ----------------------------------------
void XdcController::resetAudioTestState()
{
    m_freqPointCount = 0;
    m_freqPoints.clear();
    m_currentMonitorIndex = 0;
    m_currentFid = 0;
    m_currentCid = 0;
    m_waitingForRSSI = false;
    m_frequencySetPending = false;
}

void XdcController::requestNextMonitorChannel()     // 0x0212 查询音频监测通道频率及信号强度
{
    if (m_currentMonitorIndex >= m_freqPointCount) {
        qDebug().noquote() << QString("XdcController: 监测信道信息获取完成，共 %1 个")
                                  .arg(m_freqPointCount);

        if (m_isAudioTesting && m_audioTestStep == 2) {
            m_audioTestStep = 3;
            emit audioTestStepChanged(stepToString(m_audioTestStep));
            emit audioTestProgress(2, m_freqPointCount, m_freqPointCount,
                                   "监测信道信息获取完成，开始测试无线信道");
            startWirelessChannelTest();
        }
        return;
    }

    qDebug().noquote() << QString("XdcController: 请求监测信道 %1/%2")
                              .arg(m_currentMonitorIndex)
                              .arg(m_freqPointCount - 1);

    QByteArray payload;
    payload.append(static_cast<char>(m_currentMonitorIndex));

    sendCommand(0x01, Command::Frequency::GET_XDC_MONITOR_INFO, payload,
                QString("monitor_%1").arg(m_currentMonitorIndex));
}

void XdcController::setNextWirelessFrequency()
{
    // Fid边界校验
    if (m_currentFid >= m_freqPoints.size()) {
        finishAudioTest(true, "音频测试完成");
        return;
    }
    // Cid边界校验
    if (m_currentCid < 1 || m_currentCid > 4) {
        qWarning() << "无效的 CID 值:" << m_currentCid;
        advanceToNextTest();
        return;
    }

    int totalSteps = m_freqPoints.size() * 4;
    int currentStep = m_currentFid * 4 + m_currentCid;

    emit audioTestProgress(3, currentStep, totalSteps,
                           QString("测试中: 频率点 %1/%2, 信道 %3/4")
                               .arg(m_currentFid + 1)
                               .arg(m_freqPoints.size())
                               .arg(m_currentCid + 1));

    const auto& point = m_freqPoints[m_currentFid];

    qDebug().noquote() << QString("XdcController: 设置无线信道 - FID:%1, 频率:%2 Hz, CID:%3")
                              .arg(m_currentFid)
                              .arg(point.monitorFreq)
                              .arg(m_currentCid);

    // 构建 0x0901 指令参数
    QByteArray payload;

    // UID: 固定为 0x0001 (2字节)
    payload.append(static_cast<char>(0x00));
    payload.append(static_cast<char>(0x01));

    // CID: 无线信道号 (1-4)
    payload.append(static_cast<char>(m_currentCid));

    // FID: 频率序号（使用监测信道频率）
    payload.append(static_cast<char>(m_currentFid));

    // FREQ: 自定义频率 (4字节，大端序)
    quint32 freq = point.monitorFreq;
    payload.append(static_cast<char>((freq >> 24) & 0xFF));
    payload.append(static_cast<char>((freq >> 16) & 0xFF));
    payload.append(static_cast<char>((freq >> 8) & 0xFF));
    payload.append(static_cast<char>(freq & 0xFF));

    sendCommand(0x01, Command::Frequency::SET_XDC_FREQ, payload,
                QString("%1fid%2_cid%3").arg(KeyPrefix::SET_FREQ).arg(m_currentFid).arg(m_currentCid));

    m_frequencySetPending = true;
}

void XdcController::readWirelessRSSI()
{
    qDebug().noquote() << QString("XdcController: 读取无线信道 RSSI - FID:%1, CID:%2")
                              .arg(m_currentFid)
                              .arg(m_currentCid);

    sendCommand(0x01, Command::Frequency::GET_XDC_RSSI, QByteArray(),
                QString("%1fid%2_cid%3").arg(KeyPrefix::READ_RSSI).arg(m_currentFid).arg(m_currentCid));

    m_waitingForRSSI = true;
}

void XdcController::advanceToNextTest()
{
    // 移动到下一个无线信道
    m_currentCid++;

    if (m_currentCid > 4) {
        // 当前频率点的4个信道都测试完成，移动到下一个频率点
        m_currentCid = 1;
        m_currentFid++;
    }

    // 继续下一个测试
    setNextWirelessFrequency();
}

void XdcController::finishAudioTest(bool success, const QString& message)
{
    if (!m_isAudioTesting) return;

    m_isAudioTesting = false;
    int oldStep = m_audioTestStep;
    m_audioTestStep = 0;

    if (oldStep != 0) {
        emit audioTestStepChanged(stepToString(0));
    }

    // 测试完成后最终更新表格
    if (success) {
        updateUnifiedTableModel();
    }

    emit audioTestCompleted(success, message);

    qDebug().noquote() << QString("XdcController: 音频测试结束 - %1，地址：%2")
                              .arg(message, formatAddressHex(m_hostAddress));
}

QString XdcController::stepToString(int step) const
{
    switch (step) {
    case 0: return "idle";
    case 1: return "getting_channel_count";
    case 2: return "getting_monitor_channels";
    case 3: return "testing_wireless_channels";
    default: return "unknown";
    }
}

// 第四部分：表格模型更新 ----------------------------------------
void XdcController::updateUnifiedTableModel()
{
    if (m_freqPoints.isEmpty()) {
        m_unifiedTableModel.clear();
        emit unifiedTableModelChanged();
        return;
    }

    // 11行：FID, 监测频率, 监测RSSI,
    //       无线0频率, 无线0RSSI, 无线1频率, 无线1RSSI, 无线2频率, 无线2RSSI, 无线3频率, 无线3RSSI
    QStringList rowNames;
    rowNames << "FID"
             << "监测频率(KHz)"
             << "监测RSSI(dBm)";

    for (int cid = 0; cid < 4; ++cid) {
        rowNames << QString("无线%1频率(KHz)").arg(cid);
        rowNames << QString("无线%1RSSI(dBm)").arg(cid);
    }

    // 初始化行数据
    QList<QList<QString>> rowData;
    for (int i = 0; i < rowNames.size(); ++i) {
        rowData.append(QList<QString>());
    }

    // 遍历所有频率点
    const auto& freqPointsRef = m_freqPoints;
    for (const auto& point : freqPointsRef) {
        // 第0行：FID
        rowData[0].append(QString::number(point.fid));

        // 第1行：监测频率
        rowData[1].append(point.monitorFreqKHzStr());

        // 第2行：监测RSSI
        rowData[2].append(QString::number(point.monitorRSSI));

        // 第3-10行：4个无线信道
        for (int cid = 0; cid < 4; ++cid) {
            // 频率行（3 + cid*2）
            QString freqStr = point.wireless[cid].freqKHzStr();
            rowData[3 + cid * 2].append(freqStr.isEmpty() ? "--" : freqStr);

            // RSSI行（4 + cid*2）
            if (point.wireless[cid].rssi == 0) {
                rowData[4 + cid * 2].append("--");
            } else {
                rowData[4 + cid * 2].append(QString::number(point.wireless[cid].rssi));
            }
        }
    }

    // 转换为 QVariantList
    QVariantList model;
    for (int i = 0; i < rowNames.size(); ++i) {
        QVariantMap row;
        row["rowName"] = rowNames[i];
        row["values"] = QVariant::fromValue(rowData[i]);
        model.append(row);
    }

    m_unifiedTableModel = model;
    emit unifiedTableModelChanged();

    qDebug().noquote() << QString("XdcController: 更新统一表格模型，行数：%1，列数：%2")
                              .arg(rowNames.size())
                              .arg(m_freqPoints.size());
}

// ================== 单元管理 =====================
// 初始化连接 UnitModel 信号 ----------------------------
void XdcController::connectUnitModelSignals()
{
    if (!m_unitModel) {
        qWarning() << "XdcController: UnitModel 为空，无法连接信号";
        return;
    }

    // 连接单元操作请求信号
    connect(m_unitModel, &UnitModel::requestOpenUnit,
            this, &XdcController::onUnitModelRequestOpenUnit);
    connect(m_unitModel, &UnitModel::requestCloseUnit,
            this, &XdcController::onUnitModelRequestCloseUnit);
    connect(m_unitModel, &UnitModel::requestDeleteUnit,
            this, &XdcController::onUnitModelRequestDeleteUnit);
    connect(m_unitModel, &UnitModel::requestSetIdentity,
            this, &XdcController::onUnitModelRequestSetIdentity);
    connect(m_unitModel, &UnitModel::requestSetAlias,
            this, &XdcController::onUnitModelRequestSetAlias);
    connect(m_unitModel, &UnitModel::requestRefreshWirelessState,
            this, &XdcController::onUnitModelRequestRefreshWirelessState);

    qDebug().noquote() << QString("XdcController: UnitModel 信号连接完成，地址：%1")
                              .arg(formatAddressHex(m_hostAddress));
}

void XdcController::getAllUnits()
{
    qDebug().noquote() << QString("XdcController: 查询所有连线单元，地址：0 - %1").arg(m_maxUnitId);

    // 如果正在查询，先强制停止并清除旧批次的请求
    if (m_isQueryingAllUnits) {
        qDebug() << "XdcController: ⚠️ 检测到上一次查询未完成，强制停止并重新开始";
        m_isQueryingAllUnits = false;
        
        // 清除上一批次的所有单元查询请求
        if (m_protocolProcessor && !m_currentBatchId.isEmpty()) {
            int removedCount = m_protocolProcessor->clearCommandsByUserDataPrefix(m_currentBatchId);
            if (removedCount > 0) {
                qDebug().noquote() << QString("XdcController: 清除旧批次请求 [%1]，数量: %2")
                                          .arg(m_currentBatchId).arg(removedCount);
            }
        }
    }

    // 生成新的批次 ID（使用时间戳确保唯一性）
    m_currentBatchId = QString("%1%2").arg(KeyPrefix::UNIT_QUERY_BATCH).arg(QDateTime::currentMSecsSinceEpoch());
    
    m_isQueryingAllUnits = true;
    m_currentQueryUnitId = 0x0001;
    
    if (m_unitModel) {
        m_unitModel->clearAll();
        qDebug() << "XdcController: 已清空单元模型";
    }
    
    qDebug().noquote() << QString("XdcController: 开始新一轮单元查询 [批次: %1]，起始ID=0x0001")
                              .arg(m_currentBatchId);
    requestNextUnitInfo();
}

void XdcController::requestNextUnitInfo()
{
    if (!m_isQueryingAllUnits) {
        qDebug().noquote() << QString("XdcController: requestNextUnitInfo 被中断（m_isQueryingAllUnits=false），当前ID=0x%1")
                                  .arg(m_currentQueryUnitId, 4, 16, QChar('0'));
        return;
    }

    uint16_t maxQueryId = (m_maxUnitId != 0x00FF) ? m_maxUnitId : 36;

    qDebug().noquote() << QString("XdcController: 准备查询单元 ID=0x%1 (当前上限=0x%2, m_maxUnitId=%3)")
                              .arg(m_currentQueryUnitId, 4, 16, QChar('0'))
                              .arg(maxQueryId, 4, 16, QChar('0'))
                              .arg(m_maxUnitId);

    if (m_currentQueryUnitId > maxQueryId) {
        m_isQueryingAllUnits = false;
        int count = m_unitModel ? m_unitModel->rowCount() : 0;
        emit unitListSyncCompleted(count);
        qDebug().noquote() << QString("XdcController: 单元遍历完成 [批次: %1]")
                                  .arg(m_currentBatchId)
                           << "共查询到" << count << "个单元"
                           << "m_maxUnitId=" << m_maxUnitId
                           << "maxQueryId=" << maxQueryId
                           << "当前ID=0x" << QString::number(m_currentQueryUnitId, 16).toUpper();
        m_currentBatchId.clear();
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>((m_currentQueryUnitId >> 8) & 0xFF));
    payload.append(static_cast<char>(m_currentQueryUnitId & 0xFF));

    // 使用批次 ID + 前缀 + 单元ID 作为 userData
    QString unitIdHex = QString("%1").arg(m_currentQueryUnitId, 4, 16, QChar('0')).toUpper();
    QString userData = QString("%1%2%3")
                           .arg(m_currentBatchId,
                                KeyPrefix::UNIT_INFO,
                                unitIdHex);


    qDebug().noquote() << QString("XdcController: 发送查询指令 - 单元ID: 0x%1 [批次: %2]")
                              .arg(m_currentQueryUnitId, 4, 16, QChar('0'))
                              .arg(m_currentBatchId);

    sendCommand(0x01, Command::Unit::GET_XDC_INFO, payload, userData);

    m_currentQueryUnitId++;

    if ((m_currentQueryUnitId - 0x0001) % 10 == 0) {
        qDebug().noquote() << QString("XdcController: ⏸流量控制 - 已查询到 0x%1，延时300ms后继续")
                                  .arg(m_currentQueryUnitId - 1, 4, 16, QChar('0'));
        QTimer::singleShot(300, this, [this]() {
            requestNextUnitInfo();
        });
        return;
    }

    QTimer::singleShot(50, this, [this]() {
        requestNextUnitInfo();
    });
}

void XdcController::refreshSpeakingStatus()
{
    getCurrentWirelessSpeakerId();
    getCurrentWiredSpeakerId();
}

// 单元操作请求槽函数 --------------------------
void XdcController::onUnitModelRequestOpenUnit(uint16_t unitId)
{
    qDebug().noquote() << QString("XdcController: 执行打开单元指令 - 地址：%1，单元ID：0x%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'));

    QByteArray payload;
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));
    payload.append(static_cast<char>(0x00));  // 保留
    payload.append(static_cast<char>(0x00));  // 保留

    qDebug().noquote() << QString("XdcController: 执行打开单元指令 - 地址：%1，单元ID：0x%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(payload);

    sendCommand(0x01, Command::Unit::SET_XDC_OPEN, payload,
                QString("%1%2").arg(KeyPrefix::OPEN_UNIT).arg(unitId, 4, 16, QChar('0')));
}

void XdcController::onUnitModelRequestCloseUnit(uint16_t unitId)
{
    qDebug().noquote() << QString("XdcController: 执行关闭单元指令 - 地址：%1，单元ID：0x%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'));

    QByteArray payload;
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));
    payload.append(static_cast<char>(0x00));
    payload.append(static_cast<char>(0x00));

    sendCommand(0x01, Command::Unit::SET_XDC_CLOSE, payload,
                QString("%1%2").arg(KeyPrefix::CLOSE_UNIT).arg(unitId, 4, 16, QChar('0')));
}

void XdcController::onUnitModelRequestDeleteUnit(uint16_t unitId)
{
    qDebug().noquote() << QString("XdcController: 执行删除单元指令 - 地址：%1，单元ID：0x%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'));

    QByteArray payload;
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));
    payload.append(static_cast<char>(0x00));
    payload.append(static_cast<char>(0x00));

    sendCommand(0x01, Command::Unit::SET_XDC_DELETE, payload,
                QString("%1%2").arg(KeyPrefix::DELETE_UNIT).arg(unitId, 4, 16, QChar('0')));
}

void XdcController::onUnitModelRequestSetIdentity(uint16_t unitId, uint8_t identity)
{
    qDebug().noquote() << QString("XdcController: 执行设置单元身份指令 - 地址：%1，单元ID：0x%2，身份：0x%3")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(identity, 2, 16, QChar('0'));

    // 先获取单元当前信息，然后修改身份字段后回写
    // 注意：0x0501 设置单元信息需要完整的单元数据
    // 协议格式：unitId(2) + physicalAddr(6) + identity(1) + type(1) + reserved(2)

    // 先从 UnitModel 获取当前单元信息
    QVariantMap unitInfo = m_unitModel->getUnitInfo(unitId);
    if (unitInfo.isEmpty()) {
        qWarning() << "XdcController: 单元不存在，无法设置身份 - ID:" << unitId;
        emit operationResult(m_hostAddress, "setIdentity", false, "单元不存在");
        return;
    }

    QByteArray payload;
    // 单元ID (2字节，大端序)
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));

    // 物理地址 (6字节)
    QString physAddr = unitInfo["physicalAddr"].toString();
    QStringList addrParts = physAddr.split(':');
    for (int i = 0; i < 6; i++) {
        if (i < addrParts.size()) {
            payload.append(static_cast<char>(addrParts[i].toUInt(nullptr, 16)));
        } else {
            payload.append(static_cast<char>(0x00));
        }
    }

    // 身份 (1字节)
    payload.append(static_cast<char>(identity));

    // 单元类型 (1字节) - 保持原类型不变
    int type = unitInfo["type"].toInt();
    payload.append(static_cast<char>(type));

    // 保留字段 (2字节)
    payload.append(static_cast<char>(0x00));
    payload.append(static_cast<char>(0x00));

    sendCommand(0x01, Command::Unit::SET_XDC_IDENTITY, payload,
                QString("%1%2").arg(KeyPrefix::SET_IDENTITY).arg(unitId, 4, 16, QChar('0')));
}

void XdcController::onUnitModelRequestSetAlias(uint16_t unitId, const QString& alias)
{
    qDebug().noquote() << QString("XdcController: 执行设置单元别名指令 - 地址：%1，单元ID：0x%2，别名：%3")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(alias);

    QByteArray payload;
    // 单元ID (2字节，大端序)
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));

    // 单元别名 (16字节，不足补'\0')
    QByteArray aliasData = alias.toUtf8();
    for (int i = 0; i < 16; i++) {
        if (i < aliasData.size()) {
            payload.append(aliasData.at(i));
        } else {
            payload.append(static_cast<char>(0x00));
        }
    }

    sendCommand(0x01, Command::Unit::SET_XDC_ALIAS, payload,
                QString("%1%2").arg(KeyPrefix::SET_ALIAS).arg(unitId, 4, 16, QChar('0')));
}

void XdcController::onUnitModelRequestRefreshWirelessState(uint16_t unitId)
{
    qDebug().noquote() << QString("XdcController: 执行刷新无线单元状态指令 - 地址：%1，单元ID：0x%2")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'));

    QByteArray payload;
    // 单元ID (2字节，大端序)
    payload.append(static_cast<char>((unitId >> 8) & 0xFF));
    payload.append(static_cast<char>(unitId & 0xFF));
    // 保留字段 (2字节)
    payload.append(static_cast<char>(0x00));
    payload.append(static_cast<char>(0x00));

    sendCommand(0x01, Command::Unit::SET_XDC_REFRESH_WIRELESS, payload,
                QString("%1%2").arg(KeyPrefix::REFRESH_WIRELESS).arg(unitId, 4, 16, QChar('0')));
}

// 单元响应处理函数 ----------------------------
void XdcController::handleUnitInfoResponse(const ProtocolProcessor::ParsedFrameData& frame)     // 处理 getAllUnit()响应
{
    // 0x0601 响应格式：55 0C F3 01 80 01 06 01 00 01 86 AA --> 55 14 EB 80 01 04 06 01 (00 01) (F8 D4 83 B9) (30) (22) (00) (00） 87 AA
    // unitId(2) + physicalAddr(4) + identity(1) + type(1) + reserved(2)

    if (frame.payload.size() < 10) {
        qWarning() << "XdcController: 单元信息响应数据长度不足" << frame.payload.size();
        return;
    }

    // 解析单元ID (大端序)
    uint16_t unitId = (static_cast<uint8_t>(frame.payload[0]) << 8) |
                      static_cast<uint8_t>(frame.payload[1]);

    // 1. 检查单元ID是否有效（非零、非全FF）
    if (unitId == 0x0000 || unitId == 0xFFFF) {
        qDebug().noquote() << QString("XdcController: ⚠收到无效单元ID响应 - ID: 0x%1，忽略")
                                  .arg(unitId, 4, 16, QChar('0'));
        return;
    }

    // 解析物理地址 (4字节)，从 payload[2] 开始
    QString physicalAddr = QString("%1:%2:%3:%4")
                               .arg(static_cast<uint8_t>(frame.payload[2]), 2, 16, QChar('0'))
                               .arg(static_cast<uint8_t>(frame.payload[3]), 2, 16, QChar('0'))
                               .arg(static_cast<uint8_t>(frame.payload[4]), 2, 16, QChar('0'))
                               .arg(static_cast<uint8_t>(frame.payload[5]), 2, 16, QChar('0')).toUpper();

    // 2. 检查物理地址是否有效（非全零、非全FF）
    if (physicalAddr == "00:00:00:00" || physicalAddr == "FF:FF:FF:FF") {
        qDebug().noquote() << QString("XdcController: ⚠️ 收到无效物理地址响应 - ID: 0x%1, 地址: %2，忽略")
                                  .arg(unitId, 4, 16, QChar('0'))
                                  .arg(physicalAddr);
        return;
    }

    // 解析身份和类型
    UnitIdentity identity = static_cast<UnitIdentity>(static_cast<uint8_t>(frame.payload[6]));      // UnitModel.h已经设置全局枚举类型，此类已经包含该头文件
    UnitType type = static_cast<UnitType>(static_cast<uint8_t>(frame.payload[7]));                  // UnitModel.h已经设置全局枚举类型，此类已经包含该头文件

    // 3：检查类型和身份是否有效
    if (type == UnitType::Unknown || identity == UnitIdentity::Unknown) {
        qDebug().noquote() << QString("XdcController: 收到无效类型/身份响应 - ID: 0x%1, 类型:%2, 身份:%3，忽略")
                                  .arg(QString::asprintf("0x%04X", unitId),
                                       typeDisplayName(type),
                                       identityDisplayName(identity));
        return;
    }

    qDebug().noquote() << QString("XdcController: 收到单元信息响应 - 地址：%1, 单元ID：0x%2, 类型:%3, 身份:%4")
                              .arg(formatAddressHex(m_hostAddress),
                                   QString::asprintf("0x%04X", unitId),
                                   typeDisplayName(type),
                                   identityDisplayName(identity));

    if (m_unitModel) {
        m_unitModel->updateUnitInfo(unitId, physicalAddr, identity, type);
    }
}

void XdcController::handleUnitAliasResponse(const ProtocolProcessor::ParsedFrameData& frame)    // 0x0606, 查询单元别名
{
    // 0x0602 响应格式：
    // unitId(2) + alias(16)

    if (frame.payload.size() < 18) {
        qWarning() << "XdcController: 单元别名响应数据长度不足" << frame.payload.size();
        return;
    }

    // 解析单元ID (大端序)
    uint16_t unitId = (static_cast<uint8_t>(frame.payload[0]) << 8) |
                      static_cast<uint8_t>(frame.payload[1]);

    // 解析别名 (16字节，去除末尾的'\0')
    QByteArray aliasData = frame.payload.mid(2, 16);
    QString alias = trimNullTerminator(aliasData);

    qDebug().noquote() << QString("XdcController: 收到单元别名响应 - 地址：%1，单元ID：%2，别名：%3")
                              .arg(formatAddressHex(m_hostAddress), formatAddressHex(unitId), alias);

    if (m_unitModel) {
        m_unitModel->updateUnitAlias(unitId, alias);
    }
}

void XdcController::handleWirelessStateResponse(const ProtocolProcessor::ParsedFrameData& frame)    // 0x0606, 查询无线单元信息（电压，电量，充电状态，充电指示
{
    // 0x0606 响应格式：55 0C F3 01 80 01 06 06 00 01 81 AA -->
    // unitId(2) + batteryVoltage(2) + batteryPercent(1) + chargingStatus(1) + rssi(1) + reserved(2)

    if (frame.payload.size() < 9) {
        qWarning() << "XdcController: 无线单元状态响应数据长度不足" << frame.payload.size();
        return;
    }

    // 解析单元ID (大端序)
    uint16_t unitId = (static_cast<uint8_t>(frame.payload[0]) << 8) |
                      static_cast<uint8_t>(frame.payload[1]);

    // 解析电池电压 (大端序，2字节)，单位：mV
    uint16_t batteryVoltage = (static_cast<uint8_t>(frame.payload[2]) << 8) |
                              static_cast<uint8_t>(frame.payload[3]);

    // 解析电池百分比 (0-100)
    uint8_t batteryPercent = static_cast<uint8_t>(frame.payload[4]);

    // 解析充电状态
    ChargingStatus chargingStatus = static_cast<ChargingStatus>(static_cast<uint8_t>(frame.payload[5]));

    // 解析信号强度 (有符号字节，dBm)
    int8_t rssi = static_cast<int8_t>(frame.payload[6]);

    qDebug().noquote() << QString("XdcController: 收到无线单元状态响应 - 地址：%1, 单元ID：0x%2, 电压:%3mV, 电量:%4%%, 充电:%5, RSSI:%6dBm")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(unitId, 4, 16, QChar('0'))
                              .arg(batteryVoltage)
                              .arg(batteryPercent)
                              .arg(chargingStatusDisplayName(chargingStatus))
                              .arg(rssi);

    if (m_unitModel) {
        m_unitModel->updateWirelessState(unitId, batteryVoltage, batteryPercent,
                                         chargingStatus, rssi);
    }
}

void XdcController::handleUnitOperationResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    // 处理 0x0503/0x0504/0x0505 的响应
    // 这些命令的响应通常是原样返回，表示操作成功

    uint16_t command = frame.command;

    // 尝试解析单元ID（如果有）
    uint16_t unitId = 0;
    if (frame.payload.size() >= 2) {
        unitId = (static_cast<uint8_t>(frame.payload[0]) << 8) |
                 static_cast<uint8_t>(frame.payload[1]);
    }

    QString cmdName;
    switch (command) {
    case 0x0501: cmdName = "setUnitInfo"; break;
    case 0x0502: cmdName = "setUnitAlias"; break;
    case 0x0503: cmdName = "deleteUnit"; break;
    case 0x0504: cmdName = "openUnit"; break;
    case 0x0505: cmdName = "closeUnit"; break;
    default: cmdName = "unknown"; break;
    }

    qDebug().noquote() << QString("XdcController: 收到单元操作响应 - 地址：%1，命令：0x%2(%3)，单元ID：0x%4")
                              .arg(formatAddressHex(m_hostAddress))
                              .arg(command, 4, 16, QChar('0'))
                              .arg(cmdName)
                              .arg(unitId, 4, 16, QChar('0'));

    // 根据命令类型更新 UnitModel 中的状态
    if (m_unitModel && unitId != 0) {
        switch (command) {
        case 0x0504:  // 打开单元成功
            m_unitModel->setUnitOpened(unitId, true);
            break;
        case 0x0505:  // 关闭单元成功
            m_unitModel->setUnitOpened(unitId, false);
            m_unitModel->setUnitSpeaking(unitId, false);
            break;
        case 0x0503:  // 删除单元成功
            m_unitModel->removeUnit(unitId);
            break;
        default:
            break;
        }
    }

    // 发射操作结果信号
    emit operationResult(m_hostAddress, cmdName, true,
                         QString("操作成功，单元ID：0x%1").arg(unitId, 4, 16, QChar('0')));
}

void XdcController::handleUnitCurrentSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData& frame)
{
    // 命令码 0x020E（无线发言ID）或 0x020F（有线发言ID）
    // 0x020E: 55 0A F5 01 80 01 02 0E 8C AA  --> 55 12 ED 80 01 04 02 0E FF FF 00 02 FF FF FF FF 8B AA
    // 0x020F: 55 0A F5 01 80 01 02 0F 8D AA  --> 55 12 ED 80 01 04 02 0F 00 01 FF FF FF FF FF FF 89 AA

    bool isWireless = (frame.command == 0x020E);

    if (frame.payload.size() < 8) {
        qWarning() << "XdcController: 发言ID响应数据长度不足，期望>=8，实际：" << frame.payload.size();
        return;
    }

    // 解析4个发言单元ID，保留每个通道的原始值
    QList<uint16_t> channelIds;
    QSet<uint16_t> validIds;

    for (int i = 0; i < 4; ++i) {
        uint16_t speakerId = (static_cast<uint8_t>(frame.payload[i * 2]) << 8) |
                             static_cast<uint8_t>(frame.payload[i * 2 + 1]);
        channelIds.append(speakerId);

        // 0x0000 和 0xFFFF 都视为无效
        if (speakerId != 0x0000 && speakerId != 0xFFFF) {
            validIds.insert(speakerId);
        }
    }

    // 更新 m_speechSettings，保存四个通道的完整列表
    if (isWireless) {
        // 保存四个通道的ID列表（保持顺序）
        m_speechSettings["wirelessChannelIds"] = QVariant::fromValue(channelIds);
        // 向后兼容：保存第一个有效ID
        if (!validIds.isEmpty()) {
            m_speechSettings["currentWirelessSpeakerId"] = *validIds.begin();
        } else {
            m_speechSettings["currentWirelessSpeakerId"] = 0;
        }
        m_speechSettings["wirelessSpeakingIds"] = QVariant::fromValue(validIds.values());
    } else {
        // 保存四个通道的ID列表（保持顺序）
        m_speechSettings["wiredChannelIds"] = QVariant::fromValue(channelIds);
        // 向后兼容：保存第一个有效ID
        if (!validIds.isEmpty()) {
            m_speechSettings["currentWiredSpeakerId"] = *validIds.begin();
        } else {
            m_speechSettings["currentWiredSpeakerId"] = 0;
        }
        m_speechSettings["wiredSpeakingIds"] = QVariant::fromValue(validIds.values());
    }

    emit speechSettingsChanged();

    // 更新 UnitModel 中的发言状态
    if (m_unitModel) {
        QList<uint16_t> allUnitIds = m_unitModel->getUnitIds();
        for (const uint16_t unitId : allUnitIds) {
            QVariantMap info = m_unitModel->getUnitInfo(unitId);
            bool isWirelessUnit = info["isWireless"].toBool();
            if (isWirelessUnit == isWireless) {
                m_unitModel->setUnitSpeaking(unitId, validIds.contains(unitId));
            }
        }
    }

    // 格式化输出四个通道的状态
    QStringList channelStr;
    for (int i = 0; i < channelIds.size(); ++i) {
        uint16_t id = channelIds[i];
        if (id == 0x0000 || id == 0xFFFF) {
            channelStr << "None";
        } else {
            channelStr << QString("0x%1").arg(id, 4, 16, QChar('0'));
        }
    }

    qDebug().noquote() << QString("XdcController：当前%1发言单元 - 通道状态：[%2]")
                              .arg(isWireless ? "无线" : "有线", channelStr.join(", "));
}
