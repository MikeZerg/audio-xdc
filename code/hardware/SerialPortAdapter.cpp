#include "SerialPortAdapter.h"
#include "code/library/LogManager.h"
#include <QCoreApplication>
#include <QElapsedTimer>

SerialPortAdapter::SerialPortAdapter(QObject *parent)
    : ConnectionAdapter(parent),
    m_serialPort(nullptr)
{
    // 初始化串口对象
    m_serialPort = new QSerialPort(this);

    // 设置默认配置参数
    m_serialConfig["portName"] = "COM1";   // 默认串口号
    m_serialConfig["baudRate"] = 115200;   // 默认波特率
    m_serialConfig["dataBits"] = 3;        // 3 表示 8 位
    m_serialConfig["parity"] = 0;          // 0 表示无校验
    m_serialConfig["stopBits"] = 0;        // 0 表示 1 位停止位

    // 连接信号和槽
    QObject::connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPortAdapter::handleReadyRead);
    QObject::connect(m_serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
                     this, &SerialPortAdapter::handleError);

    LOG_INFO("SerialPortAdapter 构建完成，连接开始");
}

SerialPortAdapter::~SerialPortAdapter()
{
    LOG_INFO("SerialPortAdapter 析构开始");

    if (m_serialPort && m_serialPort->isOpen()) {
        QString portName = m_serialPort->portName();
        LOG_INFO(QString("正在关闭串口: %1").arg(portName));

        m_serialPort->clear();
        m_serialPort->close();

        // 等待关闭，但限制时间
        QElapsedTimer timer;
        timer.start();
        while (m_serialPort->isOpen() && timer.elapsed() < 2000) {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        if (m_serialPort->isOpen()) {
            LOG_WARNING(QString("串口关闭超时: %1").arg(portName));
        }
    }

    delete m_serialPort;
    m_serialPort = nullptr;

    // 清理缓冲区
    m_receivedBuffer.clear();
    m_receivedBuffer.squeeze();

    LOG_INFO("SerialPortAdapter 析构完成，连接断开");
}

// 检测可以连接的串口硬件
QVariantList SerialPortAdapter::getAvailablePorts() const
{
    QMutexLocker locker(&m_mutex);
    QVariantList portList;

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QVariantMap portInfo;
        portInfo["portName"] = info.portName();
        portInfo["description"] = info.description();
        portInfo["manufacturer"] = info.manufacturer();
        portInfo["systemLocation"] = info.systemLocation();
        portInfo["productIdentifier"] = info.productIdentifier();
        portInfo["vendorIdentifier"] = info.vendorIdentifier();
        portList.append(portInfo);
    }
    return portList;
}

// 以设定参数连接指定号串口
bool SerialPortAdapter::connectToPort(const QString &portName, qint32 baudRate, int dataBitsIndex, int parityIndex, int stopBitsIndex)
{
    QMutexLocker locker(&m_mutex);

    if (portName.isEmpty()) {
        LOG_ERROR("严重错误: 尝试连接空端口名");
        return false;
    }

    // 如果当前已经连接串口，则提示当前连接的串口名称，而不断开重连
    if (m_serialPort->isOpen()) {
        QString currentPortName = m_serialPort->portName();
        LOG_INFO(QString("%1 已经连接，无需重连").arg(currentPortName));
        return true;
    }

    // 设置数据位
    QSerialPort::DataBits dataBits;
    switch (dataBitsIndex) {
    case 5: dataBits = QSerialPort::Data5; break;
    case 6: dataBits = QSerialPort::Data6; break;
    case 7: dataBits = QSerialPort::Data7; break;
    case 8: dataBits = QSerialPort::Data8; break;
    default: dataBits = QSerialPort::Data8;
    }

    // 设置校验位
    QSerialPort::Parity parity;
    switch (parityIndex) {
    case 0: parity = QSerialPort::NoParity; break;
    case 1: parity = QSerialPort::OddParity; break;
    case 2: parity = QSerialPort::EvenParity; break;
    default: parity = QSerialPort::NoParity;
    }

    // 设置停止位
    QSerialPort::StopBits stopBits;
    switch (stopBitsIndex) {
    case 1: stopBits = QSerialPort::OneStop; break;
    case 3: stopBits = QSerialPort::OneAndHalfStop; break;
    case 2: stopBits = QSerialPort::TwoStop; break;
    default: stopBits = QSerialPort::OneStop;
    }

    // 设置串口参数
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);

    // 打开串口同时读写
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString logMessage = QStringLiteral("连接串口(%1)失败: %2")
                                 .arg(portName, m_serialPort->errorString());
        LOG_WARNING(logMessage);
        return false;
    }

    // 清空接收缓存
    m_receivedBuffer.clear();

    // 发送具体适配器的状态变化信号
    emit portStatusChanged(true, baudRate, m_serialPort->dataBits(),
                           m_serialPort->parity(), m_serialPort->stopBits());

    return true;
}

bool SerialPortAdapter::disconnectFromPort()
{
    QMutexLocker locker(&m_mutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        return true;
    }

    QString currentPortName = m_serialPort->portName();
    m_serialPort->close();

    bool isPortClosed = !m_serialPort->isOpen();
    if (isPortClosed) {
        m_receivedBuffer.clear();
    } else {
        LOG_WARNING(QString("串口 %1 关闭失败").arg(currentPortName));
    }

    // 发送具体适配器的状态变化信号
    emit portStatusChanged(false);

    return isPortClosed;
}

QVariantMap SerialPortAdapter::getConnectedPortInfo() const
{
    QMutexLocker locker(&m_mutex);
    QVariantMap settings;

    if (m_serialPort && m_serialPort->isOpen()) {
        settings["isOpen"] = true;
        settings["portName"] = m_serialPort->portName();
        settings["baudRate"] = m_serialPort->baudRate();
        settings["dataBits"] = static_cast<int>(m_serialPort->dataBits());
        settings["parity"] = static_cast<int>(m_serialPort->parity());
        settings["stopBits"] = static_cast<int>(m_serialPort->stopBits());
    } else {
        settings["isOpen"] = false;
        settings["portName"] = m_serialPort ? m_serialPort->portName() : "";
        settings["baudRate"] = 115200;
        settings["dataBits"] = 3;
        settings["parity"] = 0;
        settings["stopBits"] = 0;
    }
    return settings;
}

bool SerialPortAdapter::isPortConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialPort && m_serialPort->isOpen();
}

bool SerialPortAdapter::sendToPort(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if (!m_serialPort) {
        LOG_ERROR("错误: 串口对象未初始化");
        return false;
    }

    if (!m_serialPort->isOpen()) {
        LOG_ERROR("错误: 串口未打开");
        return false;
    }

    qint64 bytesWritten = m_serialPort->write(data);

    // 记录发送的数据量（基类方法）
    if (bytesWritten > 0) {
        recordByteSent(bytesWritten);
    }

    bool success = false;
    if (bytesWritten < 0) {
        LOG_ERROR(QString("错误: 向串口写入数据失败，错误信息: %1").arg(m_serialPort->errorString()));
    } else if (bytesWritten != data.size()) {
        LOG_WARNING(QString("警告: 发送数据不完整，预期: %1 字节，实际: %2 字节").arg(data.size()).arg(bytesWritten));
        success = (bytesWritten > 0);
    } else {
        success = true;
    }

    m_serialPort->flush();
    return success;
}

void SerialPortAdapter::handleReadyRead()
{
    QMutexLocker locker(&m_mutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        return;
    }

    QByteArray newData = m_serialPort->readAll();

    // 记录接收的数据量（基类方法）
    if (!newData.isEmpty()) {
        recordByteReceived(newData.size());
    }

    if (newData.isEmpty()) {
        return;
    }

    // 处理缓冲区大小
    if (m_receivedBuffer.size() + newData.size() > MaxBufferSize) {
        m_receivedBuffer.clear();
        if (newData.size() > MaxBufferSize) {
            newData = newData.right(MaxBufferSize);
        }
    }

    m_receivedBuffer.append(newData);

    // 发出原始数据接收信号
    emit rawDataReceived(newData);
    // 同时发出基类定义的 dataReceived 信号
    emit adapterDataReceived(newData);
}

void SerialPortAdapter::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::ResourceError) {
        return;
    }

    QString errorMessage = m_serialPort->errorString();
    QString portInfo = m_serialPort->portName().isEmpty() ? "未知端口" : m_serialPort->portName();

    QString logMessage = QStringLiteral("串口(%1)错误: %2").arg(portInfo, errorMessage);
    LOG_ERROR(logMessage);

    // 发出基类错误信号
    emit errorOccurred(errorMessage);
}

// ConnectionAdapter 接口实现

bool SerialPortAdapter::connectToAdapter()
{
    ConnectionType cType = getConnectionType();

    if (isAdapterConnected()) {
        QVariantMap cInfo = getConnectionParameters();
        emit connectionStatusChanged(cType, true, cInfo);
        return true;
    }

    // 读取配置参数
    QString portName;
    int baudRate, dataBits, parity, stopBits;
    {
        QMutexLocker locker(&m_mutex);
        portName = m_serialConfig["portName"].toString();
        baudRate = m_serialConfig.value("baudRate", 115200).toInt();
        dataBits = m_serialConfig.value("dataBits", 3).toInt();
        parity = m_serialConfig.value("parity", 0).toInt();
        stopBits = m_serialConfig.value("stopBits", 0).toInt();
    }

    if (portName.isEmpty()) {
        LOG_WARNING("连接失败: 串口配置中的端口名为空");
        emit errorOccurred("串口配置错误: 端口名不能为空，请先配置串口参数");
        return false;
    }

    if (baudRate <= 0) {
        LOG_WARNING(QString("连接失败: 波特率无效: %1").arg(baudRate));
        emit errorOccurred(QString("串口配置错误: 波特率无效: %1").arg(baudRate));
        return false;
    }

    bool result = connectToPort(portName, baudRate, dataBits, parity, stopBits);
    QVariantMap cInfo = getConnectionParameters();
    emit connectionStatusChanged(cType, result, cInfo);

    return result;
}

void SerialPortAdapter::disconnectFromAdapter()
{
    bool disconnectResult = disconnectFromPort();

    if (!disconnectResult) {
        LOG_WARNING("调用 disconnectFromPort() 失败");
    }

    QVariantMap info = getConnectionParameters();
    emit connectionStatusChanged(getConnectionType(), false, info);
}

bool SerialPortAdapter::isAdapterConnected() const
{
    return isPortConnected();
}

bool SerialPortAdapter::forwardData(const QByteArray &data)
{
    if (!isPortConnected()) {
        LOG_WARNING("串口未连接，无法发送数据");
        return false;
    }

    return sendToPort(data);
}

QVariantMap SerialPortAdapter::getConnectionParameters() const
{
    // 从m_serialConfig中提取配置的参数，以供连接函数使用
    QVariantMap params;
    QMutexLocker locker(&m_mutex);

    // 增加判断，是否已经配置过参数
    if (m_serialConfig.isEmpty()) {
        // 没有配置参数，返回空的参数映射
        return params;
    }

    // 明确指定需要的串口参数
    if (m_serialConfig.contains("portName")) {
        params["portName"] = m_serialConfig["portName"];
    }
    if (m_serialConfig.contains("baudRate")) {
        params["baudRate"] = m_serialConfig["baudRate"];
    }
    if (m_serialConfig.contains("dataBits")) {
        params["dataBits"] = m_serialConfig["dataBits"];
    }
    if (m_serialConfig.contains("parity")) {
        params["parity"] = m_serialConfig["parity"];
    }
    if (m_serialConfig.contains("stopBits")) {
        params["stopBits"] = m_serialConfig["stopBits"];
    }

    return params;
}

void SerialPortAdapter::setConnectionParameters(const QVariantMap &config)
{
    QMutexLocker locker(&m_mutex);

    if (config.contains("portName")) {
        QString portName = config["portName"].toString();
        if (!portName.isEmpty()) {
            m_serialConfig["portName"] = portName;
        }
    }
    if (config.contains("baudRate")) {
        int baudRate = config["baudRate"].toInt();
        m_serialConfig["baudRate"] = (baudRate <= 0) ? 115200 : baudRate;
    }
    if (config.contains("dataBits")) {
        m_serialConfig["dataBits"] = config["dataBits"].toInt();
    }
    if (config.contains("parity")) {
        m_serialConfig["parity"] = config["parity"].toInt();
    }
    if (config.contains("stopBits")) {
        m_serialConfig["stopBits"] = config["stopBits"].toInt();
    }
}

ConnectionAdapter::ConnectionType SerialPortAdapter::getConnectionType() const
{
    return ConnectionAdapter::SerialPort;
}

QVariantList SerialPortAdapter::getAvailableAdapters() const
{
    QMutexLocker locker(&m_mutex);
    QVariantList portList;

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        portList.append(info.portName());
    }
    return portList;
}
