#include "HostController.h"
#include "HostManager.h"
#include "ProtocolProcessor.h"
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

// ==================== 构造函数 ====================

HostController::HostController(uint8_t hostAddress, HostManager* parent)
    : QObject(parent)
    , m_hostAddress(hostAddress)
    , m_hostManager(parent)
    , m_currentWirelessSpeakerId(0)
    , m_currentWiredSpeakerId(0)
    , m_unitCapacity(0)
    , m_monitorAudioChannels(0)
{
    qDebug() << "HostController: 创建 - 地址:" << QString("0x%1").arg(m_hostAddress, 2, 16, QChar('0')).toUpper();
}

HostController::~HostController()
{
    qDebug() << "HostController: 销毁 - 地址:" << QString("0x%1").arg(m_hostAddress, 2, 16, QChar('0')).toUpper();
}

// ==================== 发送指令 ====================

void HostController::sendToHost(const QByteArray &data)
{
    if (m_hostManager) {
        m_hostManager->sendRawData(data);
    }
}

void HostController::sendCommandInternal(uint8_t opCode, uint16_t command, const QByteArray &payload)
{
    if (!m_hostManager) {
        qWarning() << "HostManager 为空";
        return;
    }

    QByteArray frame = ProtocolProcessor::buildFrame(
        m_hostAddress,
        0x80,
        opCode,
        command,
        payload
        );

    sendToHost(frame);
}

void HostController::queryHostInfo(const HostQueryInfo& queryInfo)
{
    sendCommandInternal(queryInfo.opCode, queryInfo.command, queryInfo.payload);
}

void HostController::getBasicProperties()
{
    sendCommandInternal(0x01, 0x0202, QByteArray(1, '\x00'));
    sendCommandInternal(0x01, 0x0202, QByteArray(1, '\x01'));
    sendCommandInternal(0x01, 0x0202, QByteArray(1, '\x02'));
    sendCommandInternal(0x01, 0x0203, QByteArray(1, '\x00'));
    sendCommandInternal(0x01, 0x0203, QByteArray(1, '\x01'));
    sendCommandInternal(0x01, 0x0203, QByteArray(1, '\x02'));
    sendCommandInternal(0x01, 0x0204);
    sendCommandInternal(0x01, 0x0205);
    sendCommandInternal(0x01, 0x0206);
    sendCommandInternal(0x01, 0x0207);
    sendCommandInternal(0x01, 0x0208);
    sendCommandInternal(0x01, 0x0209);
    sendCommandInternal(0x01, 0x020A);
    sendCommandInternal(0x01, 0x020B);
    sendCommandInternal(0x01, 0x020C);
    sendCommandInternal(0x01, 0x020D);
    sendCommandInternal(0x01, 0x0210);
    sendCommandInternal(0x01, 0x0211);
}

void HostController::requestMonitorChannelInfo()
{
    m_audioChannelData.clear();
    int maxChannel = m_monitorAudioChannels > 0 ? m_monitorAudioChannels - 1 : 19;

    for (int i = 0; i <= maxChannel; ++i) {
        sendCommandInternal(0x01, 0x0212, QByteArray(1, static_cast<char>(i)));
    }
}

void HostController::requestOutputChannelInfo()
{
    sendCommandInternal(0x01, 0x0404);
}

void HostController::detectingRegisteredUnits(quint32 max_UID)
{
    for (quint32 uid = 0x0001; uid <= max_UID; ++uid) {
        QByteArray payload(2, 0);
        payload[0] = static_cast<char>((uid >> 8) & 0xFF);
        payload[1] = static_cast<char>(uid & 0xFF);
        sendCommandInternal(0x01, 0x0601, payload);
    }
}

// ==================== 设置功能 ====================

void HostController::setSystemDatetime(const QDateTime& dt)
{
    if (!dt.isValid()) return;

    QDateTime baseDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), QTimeZone::utc());
    quint32 timestamp = baseDateTime.secsTo(dt);

    QByteArray payload(4, 0);
    payload[0] = static_cast<char>((timestamp >> 24) & 0xFF);
    payload[1] = static_cast<char>((timestamp >> 16) & 0xFF);
    payload[2] = static_cast<char>((timestamp >> 8) & 0xFF);
    payload[3] = static_cast<char>(timestamp & 0xFF);

    sendCommandInternal(0x01, 0x0104, payload);
}

void HostController::setSpeechMode(bool fifoMode)
{
    sendCommandInternal(0x01, 0x0108, QByteArray(1, static_cast<char>(fifoMode ? 0x00 : 0x01)));
}

void HostController::setMaxSpeechUnits(quint8 count)
{
    sendCommandInternal(0x01, 0x0109, QByteArray(1, static_cast<char>(count)));
}

void HostController::setVolume(int type, quint8 value)
{
    uint16_t cmd = 0x0105 + type;
    sendCommandInternal(0x01, cmd, QByteArray(1, static_cast<char>(value)));
}

void HostController::setWirelessAudioPath(bool antennaBox)
{
    sendCommandInternal(0x01, 0x010D, QByteArray(1, static_cast<char>(antennaBox ? 0x01 : 0x00)));
}

void HostController::setCameraConfig(const QString& protocol, quint16 address, quint32 baudrate)
{
    QByteArray protoPayload(1, 0);
    if (protocol == "VISCA") protoPayload[0] = 0x01;
    else if (protocol == "PELCO_D") protoPayload[0] = 0x02;
    else if (protocol == "PELCO_P") protoPayload[0] = 0x03;
    sendCommandInternal(0x01, 0x010A, protoPayload);

    sendCommandInternal(0x01, 0x010B, QByteArray(1, static_cast<char>(address)));

    QByteArray baudPayload(1, 0);
    if (baudrate == 4800) baudPayload[0] = 0x01;
    else if (baudrate == 9600) baudPayload[0] = 0x02;
    else if (baudrate == 115200) baudPayload[0] = 0x03;
    sendCommandInternal(0x01, 0x010C, baudPayload);
}

void HostController::removeUnit(quint32 uid)
{
    QByteArray payload(2, 0);
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);
    payload[1] = static_cast<char>(uid & 0xFF);
    sendCommandInternal(0x01, 0x0503, payload);
}

void HostController::enableUnit(quint32 uid)
{
    QByteArray payload(2, 0);
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);
    payload[1] = static_cast<char>(uid & 0xFF);
    sendCommandInternal(0x01, 0x0504, payload);
}

void HostController::disableUnit(quint32 uid)
{
    QByteArray payload(2, 0);
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);
    payload[1] = static_cast<char>(uid & 0xFF);
    sendCommandInternal(0x01, 0x0505, payload);
}

void HostController::setUnitInfo(quint32 uid, quint32 uAddr, quint8 role,
                                 quint8 type, quint8 status, quint8 response)
{
    QByteArray payload(10, 0);
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);
    payload[1] = static_cast<char>(uid & 0xFF);
    payload[2] = static_cast<char>((uAddr >> 24) & 0xFF);
    payload[3] = static_cast<char>((uAddr >> 16) & 0xFF);
    payload[4] = static_cast<char>((uAddr >> 8) & 0xFF);
    payload[5] = static_cast<char>(uAddr & 0xFF);
    payload[6] = static_cast<char>(role);
    payload[7] = static_cast<char>(type);
    payload[8] = static_cast<char>(status);
    payload[9] = static_cast<char>(response);

    sendCommandInternal(0x01, 0x0501, payload);
}

// ==================== 接收数据处理 ====================

void HostController::handleIncomingFrame(const ProtocolProcessor::ParsedFrameData &frame)
{
    switch (frame.command) {
    case 0x0202: handleVersionResponse(frame); break;
    case 0x0203: handleBuildDateResponse(frame); break;
    case 0x0204: handleDatetimeResponse(frame); break;
    case 0x0205: case 0x0206: case 0x0207: handleVolumeResponse(frame); break;
    case 0x0208: handleSpeechModeResponse(frame); break;
    case 0x0209: handleMaxSpeechUnitsResponse(frame); break;
    case 0x020A: handleCameraProtocolResponse(frame); break;
    case 0x020B: handleCameraAddressResponse(frame); break;
    case 0x020C: handleCameraBaudrateResponse(frame); break;
    case 0x020D: handleAudioPathResponse(frame); break;
    case 0x020E: handleWirelessSpeakerIdResponse(frame); break;
    case 0x020F: handleWiredSpeakerIdResponse(frame); break;
    case 0x0210: handleUnitCapacityResponse(frame); break;
    case 0x0211: handleMonitorChannelsResponse(frame); break;
    case 0x0212: handleMonitorChannelInfoResponse(frame); break;
    default:
        qWarning() << "未知命令:" << QString::number(frame.command, 16);
    }
}

void HostController::handleVersionResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (frame.payload.isEmpty()) return;

    quint8 subCmd = static_cast<quint8>(frame.payload.at(0));
    QString version = QString::fromUtf8(frame.payload.mid(1));

    QString key;
    if (subCmd == 0x00) key = "kernel";
    else if (subCmd == 0x01) key = "link";
    else if (subCmd == 0x02) key = "anta";

    if (!key.isEmpty()) {
        m_versionInfo[key] = version;
        emit versionUpdated(key, version);
    }
}

void HostController::handleBuildDateResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (frame.payload.isEmpty()) return;

    quint8 subCmd = static_cast<quint8>(frame.payload.at(0));
    QString date = QString::fromUtf8(frame.payload.mid(1));

    QString key;
    if (subCmd == 0x00) key = "kernelDate";
    else if (subCmd == 0x01) key = "linkDate";
    else if (subCmd == 0x02) key = "antaDate";

    if (!key.isEmpty()) {
        m_versionInfo[key] = date;
        emit versionUpdated(key, date);
    }
}

void HostController::handleDatetimeResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (frame.payload.size() >= 4) {
        quint32 timestamp = static_cast<quint8>(frame.payload.at(0)) << 24 |
                            static_cast<quint8>(frame.payload.at(1)) << 16 |
                            static_cast<quint8>(frame.payload.at(2)) << 8 |
                            static_cast<quint8>(frame.payload.at(3));

        QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::utc());
        m_systemSettings["datetime"] = dt.toString("yyyy-MM-dd HH:mm:ss");
        emit settingsUpdated("datetime", m_systemSettings["datetime"]);
    }
}

void HostController::handleVolumeResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (frame.payload.isEmpty()) return;

    int type = frame.command - 0x0205;
    quint8 value = static_cast<quint8>(frame.payload.at(0));

    QString key;
    if (type == 0) key = "wireless";
    else if (type == 1) key = "wired";
    else if (type == 2) key = "antaBox";

    m_volumeSettings[key] = value;
    emit volumeUpdated(type, value);
}

void HostController::handleSpeechModeResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        bool fifo = static_cast<quint8>(frame.payload.at(0)) == 0x00;
        m_systemSettings["speechMode"] = fifo ? "FIFO" : "Self Lock";
        emit settingsUpdated("speechMode", m_systemSettings["speechMode"]);
    }
}

void HostController::handleMaxSpeechUnitsResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        quint8 count = static_cast<quint8>(frame.payload.at(0));
        m_systemSettings["maxSpeechUnits"] = count;
        emit settingsUpdated("maxSpeechUnits", count);
    }
}

void HostController::handleCameraProtocolResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        quint8 proto = static_cast<quint8>(frame.payload.at(0));
        QString str;
        if (proto == 0x00) str = "NONE";
        else if (proto == 0x01) str = "VISCA";
        else if (proto == 0x02) str = "PELCO_D";
        else if (proto == 0x03) str = "PELCO_P";

        m_systemSettings["cameraProtocol"] = str;
        emit settingsUpdated("cameraProtocol", str);
    }
}

void HostController::handleCameraAddressResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        quint16 addr = static_cast<quint8>(frame.payload.at(0));
        m_systemSettings["cameraAddress"] = addr;
        emit settingsUpdated("cameraAddress", addr);
    }
}

void HostController::handleCameraBaudrateResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        quint8 baud = static_cast<quint8>(frame.payload.at(0));
        QString str;
        if (baud == 0x00) str = "2400";
        else if (baud == 0x01) str = "4800";
        else if (baud == 0x02) str = "9600";
        else if (baud == 0x03) str = "115200";

        m_systemSettings["cameraBaudrate"] = str;
        emit settingsUpdated("cameraBaudrate", str);
    }
}

void HostController::handleAudioPathResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        bool antennaBox = static_cast<quint8>(frame.payload.at(0)) == 0x01;
        m_volumeSettings["audioPath"] = antennaBox ? "Antenna Box" : "Host";
        emit volumeUpdated(-1, antennaBox ? 1 : 0);
    }
}

void HostController::handleWirelessSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        m_currentWirelessSpeakerId = static_cast<quint8>(frame.payload.at(0));
        emit settingsUpdated("wirelessSpeakerId", m_currentWirelessSpeakerId);
    }
}

void HostController::handleWiredSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        m_currentWiredSpeakerId = static_cast<quint8>(frame.payload.at(0));
        emit settingsUpdated("wiredSpeakerId", m_currentWiredSpeakerId);
    }
}

void HostController::handleUnitCapacityResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        m_unitCapacity = static_cast<quint8>(frame.payload.at(0));
        emit unitInfoUpdated();
    }
}

void HostController::handleMonitorChannelsResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!frame.payload.isEmpty()) {
        m_monitorAudioChannels = static_cast<quint8>(frame.payload.at(0));
        emit unitInfoUpdated();
    }
}

void HostController::handleMonitorChannelInfoResponse(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (frame.payload.size() >= 9) {
        AudioChannelInfo info;
        info.index = static_cast<quint8>(frame.payload.at(0));
        info.monitorFREQ = static_cast<quint8>(frame.payload.at(1)) << 24 |
                           static_cast<quint8>(frame.payload.at(2)) << 16 |
                           static_cast<quint8>(frame.payload.at(3)) << 8 |
                           static_cast<quint8>(frame.payload.at(4));
        info.monitorRSSI = static_cast<qint8>(frame.payload.at(5));

        m_audioChannelData.append(info);
        emit audioChannelsUpdated();
    }
}

// ==================== Getter 实现 ====================

QVariantMap HostController::getVersionInfo() const
{
    QVariantMap result;
    for (auto it = m_versionInfo.constBegin(); it != m_versionInfo.constEnd(); ++it) {
        result.insert(it.key(), QVariant(it.value()));
    }
    return result;
}

QVariantMap HostController::getUnitInfo() const
{
    QVariantMap result;
    result["capacity"] = m_unitCapacity;
    result["monitorChannels"] = m_monitorAudioChannels;
    result["wirelessSpeakerId"] = m_currentWirelessSpeakerId;
    result["wiredSpeakerId"] = m_currentWiredSpeakerId;
    return result;
}

QVariantList HostController::audioChannelsToVariantList() const
{
    QVariantList list;
    for (const auto& channel : m_audioChannelData) {
        QVariantMap map;
        map["index"] = channel.index;
        map["monitorFREQ"] = channel.monitorFREQ;
        map["monitorRSSI"] = channel.monitorRSSI;
        map["outputFREQ1"] = channel.outputFREQ[0];
        map["outputFREQ2"] = channel.outputFREQ[1];
        map["outputFREQ3"] = channel.outputFREQ[2];
        map["outputFREQ4"] = channel.outputFREQ[3];
        map["outputRSSI1"] = channel.outputRSSI[0];
        map["outputRSSI2"] = channel.outputRSSI[1];
        map["outputRSSI3"] = channel.outputRSSI[2];
        map["outputRSSI4"] = channel.outputRSSI[3];
        list.append(map);
    }
    return list;
}