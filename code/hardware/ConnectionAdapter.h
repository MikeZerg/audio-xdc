#ifndef CONNECTIONADAPTER_H
#define CONNECTIONADAPTER_H

#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMutex>

// 连接适配器基类，定义所有通讯接口（串口、TCP、蓝牙等）的统一规范
class ConnectionAdapter : public QObject
{
    Q_OBJECT

    // 注册属性以便在 QML 中直接访问
    Q_PROPERTY(ConnectionType connectionType READ getConnectionType CONSTANT)
    Q_PROPERTY(QVariantMap connectionParameter READ getConnectionParameters NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool isConnected READ isAdapterConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(qint64 byteSent READ getByteSent NOTIFY statisticsUpdated)
    Q_PROPERTY(qint64 byteReceived READ getByteReceived NOTIFY statisticsUpdated)

public:
    // 定义支持的连接类型枚举
    enum ConnectionType {
        SerialPort,   // 串口通讯
        TCPSocket,    // TCP 网络通讯
        Bluetooth     // 蓝牙通讯（预留）
    };
    Q_ENUM(ConnectionType)

    explicit ConnectionAdapter(QObject *parent = nullptr);
    virtual ~ConnectionAdapter();

    // ===== 统计接口函数 =====
    // Q_INVOKABLE virtual qint64 getConnectedTime() const;
    Q_INVOKABLE virtual qint64 getByteSent() const;          // 获取累计发送字节数
    Q_INVOKABLE virtual qint64 getByteReceived() const;      // 获取累计接收字节数
    Q_INVOKABLE virtual QVariantMap getStatistics() const;   // 获取完整的统计信息映射
    Q_INVOKABLE virtual void resetStatistics();              // 重置统计数据

    // ===== 核心通信接口（纯虚函数，由子类实现） =====
    virtual bool connectToAdapter() = 0;                     // 建立连接
    virtual void disconnectFromAdapter() = 0;                // 断开连接
    virtual bool isAdapterConnected() const = 0;             // 检查连接状态
    virtual bool forwardData(const QByteArray &data) = 0;    // 转发数据

    // ===== 配置接口（由子类实现） =====
    virtual QVariantList getAvailableAdapters() const = 0;   // 获取可用的适配器列表（如 COM 口或 IP）
    virtual void setConnectionParameters(const QVariantMap &config) = 0; // 设置连接参数
    virtual QVariantMap getConnectionParameters() const = 0; // 获取当前连接参数
    virtual ConnectionType getConnectionType() const = 0;    // 获取当前适配器的类型

signals:
    // 连接状态改变信号：包含类型、是否连接以及当前的参数配置
    void connectionStatusChanged(ConnectionAdapter::ConnectionType type,
                                 bool isConnected,
                                 const QVariantMap &connectionParameters);

    void adapterDataReceived(const QByteArray &data);        // 接收到原始数据信号
    void errorOccurred(const QString &error);                // 发生错误信号
    void statisticsUpdated();                                // 统计数据更新信号

protected:
    // 收发字节统计成员变量
    qint64 m_ByteSent;                                       // 已发送字节计数
    qint64 m_ByteReceived;                                   // 已接收字节计数

    // 统计方法：供子类调用以更新统计数据
    virtual void recordByteSent(qint64 bytes);               // 记录发送的字节数
    virtual void recordByteReceived(qint64 bytes);           // 记录接收的字节数

private:
    // 不再有任何静态成员变量
};

#endif // CONNECTIONADAPTER_H