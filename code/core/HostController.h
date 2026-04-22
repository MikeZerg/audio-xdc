#ifndef HOSTCONTROLLER_H
#define HOSTCONTROLLER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include <QMap>
#include <cstdint>

#include "ProtocolProcessor.h"

class HostManager;

class HostController : public QObject
{
    Q_OBJECT

    // ==================== QML 属性 ====================
    Q_PROPERTY(uint8_t address READ address CONSTANT FINAL)
    Q_PROPERTY(QVariantMap versionInfo READ getVersionInfo NOTIFY versionUpdated)
    Q_PROPERTY(QVariantMap systemSettings READ getSystemSettings NOTIFY settingsUpdated)
    Q_PROPERTY(QVariantMap volumeSettings READ getVolumeSettings NOTIFY volumeUpdated)
    Q_PROPERTY(QVariantList audioChannels READ getAudioChannels NOTIFY audioChannelsUpdated)
    Q_PROPERTY(QVariantMap unitInfo READ getUnitInfo NOTIFY unitInfoUpdated)

public:
    // ==================== 数据结构体 ====================
    struct HostQueryInfo {
        uint8_t opCode;
        uint16_t command;
        QByteArray payload;

        HostQueryInfo() : opCode(0x01), command(0), payload(QByteArray()) {}
        HostQueryInfo(uint8_t op, uint16_t cmd, const QByteArray& data = QByteArray())
            : opCode(op), command(cmd), payload(data) {}
    };

    struct AudioChannelInfo {
        quint8 index = 0;
        quint32 monitorFREQ = 0;
        double monitorRSSI = 0.0;
        quint32 outputFREQ[4] = {0, 0, 0, 0};
        double outputRSSI[4] = {0.0, 0.0, 0.0, 0.0};
    };

    // ==================== 构造函数 ====================
    explicit HostController(uint8_t hostAddress, HostManager* parent);
    virtual ~HostController();

    // ==================== Getter ====================
    uint8_t address() const { return m_hostAddress; }

    QVariantMap getVersionInfo() const;
    QVariantMap getSystemSettings() const { return m_systemSettings; }
    QVariantMap getVolumeSettings() const { return m_volumeSettings; }
    QVariantList getAudioChannels() const { return audioChannelsToVariantList(); }
    QVariantMap getUnitInfo() const;

    // ==================== 发送指令功能 ====================
    Q_INVOKABLE void queryHostInfo(const HostQueryInfo& queryInfo);
    Q_INVOKABLE void getBasicProperties();
    Q_INVOKABLE void requestMonitorChannelInfo();
    Q_INVOKABLE void requestOutputChannelInfo();
    Q_INVOKABLE void detectingRegisteredUnits(quint32 max_UID);

    // 设置功能
    Q_INVOKABLE void setSystemDatetime(const QDateTime& dt);
    Q_INVOKABLE void setSpeechMode(bool fifoMode);
    Q_INVOKABLE void setMaxSpeechUnits(quint8 count);
    Q_INVOKABLE void setVolume(int type, quint8 value);
    Q_INVOKABLE void setWirelessAudioPath(bool antennaBox);
    Q_INVOKABLE void setCameraConfig(const QString& protocol, quint16 address, quint32 baudrate);

    // 单元控制
    Q_INVOKABLE void removeUnit(quint32 uid);
    Q_INVOKABLE void enableUnit(quint32 uid);
    Q_INVOKABLE void disableUnit(quint32 uid);
    Q_INVOKABLE void setUnitInfo(quint32 uid, quint32 uAddr, quint8 role,
                                 quint8 type, quint8 status, quint8 response);

    // ==================== 接收数据处理 ====================
    void handleIncomingFrame(const ProtocolProcessor::ParsedFrameData &frame);

private:
    // 内部处理函数
    void handleVersionResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleBuildDateResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleDatetimeResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleVolumeResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleSpeechModeResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleMaxSpeechUnitsResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleCameraProtocolResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleCameraAddressResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleCameraBaudrateResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleAudioPathResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleWirelessSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleWiredSpeakerIdResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleUnitCapacityResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleMonitorChannelsResponse(const ProtocolProcessor::ParsedFrameData &frame);
    void handleMonitorChannelInfoResponse(const ProtocolProcessor::ParsedFrameData &frame);

signals:
    // ==================== 信号 ====================
    void versionUpdated(const QString& key, const QString& value);
    void settingsUpdated(const QString& key, const QVariant& value);
    void volumeUpdated(int type, quint8 value);
    void audioChannelsUpdated();
    void unitInfoUpdated();
    void commandDataReady(uint8_t address, const QByteArray &data);

private:
    // ==================== 私有方法 ====================
    uint8_t m_hostAddress;
    HostManager* m_hostManager;

    void sendToHost(const QByteArray &data);
    void sendCommandInternal(uint8_t opCode, uint16_t command, const QByteArray &payload = QByteArray());
    QVariantList audioChannelsToVariantList() const;

    // ==================== 成员变量 ====================
    QMap<QString, QString> m_versionInfo;              // 版本信息
    QVariantMap m_systemSettings;                       // 系统设置
    QVariantMap m_volumeSettings;                       // 音量设置
    QVector<AudioChannelInfo> m_audioChannelData;       // 音频通道数据
    quint8 m_currentWirelessSpeakerId;                  // 当前无线发言 ID
    quint8 m_currentWiredSpeakerId;                     // 当前有线发言 ID
    quint8 m_unitCapacity;                              // 单元容量
    quint8 m_monitorAudioChannels;                      // 监测通道数
};

#endif // HOSTCONTROLLER_H