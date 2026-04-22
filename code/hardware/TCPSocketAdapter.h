#ifndef TCPSOCKETADAPTER_H
#define TCPSOCKETADAPTER_H

#include "ConnectionAdapter.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QMutex>
#include <QByteArray>

class TCPSocketAdapter : public ConnectionAdapter
{
    Q_OBJECT

public:
    explicit TCPSocketAdapter(QObject *parent = nullptr);
    ~TCPSocketAdapter();

    Q_INVOKABLE QVariantList getAvailableHosts() const;
    Q_INVOKABLE QString getHostsIP() const;

    Q_INVOKABLE bool connectToHost(const QString &hostName, quint16 port);
    Q_INVOKABLE bool disconnectFromHost();
    Q_INVOKABLE bool isHostConnected() const;
    Q_INVOKABLE bool sendToHost(const QByteArray &data);
    Q_INVOKABLE QVariantMap getConnectedHostInfo() const;

    // 实现 ConnectionAdapter 接口
    Q_INVOKABLE bool connectToAdapter() override;
    Q_INVOKABLE void disconnectFromAdapter() override;
    Q_INVOKABLE bool isAdapterConnected() const override;
    Q_INVOKABLE bool forwardData(const QByteArray &data) override;

    Q_INVOKABLE QVariantList getAvailableAdapters() const override;
    Q_INVOKABLE void setConnectionParameters(const QVariantMap &config) override;
    Q_INVOKABLE QVariantMap getConnectionParameters() const override;
    Q_INVOKABLE ConnectionAdapter::ConnectionType getConnectionType() const override;

    // 统计接口已继承，不需要重复声明

signals:
    void hostStatusChanged(bool isOpen, QString hostName, quint16 port);
    void rawDataReceived(const QByteArray &data);

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleError(QAbstractSocket::SocketError socketError);
    void handleReadyRead();

private:
    mutable QMutex m_mutex;
    QTcpSocket *m_socket;
    QVariantMap m_tcpConfig;
    QByteArray m_receivedBuffer;
    static const int MaxBufferSize = 128 * 1024;  // 128KB
};

#endif // TCPSOCKETADAPTER_H
