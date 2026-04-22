#include <QNetworkInterface>
#include <QCoreApplication>
#include "TCPSocketAdapter.h"
#include "code/library/LogManager.h"

TCPSocketAdapter::TCPSocketAdapter(QObject *parent)
    : ConnectionAdapter(parent),
    m_socket(nullptr)
{
    m_socket = new QTcpSocket(this);
    m_tcpConfig = QVariantMap();

    // 连接信号和槽
    QObject::connect(m_socket, &QTcpSocket::connected, this, &TCPSocketAdapter::handleConnected);
    QObject::connect(m_socket, &QTcpSocket::disconnected, this, &TCPSocketAdapter::handleDisconnected);
    QObject::connect(m_socket, &QTcpSocket::errorOccurred, this, &TCPSocketAdapter::handleError);
    QObject::connect(m_socket, &QTcpSocket::readyRead, this, &TCPSocketAdapter::handleReadyRead);

    LOG_INFO("TCPSocketAdapter 构建完成，连接开始");
}

TCPSocketAdapter::~TCPSocketAdapter()
{
    LOG_INFO("TCPSocketAdapter 析构开始");

    if (m_socket) {
        auto socketState = m_socket->state();
        QString hostInfo = "未知连接";

        if (socketState == QAbstractSocket::ConnectedState) {
            QString hostName = m_socket->peerName();
            if (hostName.isEmpty()) {
                hostName = m_socket->peerAddress().toString();
            }
            hostInfo = QStringLiteral("%1:%2").arg(hostName, QString::number(m_socket->peerPort()));
        }

        // qDebug() << "TCP连接状态:" << socketState << "(" << hostInfo << ")";

        if (socketState == QAbstractSocket::ConnectedState) {
            m_socket->disconnectFromHost();

            // 简化断开逻辑，移除 processEvents
            if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                bool disconnected = m_socket->waitForDisconnected(1000);
                if (!disconnected) {
                    LOG_WARNING("TCP断开超时，强制关闭");
                    m_socket->abort();
                }
            }
        }

        // m_socket 是 QObject，父对象是 this，会自动删除
        m_socket = nullptr;  // 只需设置为 nullptr
    }

    m_receivedBuffer.clear();
    m_receivedBuffer.squeeze();
    m_tcpConfig.clear();

    LOG_INFO("TCPSocketAdapter 析构完成, 连接断开");
}

QVariantList TCPSocketAdapter::getAvailableHosts() const
{
    QVariantList serverList;

    // 1. 添加本地回环地址
    QVariantMap localhost;
    localhost["host"] = "127.0.0.1";
    localhost["port"] = 8080;
    localhost["description"] = "本地回环地址";
    serverList.append(localhost);

    // 2. 添加本机网络接口地址
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : hostAddresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback()) {
            QVariantMap localAddress;
            localAddress["host"] = address.toString();
            localAddress["port"] = 8080;
            localAddress["description"] = "本地网络接口";
            serverList.append(localAddress);
        }
    }

    // 3. 添加常见局域网服务器地址段
    QString localIP = getHostsIP();
    if (!localIP.isEmpty()) {
        QStringList ipParts = localIP.split('.');
        if (ipParts.size() >= 3) {
            QString networkSegment = ipParts[0] + "." + ipParts[1] + "." + ipParts[2] + ".";
            QVariantMap gateway;
            gateway["host"] = networkSegment + "1";
            gateway["port"] = 8080;
            gateway["description"] = "网关地址";
            serverList.append(gateway);
        }
    }

    return serverList;
}

QString TCPSocketAdapter::getHostsIP() const
{
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : hostAddresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback() &&
            address.toString().startsWith("192.168")) {
            return address.toString();
        }
    }

    for (const QHostAddress& address : hostAddresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback()) {
            return address.toString();
        }
    }

    return QString();
}

bool TCPSocketAdapter::connectToHost(const QString &hostName, quint16 port)
{
    if (hostName.isEmpty() || port == 0) {
        LOG_ERROR("严重错误: 主机名或端口无效");
        return false;
    }

    bool isConnectedOrConnecting = false;
    QString currentHostName;
    quint16 currentPort = 0;

    {
        QMutexLocker locker(&m_mutex);
        // 修改：判断是否已连接或正在连接
        isConnectedOrConnecting = (m_socket &&
                                   (m_socket->state() == QAbstractSocket::ConnectedState ||
                                    m_socket->state() == QAbstractSocket::ConnectingState));

        // 如果是已经连接或正在连接状态,获取连接hostName和Port
        if (isConnectedOrConnecting) {
            currentHostName = m_socket->peerName();
            if (currentHostName.isEmpty()) {
                currentHostName = m_socket->peerAddress().toString();
            }
            currentPort = m_socket->peerPort();
        }
    }

    // 首先判断m_socket是否连接或正在连接状态
    if (!isConnectedOrConnecting) {
        // 如果是非连接状态，则使用参数连接
        {
            QMutexLocker locker(&m_mutex);
            m_socket->connectToHost(hostName, port);

            if (!m_socket->waitForConnected(3000)) {
                QString errorMsg = m_socket->errorString();
                m_socket->abort();  // 连接失败后清理状态

                QString logMessage = QStringLiteral("连接TCP服务器(%1:%2)失败: %3")
                                         .arg(hostName, QString::number(port), errorMsg);
                LOG_WARNING(logMessage);
                emit errorOccurred(errorMsg);

                // 连接失败时发送hostStatusChanged信号
                emit hostStatusChanged(false, hostName, port);

                return false;
            }

            m_receivedBuffer.clear();

            // 连接成功时发送hostStatusChanged信号
            emit hostStatusChanged(true, hostName, port);

            return true;
        }
    } else {
        // 如果是已经连接或正在连接状态，则判断连接hostName和Port是否与需要连接相同
        if (currentHostName != hostName || currentPort != port) {
            // 如果不相同，则打印输出已经连接的hostName和Port, "连接失败,当前已经连接到hostName, port"
            QString currentHostInfo = QStringLiteral("%1:%2").arg(currentHostName, QString::number(currentPort));
            QString targetHostInfo = QStringLiteral("%1:%2").arg(hostName, QString::number(port));
            LOG_WARNING(QString("连接失败，当前已经连接到 %1 ，无法连接到 %2").arg(currentHostInfo, targetHostInfo));

            // 连接目标不匹配时发送hostStatusChanged信号
            emit hostStatusChanged(false, hostName, port);

            return false;
        } else {
            // 如果相同，则打印输出"无需重复连接"
            QString hostInfo = QStringLiteral("%1:%2").arg(currentHostName, QString::number(currentPort));
            LOG_INFO(QString("TCP已经连接 %1 ，无需重复连接").arg(hostInfo));
            return true;
        }
    }
}

bool TCPSocketAdapter::disconnectFromHost()
{
    QMutexLocker locker(&m_mutex);

    if (!m_socket) return true;

    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QString hostName = m_socket->peerName();
        if (hostName.isEmpty()) {
            hostName = m_socket->peerAddress().toString();
        }
        quint16 port = m_socket->peerPort();
        QString hostInfo = QStringLiteral("%1:%2").arg(hostName, QString::number(port));

        m_socket->disconnectFromHost();

        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }

        LOG_INFO(QString("TCP： %1 连接已经断开").arg(hostInfo));

        // 重要修复：移除 const_cast
        // 发送断开连接信号
        emit hostStatusChanged(false, hostName, port);
    } else {
        // 如果本来就没有连接，也发送断开连接信号
        emit hostStatusChanged(false, QString(), 0);
    }

    return m_socket->state() == QAbstractSocket::UnconnectedState;
}

QVariantMap TCPSocketAdapter::getConnectedHostInfo() const
{
    QMutexLocker locker(&m_mutex);
    QVariantMap settings;
    bool isConnected = (m_socket && m_socket->state() == QAbstractSocket::ConnectedState);

    if (isConnected) {
        QString hostName = m_socket->peerName();
        if (hostName.isEmpty()) {
            hostName = m_socket->peerAddress().toString();
        }
        settings["hostName"] = hostName;
        settings["port"] = m_socket->peerPort();
    } else {
        settings["hostName"] = m_tcpConfig.value("hostName", "");
        settings["port"] = m_tcpConfig.value("port", 0);
    }
    settings["isConnected"] = isConnected;

    return settings;
}

bool TCPSocketAdapter::isHostConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TCPSocketAdapter::sendToHost(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        qint64 bytesWritten = m_socket->write(data);
        m_socket->flush();

        if (bytesWritten == -1) {
            QString logMessage = QStringLiteral("向TCP服务器写入数据失败: %1")
                                     .arg(m_socket->errorString());
            LOG_WARNING(logMessage);
            emit errorOccurred(m_socket->errorString());
            return false;
        } else if (bytesWritten > 0) {
            // 记录发送的数据量（基类方法）
            recordByteSent(bytesWritten);
            return true;
        } else {
            // bytesWritten == 0 的情况
            return false;
        }
    }

    // socket未连接的情况
    return false;
}

// ConnectionAdapter 接口实现

bool TCPSocketAdapter::connectToAdapter()
{
    QString hostName;
    quint16 port = 0;

    {
        QMutexLocker locker(&m_mutex);
        hostName = m_tcpConfig.value("hostName").toString();
        port = m_tcpConfig.value("port").toUInt();
    }

    if (hostName.isEmpty() || port == 0) {
        return false;
    }

    bool result = connectToHost(hostName, port);
    QVariantMap info = getConnectionParameters();
    emit connectionStatusChanged(getConnectionType(), result, info);

    return result;
}

void TCPSocketAdapter::disconnectFromAdapter()
{
    QString currentHostName;
    quint16 currentPort = 0;

    {
        QMutexLocker locker(&m_mutex);
        if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
            currentHostName = m_socket->peerName();
            if (currentHostName.isEmpty()) {
                currentHostName = m_socket->peerAddress().toString();
            }
            currentPort = m_socket->peerPort();
        }
    }

    disconnectFromHost();
    LOG_INFO(QString("%1 : %2 已经断开").arg(currentHostName).arg(currentPort));

    QVariantMap info = getConnectionParameters();
    emit connectionStatusChanged(getConnectionType(), false, info);
}

bool TCPSocketAdapter::isAdapterConnected() const
{
    return isHostConnected();
}

bool TCPSocketAdapter::forwardData(const QByteArray &data)
{
    return sendToHost(data);  // 直接返回结果
}

void TCPSocketAdapter::setConnectionParameters(const QVariantMap &config)
{
    QMutexLocker locker(&m_mutex);

    if (config.contains("hostName")) {
        m_tcpConfig["hostName"] = config["hostName"].toString();
    }
    if (config.contains("port")) {
        m_tcpConfig["port"] = config["port"].toInt();
    }
}

QVariantMap TCPSocketAdapter::getConnectionParameters() const
{
    // 从m_tcpConfig中提取配置的参数，以供连接函数使用
    QVariantMap params;
    QMutexLocker locker(&m_mutex);

    // 增加判断，是否已经配置过参数
    if (m_tcpConfig.isEmpty()) {
        // 没有配置参数，返回空的参数映射
        return params;
    }

    if (m_tcpConfig.contains("hostName")) {
        params["hostName"] = m_tcpConfig["hostName"];
    }
    if (m_tcpConfig.contains("port")) {
        params["port"] = m_tcpConfig["port"];
    }

    return params;
}

void TCPSocketAdapter::handleConnected()
{
    // 连接成功，由 connectToAdapter() 方法统一处理接口信号发射
}

void TCPSocketAdapter::handleDisconnected()
{
    QMutexLocker locker(&m_mutex);

    QString hostName = m_socket->peerName();
    if (hostName.isEmpty()) {
        hostName = m_socket->peerAddress().toString();
    }
    quint16 port = m_socket->peerPort();
    QString hostInfo = QStringLiteral("%1:%2").arg(hostName, QString::number(port));
    LOG_INFO(hostInfo);
}

void TCPSocketAdapter::handleError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QMutexLocker locker(&m_mutex);

    QString errorMessage = m_socket->errorString();
    QString hostName;
    quint16 port = 0;

    if (m_socket && (m_socket->state() == QAbstractSocket::ConnectedState ||
                     m_socket->state() == QAbstractSocket::ConnectingState)) {
        hostName = m_socket->peerName();
        if (hostName.isEmpty()) {
            hostName = m_tcpConfig.value("hostName", "").toString();
        }
        port = m_socket->peerPort();
        if (port == 0) {
            port = m_tcpConfig.value("port", 0).toUInt();
        }
    } else {
        hostName = m_tcpConfig.value("hostName", "").toString();
        port = m_tcpConfig.value("port", 0).toUInt();
    }

    QString hostInfo = QStringLiteral("%1:%2").arg(hostName, QString::number(port));
    QString logMessage = QStringLiteral("TCP连接(%1)错误: %2").arg(hostInfo, errorMessage);
    LOG_ERROR(logMessage);

    emit errorOccurred(errorMessage);
}

void TCPSocketAdapter::handleReadyRead()
{
    QMutexLocker locker(&m_mutex);

    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray newData = m_socket->readAll();

        if (!newData.isEmpty()) {
            // 记录接收的数据量（基类方法）
            recordByteReceived(newData.size());

            m_receivedBuffer.append(newData);

            if (m_receivedBuffer.size() > MaxBufferSize) {
                m_receivedBuffer = m_receivedBuffer.right(MaxBufferSize);
            }

            emit rawDataReceived(newData);
            emit adapterDataReceived(newData);
        }
    }
}

ConnectionAdapter::ConnectionType TCPSocketAdapter::getConnectionType() const
{
    return ConnectionAdapter::TCPSocket;
}

QVariantList TCPSocketAdapter::getAvailableAdapters() const
{
    return getAvailableHosts();
}
