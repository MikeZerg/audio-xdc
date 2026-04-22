#ifndef SERIALPORTADAPTER_H
#define SERIALPORTADAPTER_H

#include "ConnectionAdapter.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVariantList>
#include <QVariantMap>
#include <QByteArray>
#include <QMutex>

class SerialPortAdapter : public ConnectionAdapter
{
    Q_OBJECT

public:
    explicit SerialPortAdapter(QObject *parent = nullptr);
    ~SerialPortAdapter();

    // 具体实现函数
    Q_INVOKABLE QVariantList getAvailablePorts() const;     ///< 获取可用串口
    Q_INVOKABLE bool connectToPort(const QString &portName, qint32 baudRate, int dataBitsIndex, int parityIndex, int stopBitsIndex);    ///< 连接到串口
    Q_INVOKABLE bool disconnectFromPort();      ///< 从串口断开
    Q_INVOKABLE QVariantMap getConnectedPortInfo() const;       ///< 获取串口连接参数
    Q_INVOKABLE bool isPortConnected() const;       ///< 串口是否已连接
    Q_INVOKABLE bool sendToPort(const QByteArray &data);        ///< 向串口发送数据

    // 实现 ConnectionAdapter 接口
    Q_INVOKABLE bool connectToAdapter() override;       ///< 连接到接口
    Q_INVOKABLE void disconnectFromAdapter() override;      ///< 从接口断开
    Q_INVOKABLE bool isAdapterConnected() const override;       ///< 接口是否已连接
    Q_INVOKABLE bool forwardData(const QByteArray &data) override;      ///< 向接口发送数据

    Q_INVOKABLE QVariantList getAvailableAdapters() const override;     ///< 获取可用接口
    Q_INVOKABLE void setConnectionParameters(const QVariantMap &config) override;     ///< 接口配置函数
    Q_INVOKABLE QVariantMap getConnectionParameters() const override;       ///< 获取接口配置参数
    Q_INVOKABLE ConnectionAdapter::ConnectionType getConnectionType() const override;       ///< 获取接口类型

    // 统计接口已继承，不需要重复声明

signals:
    void portStatusChanged(bool isOpen, qint32 baudRate = 0, int dataBits = QSerialPort::Data8,
                           int parity = QSerialPort::NoParity, int stopBits = QSerialPort::OneStop);
    void rawDataReceived(const QByteArray &data);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    mutable QMutex m_mutex;
    QSerialPort *m_serialPort;
    QVariantMap m_serialConfig;
    QByteArray m_receivedBuffer;
    static const int MaxBufferSize = 128 * 1024;  // 128KB
};

#endif // SERIALPORTADAPTER_H
