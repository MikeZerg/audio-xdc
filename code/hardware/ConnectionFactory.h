#ifndef CONNECTIONFACTORY_H
#define CONNECTIONFACTORY_H

#include <QObject>
#include <QMap>
#include <QReadWriteLock>
#include <QTimer>
#include "ConnectionAdapter.h"

class ConnectionFactory : public QObject
{
    Q_OBJECT

    // QML属性：暴露给前端使用（根据UI设计规范）
    Q_PROPERTY(ConnectionAdapter::ConnectionType currentConnectedType READ currentConnectedType NOTIFY adapterChanged)
    Q_PROPERTY(bool isConnected READ isFactoryConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(qint64 connectedTime READ connectedTime NOTIFY statisticsUpdated)
    Q_PROPERTY(qint64 bytesSent READ bytesSent NOTIFY statisticsUpdated)
    Q_PROPERTY(qint64 bytesReceived READ bytesReceived NOTIFY statisticsUpdated)
    Q_PROPERTY(QVariantMap currentConnectionParameters READ getCurrentConnectionParameters NOTIFY currentConnectionParametersChanged)

public:
    // 连接状态枚举
    enum ConnectionStatus {Disconnected, Connecting, Connected, Disconnecting};
    Q_ENUM(ConnectionStatus)

    explicit ConnectionFactory(QObject *parent = nullptr);
    ~ConnectionFactory();

    // ===== 工厂方法接口（面向C++开发者） =====
    // 1. 创建新适配器，UI第一步
    Q_INVOKABLE ConnectionAdapter* createNewAdapter(ConnectionAdapter::ConnectionType type);
    Q_INVOKABLE bool switchAdapter(ConnectionAdapter::ConnectionType type);
    Q_INVOKABLE void closeCurrentAdapter();
    Q_INVOKABLE void cleanupAdapterResources();

    // ===== QML友好接口（面向前端开发者） =====
    // 2. 获取适配器的端口名称（比如串口的串口名，TCP的IP和Port，等等），UI第二步
    Q_INVOKABLE QVariantList getAdapterPorts(ConnectionAdapter::ConnectionType type) const;

    // 3. 保存UI设置的连接参数，用于连接。 UI第三步
    Q_INVOKABLE bool saveAdapterConnectionParameters(ConnectionAdapter::ConnectionType type, const QVariantMap &config);
    // 验证不同连接类型的连接参数是否符合规范
    Q_INVOKABLE bool validateConnectionParameters(ConnectionAdapter::ConnectionType type, const QVariantMap &config);
    // 从成员变量获取当前已连接适配器的连接参数
    Q_INVOKABLE QVariantMap getConnectionParameters() const;    // 用于连接时获取参数
    Q_INVOKABLE QVariantMap getCurrentConnectionParameters() const;  // 复用函数，读取连接参数用于属性更新

    // 4. 连接到适配器。UI第四步
    Q_INVOKABLE bool connectToAdapter(ConnectionAdapter::ConnectionType type, const QVariantMap &config = QVariantMap());

    // 5. 发送数据。 UI第五步
    Q_INVOKABLE bool sendData(const QByteArray &data);
    // 获取统计数据（连接时长，发送数据字节数，接收数据字节数）
    Q_INVOKABLE QVariantMap getStatistics() const;
    // 重置统计数据（连接时长，发送数据字节数，接收数据字节数）
    Q_INVOKABLE void resetStatistics();

    // 6. 断开适配器连接。UI第六步
    Q_INVOKABLE void disconnectFromAdapter();

    // ===== 属性访问方法 =====
    ConnectionAdapter::ConnectionType currentConnectedType() const;
    bool isFactoryConnected() const;
    ConnectionStatus connectionStatus() const;
    qint64 connectedTime() const;
    qint64 bytesSent() const;
    qint64 bytesReceived() const;

signals:
    // === 优化后的信号列表 ===
    
    // 1. 适配器变化信号（合并currentAdapterChanged和currentTypeChanged）
    void adapterChanged(ConnectionAdapter::ConnectionType type);
    
    // 2. 连接状态变化信号（合并connectionStatusChanged和adapterConnected/Disconnected）
    void connectionStatusChanged(ConnectionFactory::ConnectionStatus status, ConnectionAdapter::ConnectionType type);
    
    // 3. 统计信息更新信号（合并相关统计信息）
    void statisticsUpdated(qint64 connectedTime, qint64 bytesSent, qint64 bytesReceived);
    
    // 4. 数据传输相关信号
    void factoryDataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    
    // 5. 参数保存信号（保持不变）
    void adapterParametersSaved(ConnectionAdapter::ConnectionType type, const QVariantMap &params);
    void currentConnectionParametersChanged();

private slots:
    // 适配器信号处理
    void handleConnectionStatusChanged(ConnectionAdapter::ConnectionType type, bool connected, const QVariantMap &params);
    void handleDataReceived(const QByteArray &data);
    void handleErrorOccurred(const QString &error);
    void handleStatisticsUpdated();

private:
    // 适配器管理（使用读写锁优化性能）
    mutable QReadWriteLock m_rwLock;
    ConnectionAdapter* m_Adapter;
    
    // 连接状态管理
    ConnectionStatus m_connectionStatus;
    qint64 m_bytesSent;
    qint64 m_bytesReceived;
    qint64 m_connectedTimer;
    QTimer* m_updateTimer;
    
    // 临时配置参数存储（供connectToAdapter使用）
    QMap<ConnectionAdapter::ConnectionType, QVariantMap> m_factoryConfigurations;

    // 工厂方法
    ConnectionAdapter* createFactoryAdapter(ConnectionAdapter::ConnectionType type);
    
    // 信号管理
    void connectAdapterSignals(ConnectionAdapter* adapter);
    void disconnectAdapterSignals(ConnectionAdapter* adapter);
    
    // 核心逻辑
    void updateConnectionStatus(ConnectionStatus status);
    void resetConnectionTimer();
    
    // 辅助方法
    QString getAdapterTypeName(ConnectionAdapter::ConnectionType type) const;
};

#endif // CONNECTIONFACTORY_H
