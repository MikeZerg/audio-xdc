#ifndef BLUETOOTHADAPTER_H
#define BLUETOOTHADAPTER_H

#include "ConnectionAdapter.h"
#include <QMutex>
#include <QTimer>
#include <QVariantMap>

class BluetoothAdapter : public ConnectionAdapter
{
    Q_OBJECT

public:
    explicit BluetoothAdapter(QObject *parent = nullptr);
    ~BluetoothAdapter() override;

    // ===== 核心通信接口实现 =====
    bool connectToAdapter() override;
    void disconnectFromAdapter() override;
    bool isAdapterConnected() const override;
    bool forwardData(const QByteArray &data) override;

    // ===== 配置接口实现 =====
    QVariantList getAvailableAdapters() const override;
    void setConnectionParameters(const QVariantMap &config) override;
    QVariantMap getConnectionParameters() const override;
    ConnectionType getConnectionType() const override;

    // ===== 统计接口 =====
    void resetStatistics() override;

private slots:
    void onConnectionTimeout();

private:
    mutable QMutex m_mutex;
    QVariantMap m_bluetoothConfig;
    
    // 连接状态
    bool m_isConnected;
    
    // 统计信息
    qint64 m_sentByteCount;
    qint64 m_recvByteCount;
    
    // 连接超时定时器
    QTimer *m_connectionTimer;
};

#endif // BLUETOOTHADAPTER_H