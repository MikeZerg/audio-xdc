#include "ConnectionFactory.h"
#include "SerialPortAdapter.h"
#include "TCPSocketAdapter.h"
#include "code/library/LogManager.h"
#include <QTimer>
#include <QDateTime>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <QReadLocker>

// 定义计时间隔常量，单位为毫秒
const int TIMER_INTERVAL_MS = 1000;  // 1秒更新一次计时

ConnectionFactory::ConnectionFactory(QObject *parent)
    : QObject(parent)
    , m_Adapter(nullptr)
    , m_updateTimer(nullptr)
    , m_connectionStatus(Disconnected)
    , m_connectedTimer(0)
    , m_bytesSent(0)
    , m_bytesReceived(0)
{
    // 创建连接时长更新定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(TIMER_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        if (this->isFactoryConnected()) {
            m_connectedTimer++;
            // 每60秒发射一次统计信号，避免过于频繁的UI更新
            if (m_connectedTimer % 60 == 0) {
                emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
            }
        }
    });

    LOG_INFO("ConnectionFactory 构建完成");
}

ConnectionFactory::~ConnectionFactory()
{
    closeCurrentAdapter();

    if (m_updateTimer) {
        m_updateTimer->stop();
        m_updateTimer->deleteLater();
    }

    LOG_INFO("ConnectionFactory 析构完成");
}

// ======================================================================
// ===== UI流程第一步：适配器类型选择 =====
// ======================================================================

// 0. 工厂方法：创建适配器实例（内部使用）
ConnectionAdapter* ConnectionFactory::createFactoryAdapter(ConnectionAdapter::ConnectionType type)
{
    switch(type) {
    case ConnectionAdapter::SerialPort:
        return new SerialPortAdapter(this);
    case ConnectionAdapter::TCPSocket:
        return new TCPSocketAdapter(this);
    case ConnectionAdapter::Bluetooth:
        LOG_WARNING("蓝牙适配器尚未实现");
        return nullptr;
    default:
        LOG_WARNING(QString("未知的连接类型: %1").arg(type));
        return nullptr;
    }
}

// 1. UI第一步：创建或切换适配器类型
ConnectionAdapter* ConnectionFactory::createNewAdapter(ConnectionAdapter::ConnectionType type)
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁

    // 检查是否已存在适配器
    if (m_Adapter) {
        // 如果当前适配器类型匹配，直接复用
        if (m_Adapter->getConnectionType() == type) {
            LOG_INFO(QString("适配器类型 %1 已存在，复用现有实例").arg(type));
            return m_Adapter;
        } else {
            // 日志记录: 如果类型不匹配，不允许创建，提示用户使用switchAdapter
            LOG_WARNING(QString("已存在适配器，类型不匹配。当前类型: %1, ，请求类型: %2。请先关闭当前适配器或使用switchAdapter").arg(m_Adapter->getConnectionType()).arg(type));
        }
    }

    // 当前没有适配器，创建新适配器
    ConnectionAdapter* adapter = createFactoryAdapter(type);
    if (adapter) {
        // 设置为当前适配器
        m_Adapter = adapter;
        // 连接适配器信号
        connectAdapterSignals(adapter);
        locker.unlock();  // 手动解锁，发射信号
        emit adapterChanged(type);
        LOG_INFO(QString("创建适配器类型: %1").arg(type));
    }

    return adapter;
}

// 切换适配器
// 2. UI第一步扩展：安全切换适配器类型
bool ConnectionFactory::switchAdapter(ConnectionAdapter::ConnectionType type)
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁

    // === 场景1：未创建适配器 ===
    if (!m_Adapter) {
        ConnectionAdapter* newAdapter = createFactoryAdapter(type);
        if (!newAdapter) {
            LOG_WARNING(QString("无法创建类型为 %1 的适配器.").arg(type));
            return false;
        }

        m_Adapter = newAdapter;
        connectAdapterSignals(m_Adapter);

        locker.unlock();
        emit adapterChanged(type);
        LOG_INFO(QString("场景1：创建并切换到适配器类型: %1").arg(type));
        return true;
    }

    // 获取当前适配器状态
    ConnectionAdapter::ConnectionType currentType = m_Adapter->getConnectionType();
    bool isConnected = m_Adapter->isAdapterConnected();

    // === 场景2：已经存在适配器并且已经连接 ===
    if (isConnected) {
        LOG_WARNING("场景2：当前适配器已连接，无法切换，请先断开连接");
        return false;
    }

    // === 场景3：已经存在适配器但未连接 ===

    // 类型相同 → 不用切换，直接返回成功
    if (currentType == type) {
        LOG_INFO(QString("场景3a：适配器类型相同且未连接，无需切换: %1").arg(type));
        return true;
    }

    // 类型不同 → 可以切换
    // 保存旧适配器用于清理
    ConnectionAdapter* oldAdapter = m_Adapter;

    // 断开旧适配器的信号连接
    disconnectAdapterSignals(oldAdapter);

    // 创建新适配器
    ConnectionAdapter* newAdapter = createFactoryAdapter(type);
    if (!newAdapter) {
        // 创建失败，恢复原适配器
        LOG_WARNING(QString("无法创建类型为 %1 的适配器，恢复原适配器").arg(type));
        connectAdapterSignals(oldAdapter);
        m_Adapter = oldAdapter;
        return false;
    }

    // 设置新适配器
    m_Adapter = newAdapter;
    connectAdapterSignals(m_Adapter);

    // 安全删除旧适配器（延迟删除确保线程安全）
    QTimer::singleShot(0, oldAdapter, [oldAdapter]() {
        oldAdapter->deleteLater();
    });

    locker.unlock();

    // 发射信号通知UI更新
    emit adapterChanged(type);
    // 适配器切换，参数变化
    emit currentConnectionParametersChanged();

    LOG_INFO(QString("切换适配器类型ConnectionFactory: %1").arg(type));
    return true;
}

// 关闭当前适配器（用户主动操作）
void ConnectionFactory::closeCurrentAdapter()
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁

    if (!m_Adapter) {
        LOG_WARNING("当前没有适配器可关闭");
        return;
    }

    // 停止时长更新定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }

    // 重置连接时间和统计信息
    m_connectedTimer = 0;
    m_bytesSent = 0;
    m_bytesReceived = 0;

    // 断开信号连接
    disconnectAdapterSignals(m_Adapter);

    // 如果适配器已连接，先断开连接
    if (m_Adapter->isAdapterConnected()) {
        m_Adapter->disconnectFromAdapter();
    }

    // 删除适配器
    m_Adapter->deleteLater();
    m_Adapter = nullptr;
    updateConnectionStatus(Disconnected);

    locker.unlock();

    // 发射信号（使用合并后的信号）
    emit adapterChanged(ConnectionAdapter::SerialPort); // 使用默认类型
    emit connectionStatusChanged(Disconnected, ConnectionAdapter::SerialPort); // 使用默认类型
    emit statisticsUpdated(0, 0, 0);

    LOG_INFO("关闭当前适配器");
}

// 清理所有资源（系统级清理）
void ConnectionFactory::cleanupAdapterResources()
{
    // 直接调用closeCurrentAdapter()，因为功能完全相同
    closeCurrentAdapter();

    LOG_INFO("清理所有资源");
}

// ======================================================================
// ===== UI流程第二步：获取适配器端口列表 =====
// ======================================================================

// 3. UI第二步：获取适配器的端口列表（串口名、TCP主机等）
QVariantList ConnectionFactory::getAdapterPorts(ConnectionAdapter::ConnectionType type) const
{
    QReadLocker locker(&m_rwLock);  // 读操作使用读锁

    // 检查当前适配器是否存在且类型匹配
    if (m_Adapter && m_Adapter->getConnectionType() == type) {
        return m_Adapter->getAvailableAdapters();
    }

    // 如果没有适配器或类型不匹配，返回空列表并记录警告
    LOG_WARNING(QString("无法获取端口列表：适配器尚未创建或类型不匹配，请求类型: %1").arg(type));
    return QVariantList();
}

// ======================================================================
// ===== UI流程第三步：保存连接参数 =====
// ======================================================================

// 4. UI第三步：保存UI设置的连接参数
bool ConnectionFactory::saveAdapterConnectionParameters(ConnectionAdapter::ConnectionType type, const QVariantMap &config)
{
    // qDebug() << "UI流程第三步：保存适配器连接参数，类型:" << type << "配置:" << config;

    // 验证类型有效性
    if (type < ConnectionAdapter::SerialPort || type > ConnectionAdapter::Bluetooth) {
        LOG_WARNING(QString("无效的适配器类型: %1").arg(type));
        return false;
    }

    // 验证配置参数
    if (!validateConnectionParameters(type, config)) {
        LOG_WARNING(QString("配置参数验证失败，类型: %1").arg(type));
        return false;
    }

    // 保存配置到临时存储
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁
    m_factoryConfigurations[type] = config;

    // 🔥新增：同步更新当前适配器的配置
    if (m_Adapter && m_Adapter->getConnectionType() == type) {
        m_Adapter->setConnectionParameters(config);
        // qDebug() << "ConnectionFactory::saveAdapterConnectionParameters() 已同步更新当前适配器的配置参数";
    }

    // qDebug() << "ConnectionFactory::saveAdapterConnectionParameters() 适配器参数已保存到临时存储，类型:" << type
    //          << "参数:" << config;

    // 发射参数保存成功的信号
    emit adapterParametersSaved(type, config);

    return true;
}

bool ConnectionFactory::validateConnectionParameters(ConnectionAdapter::ConnectionType type, const QVariantMap &config)
{
    // 通用验证：配置不能为空
    if (config.isEmpty()) {
        LOG_WARNING("配置参数为空");
        return false;
    }

    switch (type) {
    case ConnectionAdapter::SerialPort:
        // 验证串口参数 - 支持两种参数格式
        if (!config.contains("portName") || config["portName"].toString().isEmpty()) {
            LOG_WARNING("串口配置缺少portName参数");
            return false;
        }
        if (!config.contains("baudRate") || config["baudRate"].toInt() <= 0) {
            LOG_WARNING("串口配置缺少有效的baudRate参数");
            return false;
        }
        // 可选参数验证：支持dataBits/parity/stopBits格式
        if (config.contains("dataBits") && config["dataBits"].toInt() < 0) {
            LOG_WARNING("串口配置dataBits参数无效");
            return false;
        }
        if (config.contains("parity") && config["parity"].toInt() < 0) {
            LOG_WARNING("串口配置parity参数无效");
            return false;
        }
        if (config.contains("stopBits") && config["stopBits"].toInt() < 0) {
            LOG_WARNING("串口配置stopBits参数无效");
            return false;
        }
        return true;

    case ConnectionAdapter::TCPSocket:
        // 验证TCP参数
        if (!config.contains("hostName") || config["hostName"].toString().isEmpty()) {
            LOG_WARNING("TCP配置缺少hostName参数");
            return false;
        }
        if (!config.contains("port") || config["port"].toInt() <= 0 ||
            config["port"].toInt() > 65535) {
            LOG_WARNING("TCP配置缺少有效的port参数");
            return false;
        }
        return true;

    case ConnectionAdapter::Bluetooth:
        LOG_WARNING("蓝牙适配器配置验证暂未实现");
        return false;

    default:
        LOG_WARNING(QString("未知的适配器类型: %1").arg(type));
        return false;
    }
}

// ======================================================================
// ===== UI流程第四步：建立连接 =====
// ======================================================================

// 5. UI第四步：连接到适配器
bool ConnectionFactory::connectToAdapter(ConnectionAdapter::ConnectionType type, const QVariantMap &config)
{
    // qDebug() << "UI流程第四步：开始连接适配器，类型:" << type;

    // 验证类型有效性
    if (type < ConnectionAdapter::SerialPort || type > ConnectionAdapter::Bluetooth) {
        LOG_WARNING(QString("无效的适配器类型: %1").arg(type));
        return false;
    }

    // 1. 检查当前连接状态
    if (isFactoryConnected()) {
        ConnectionAdapter::ConnectionType currentType = currentConnectedType();
        if (currentType == type) {
            LOG_INFO(QString("适配器已连接，类型相同，无需重新连接: %1").arg(type));
            return true;
        } else {
            LOG_WARNING(QString("已有连接的适配器，类型: %1 ，请先断开当前连接再连接新类型: %2").arg(currentType).arg(type));
            return false;
        }
    }

    // 2. 获取已创建的适配器
    QReadLocker locker(&m_rwLock);  // 读操作使用读锁

    if (!m_Adapter) {
        LOG_WARNING(QString("适配器尚未创建，请先选择适配器类型，类型: %1").arg(type));
        return false;
    }

    // 检查适配器类型是否匹配
    if (m_Adapter->getConnectionType() != type) {
        LOG_WARNING(QString("当前适配器类型不匹配，当前类型: %1 ，请求类型: %2").arg(m_Adapter->getConnectionType()).arg(type));
        return false;
    }

    // 当前适配器就是需要连接的适配器，无需重新设置
    ConnectionAdapter* adapter = m_Adapter;
    locker.unlock();  // 手动解锁，后续操作不需要锁保护

    // qDebug() << "使用已创建的适配器进行连接，类型:" << type;

    // 3. 获取连接配置
    QVariantMap connectionConfig = config;
    if (connectionConfig.isEmpty()) {
        // 如果没有提供配置，尝试从临时存储中获取
        QReadLocker configLocker(&m_rwLock);  // 读操作使用读锁
        if (m_factoryConfigurations.contains(type)) {
            connectionConfig = m_factoryConfigurations[type];
            LOG_INFO(QString("使用已保存的配置进行连接，类型: %1").arg(type));
        } else {
            LOG_WARNING(QString("未提供连接配置且无已保存的配置，类型: %1").arg(type));
            return false;
        }
    }

    // 4. 验证配置参数
    if (!validateConnectionParameters(type, connectionConfig)) {
        LOG_WARNING(QString("连接配置验证失败，类型: %1").arg(type));
        return false;
    }

    // 5. 设置连接参数
    adapter->setConnectionParameters(connectionConfig);

    // 6. 更新连接状态为连接中
    updateConnectionStatus(Connecting);

    // 7. 执行连接操作
    bool success = adapter->connectToAdapter();

    if (success) {
        // qDebug() << "UI流程第四步完成：适配器连接成功，类型:" << type;
    } else {
        LOG_WARNING(QString("UI流程第四步失败：适配器连接失败，类型: %1").arg(type));
        updateConnectionStatus(Disconnected);
    }

    return success;
}

// ======================================================================
// ===== UI流程第五步：数据传输 =====
// ======================================================================

// 6. UI第五步：发送数据
bool ConnectionFactory::sendData(const QByteArray &data)
{
    if (data.isEmpty()) {
        LOG_WARNING("发送数据为空");
        return false;
    }

    QReadLocker locker(&m_rwLock);  // 读操作使用读锁
    if (!m_Adapter || !m_Adapter->isAdapterConnected()) {
        LOG_WARNING("没有连接或没有适配器，无法发送数据");
        return false;
    }

    // qDebug() << "UI流程第五步：发送数据，长度:" << data.length();
    return m_Adapter->forwardData(data);
}

// ======================================================================
// ===== UI流程第六步：断开连接 =====
// ======================================================================

// 7. UI第六步：断开适配器连接
void ConnectionFactory::disconnectFromAdapter()
{
    LOG_INFO("UI流程第六步：断开适配器连接");

    QReadLocker locker(&m_rwLock);  // 读操作使用读锁
    if (!m_Adapter) {
        return;
    }

    ConnectionAdapter* adapter = m_Adapter;
    locker.unlock();  // 手动解锁，后续操作不需要锁保护

    updateConnectionStatus(Disconnecting);

    adapter->disconnectFromAdapter();

    // 停止时长更新定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }

    emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
    LOG_INFO("UI流程第六步完成：适配器已断开");
}

// ======================================================================
// ===== 辅助方法：属性访问和状态管理 =====
// ======================================================================
QVariantMap ConnectionFactory::getConnectionParameters() const
{
    QReadLocker locker(&m_rwLock);  // 读操作使用读锁

    if (!m_Adapter) {
        return QVariantMap();
    }

    QVariantMap params = m_Adapter->getConnectionParameters();

    // qDebug() << "ConnectionFactory::getConnectionParameters()读取适配器参数:"<< params;

    return params;
}

QVariantMap ConnectionFactory::getCurrentConnectionParameters() const
{
    // 1. 先尝试获取适配器参数
    QVariantMap params = getConnectionParameters();

    // 2. 如果适配器参数为空，返回已保存配置
    if (params.isEmpty()) {
        QReadLocker locker(&m_rwLock);
        ConnectionAdapter::ConnectionType currentType = currentConnectedType();
        if (m_factoryConfigurations.contains(currentType)) {
            return m_factoryConfigurations[currentType];
        }
    }

    return params;
}

QVariantMap ConnectionFactory::getStatistics() const
{
    QVariantMap statistics;

    {
        QReadLocker locker(&m_rwLock);  // 读操作使用读锁
        statistics["isConnected"] = isFactoryConnected();
        statistics["connectedTime"] = connectedTime();
        statistics["bytesSent"] = bytesSent();
        statistics["bytesReceived"] = bytesReceived();

        if (m_Adapter) {
            QVariantMap adapterStats = m_Adapter->getStatistics();
            statistics["adapterStats"] = adapterStats;
        }
    }

    return statistics;
}

void ConnectionFactory::resetStatistics()
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁

    m_bytesSent = 0;
    m_bytesReceived = 0;

    if (m_Adapter) {
        m_Adapter->resetStatistics();
    }

    locker.unlock();  // 手动解锁发射信号
    emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
}

// ===== 属性访问方法 =====

ConnectionAdapter::ConnectionType ConnectionFactory::currentConnectedType() const
{
    return m_Adapter ? m_Adapter->getConnectionType() : ConnectionAdapter::SerialPort;
}

bool ConnectionFactory::isFactoryConnected() const
{
    return m_Adapter ? m_Adapter->isAdapterConnected() : false;
}

ConnectionFactory::ConnectionStatus ConnectionFactory::connectionStatus() const
{
    QReadLocker locker(&m_rwLock);  // 读操作使用读锁
    return m_connectionStatus;
}

qint64 ConnectionFactory::connectedTime() const
{
    return m_connectedTimer;
}

qint64 ConnectionFactory::bytesSent() const
{
    return m_bytesSent;
}

qint64 ConnectionFactory::bytesReceived() const
{
    return m_bytesReceived;
}

// ===== 核心逻辑实现 =====
void ConnectionFactory::updateConnectionStatus(ConnectionStatus status)
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁

    if (m_connectionStatus != status) {
        m_connectionStatus = status;
        ConnectionAdapter::ConnectionType type = m_Adapter ? m_Adapter->getConnectionType() : ConnectionAdapter::SerialPort;
        locker.unlock();
        emit connectionStatusChanged(status, type);
    }
}

void ConnectionFactory::resetConnectionTimer()
{
    QWriteLocker locker(&m_rwLock);  // 写操作使用写锁
    m_connectedTimer = 0;
    locker.unlock();
    emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
}

// ===== 信号处理 =====
void ConnectionFactory::handleConnectionStatusChanged(ConnectionAdapter::ConnectionType type,
                                                      bool connected,
                                                      const QVariantMap &connectionParameters)
{
    // 更新连接状态（updateConnectionStatus会发射合并后的connectionStatusChanged信号）
    updateConnectionStatus(connected ? Connected : Disconnected);

    if (connected) {
        // 重置连接时间计数器
        resetConnectionTimer();

        // 启动UI更新定时器
        if (m_updateTimer) {
            m_updateTimer->start();
        }

        // 发射连接状态变化信号
        emit connectionStatusChanged(Connected, type);
        // 发射连接参数变化信号
        emit currentConnectionParametersChanged();

    } else {
        // 停止UI更新定时器
        if (m_updateTimer) {
            m_updateTimer->stop();
        }

        // 发射断开连接信息
        emit connectionStatusChanged(Disconnected, type);
        // 发射连接参数变化信号
        emit currentConnectionParametersChanged();
        // 发射最终的统计信息
        emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
    }
}

void ConnectionFactory::handleStatisticsUpdated()
{
    // 同步适配器的字节数统计到工厂
    if (m_Adapter) {
        m_bytesSent = m_Adapter->getByteSent();
        m_bytesReceived = m_Adapter->getByteReceived();
    }
    emit statisticsUpdated(m_connectedTimer, m_bytesSent, m_bytesReceived);
}

void ConnectionFactory::handleDataReceived(const QByteArray &data)
{
    emit factoryDataReceived(data);
}

void ConnectionFactory::handleErrorOccurred(const QString &error)
{
    emit errorOccurred(error);
}

// ===== 信号连接管理 =====
void ConnectionFactory::connectAdapterSignals(ConnectionAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    connect(adapter, &ConnectionAdapter::connectionStatusChanged,
            this, &ConnectionFactory::handleConnectionStatusChanged);
    connect(adapter, &ConnectionAdapter::adapterDataReceived,
            this, &ConnectionFactory::handleDataReceived);
    connect(adapter, &ConnectionAdapter::errorOccurred,
            this, &ConnectionFactory::handleErrorOccurred);
    connect(adapter, &ConnectionAdapter::statisticsUpdated,
            this, &ConnectionFactory::handleStatisticsUpdated);
}

void ConnectionFactory::disconnectAdapterSignals(ConnectionAdapter* adapter)
{
    if (!adapter) {
        return;
    }

    disconnect(adapter, &ConnectionAdapter::connectionStatusChanged,
               this, &ConnectionFactory::handleConnectionStatusChanged);
    disconnect(adapter, &ConnectionAdapter::adapterDataReceived,
               this, &ConnectionFactory::handleDataReceived);
    disconnect(adapter, &ConnectionAdapter::errorOccurred,
               this, &ConnectionFactory::handleErrorOccurred);
    disconnect(adapter, &ConnectionAdapter::statisticsUpdated,
               this, &ConnectionFactory::handleStatisticsUpdated);
}

// ===== 辅助方法 =====
QString ConnectionFactory::getAdapterTypeName(ConnectionAdapter::ConnectionType type) const
{
    switch (type) {
    case ConnectionAdapter::SerialPort:
        return "串口";
    case ConnectionAdapter::TCPSocket:
        return "TCP网络";
    case ConnectionAdapter::Bluetooth:
        return "蓝牙";
    default:
        return "未知类型";
    }
}
