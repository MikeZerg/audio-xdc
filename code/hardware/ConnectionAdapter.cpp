#include "ConnectionAdapter.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>


ConnectionAdapter::ConnectionAdapter(QObject *parent)
    : QObject(parent)
    , m_ByteSent(0)         // 初始化发送计数器
    , m_ByteReceived(0)     // 初始化接收计数器
{
    // qDebug() << "ConnectionAdapter 构建完成";
}

ConnectionAdapter::~ConnectionAdapter()
{
    // qDebug() << "ConnectionAdapter 析构完成";
}

// 获取累计发送的字节数
qint64 ConnectionAdapter::getByteSent() const
{
    return m_ByteSent;
}

// 获取累计接收的字节数
qint64 ConnectionAdapter::getByteReceived() const
{
    return m_ByteReceived;
}

// 记录发送的字节数并触发更新信号
void ConnectionAdapter::recordByteSent(qint64 bytes)
{
    if (bytes > 0) {
        m_ByteSent += bytes;
        emit statisticsUpdated();
    }
}

// 记录接收的字节数并触发更新信号
void ConnectionAdapter::recordByteReceived(qint64 bytes)
{
    if (bytes > 0) {
        m_ByteReceived += bytes;
        emit statisticsUpdated();
    }
}

// 获取包含发送和接收字节的统计信息映射
QVariantMap ConnectionAdapter::getStatistics() const
{
    QVariantMap statistics;
    statistics["byteSent"] = m_ByteSent;
    statistics["byteReceived"] = m_ByteReceived;
    return statistics;
}

// 重置所有统计数据为零
void ConnectionAdapter::resetStatistics()
{
    m_ByteSent = 0;
    m_ByteReceived = 0;
    emit statisticsUpdated();
}