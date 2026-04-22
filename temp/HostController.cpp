// source/HostController.cpp
#include "HostController.h"
#include <QThread>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QApplication>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QThread>
#include <QVariantList>

/**
 * HostManager类说明
 * 1. 构建查询命令常量，常量供查询单指令函数queryHostInfo()调用，或者供队列指令查询函数queryByCommandQueue()调用
 * 2. 创建一个HostManager对象，并传入SerialPortHandler和ProtocolProcessor对象作为参数。
 * 3. 成员函数detectingHost()用于向发送探测帧，等待触发single_rsp_Host_Detected信号。
 * 4. 成员函数detectedHost(quint8 hostAddress)，槽函数，将信号中解析出的地址，加入主机实例。
 * 5. 成员函数initializeConnection()用于连接信号和槽。接收single_rsp_Host_Detected信号并解析出主机地址。
 * 6. 成员函数setupHostController(quint8 hostAddress),根据地址生成单个实例，并添加到m_hostControllers中。
 * 7. 成员函数removeHostController(quint8 hostAddress)，移除一个主机，删除地址及控制器。
 * 8. 成员函数stopHostController(quint8 hostAddress)，停用一个主机控制器实例，保留地址删除控制器。
 * 9. 成员函数switchHostController(quint8 hostAddress)，切换主机控制器实例。
 * 10.成员函数currentHostController() const,返回当前主机控制器实例。
 * 11.成员函数initializeCurrentHostProperties(), 初始化当前主机控制器实例。
 * 12.成员函数queryHostStatus() const,查询主机地址列表。
 */


// ********* 注释了一些暂时不查询的信息的发送指令常量，发送函数，
// 构建常用的查询命令常量（当前只有18个初始化指令常量， 参数为操作类型opCode, 操作指令Command，指令参数payload）
const HostController::HostQueryInfo HostController::const_hostKernelVersion(0x01, 0x0202, QByteArray(1, '\x00'));               // 0x0202-(0x00) 获取主机版本号(Kernel)
const HostController::HostQueryInfo HostController::const_hostLinkVersion(0x01, 0x0202, QByteArray(1, '\x01'));                 // 0x0202-(0x01) 获取主机版本号(Link)
const HostController::HostQueryInfo HostController::const_hostAntaBoxVersion(0x01, 0x0202, QByteArray(1, '\x02'));              // 0x0202-(0x02) 获取主机版本号(Anta)
const HostController::HostQueryInfo HostController::const_hostKernelBuildDate(0x01, 0x0203, QByteArray(1, '\x00'));             // 0x0203-(0x00) 获取主机编译日期(Kernel)
const HostController::HostQueryInfo HostController::const_hostLinkBuildDate(0x01, 0x0203, QByteArray(1, '\x01'));               // 0x0203-(0x01) 获取主机编译日期(Link)
const HostController::HostQueryInfo HostController::const_hostAntaBoxBuildDate(0x01, 0x0203, QByteArray(1, '\x02'));            // 0x0203-(0x02) 获取主机编译日期(Anta)
const HostController::HostQueryInfo HostController::const_systemDatetime(0x01, 0x0204);                                         // 0x0204-(    ) 获取主机系统时间
const HostController::HostQueryInfo HostController::const_wirelessVolume(0x01, 0x0205);                                         // 0x0205-(    ) 获取无线音量
const HostController::HostQueryInfo HostController::const_wiredVolume(0x01, 0x0206);                                            // 0x0206-(    ) 获取有线音量
const HostController::HostQueryInfo HostController::const_antaBoxVolume(0x01, 0x0207);                                          // 0x0207-(    ) 获取天线盒音量
const HostController::HostQueryInfo HostController::const_speechMode(0x01, 0x0208);                                             // 0x0208-(    ) 获取发言模式
const HostController::HostQueryInfo HostController::const_maxSpeechUnits(0x01, 0x0209);                                         // 0x0209-(    ) 获取最大发言数量
const HostController::HostQueryInfo HostController::const_cameraTrackingProtocol(0x01, 0x020A);                                 // 0x020A-(    ) 获取摄像跟踪协议
const HostController::HostQueryInfo HostController::const_cameraTrackingAddress(0x01, 0x020B);                                  // 0x020B-(    ) 获取摄像跟踪地址
const HostController::HostQueryInfo HostController::const_cameraTrackingBaudrate(0x01, 0x020C);                                 // 0x020C-(    ) 获取摄像跟踪波特率
const HostController::HostQueryInfo HostController::const_wirelessAudioPath(0x01, 0x020D);                                      // 0x020D-(    ) 获取无线音频路径
const HostController::HostQueryInfo HostController::const_unitCapacity(0x01, 0x0210);                                           // 0x0210-(    ) 获取单元容量
const HostController::HostQueryInfo HostController::const_monitorAudioChannels(0x01, 0x0211);                                   // 0x0211-(    ) 获取监测音频信道数量
const HostController::HostQueryInfo HostController::const_currentWirelessSpeakerId(0x01,0x020E);                                // 0x020E-(    ) 获取当前无线发言Id
const HostController::HostQueryInfo HostController::const_currentWiredSpeakerId(0x01,0x020F);                                   // 0x020F-(    ) 获取当前有线发言Id


// 构造函数
HostController::HostController(quint8 address, SerialPortHandler *serialHandler,
                               ProtocolProcessor *protocolProcessor, QObject *parent)
    : QObject(parent),
    m_address(address),
    m_serialHandler(serialHandler),
    m_protocolProcessor(protocolProcessor),
    m_isInitialized(false),
    //m_currentInitCommandIndex(0),
    m_timeoutTimer(nullptr)
{
    // 只有在有缓存数据时，初始化时从缓存加载数据
    if (isDataCached()) {
        loadFromCache();
    }
}

// 析构函数
HostController::~HostController()
{
    saveToCache();
    delete m_timeoutTimer;
    m_timeoutTimer = nullptr;
}

/*********************************缓存配置文件*************************************/
// 1.1 获取主机缓存文件名
QString HostController::getCacheFileName() const
{
    // 首选程序安装目录下的 configs 文件夹
    QString cacheDir = QCoreApplication::applicationDirPath() + "/configs";

    // 检查是否有写入权限
    QDir dir(cacheDir);
    if (!dir.exists()) {
        if (!QDir().mkpath(cacheDir)) {
            // 如果没有权限在程序目录创建文件夹，回退到用户数据目录
            cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/configs";
            QDir().mkpath(cacheDir);
        }
    }
    return QString("%1/config_%2.ini").arg(cacheDir).arg(m_address,2,16, QChar('0'));
}

// 1.2 加载主机参数缓存文件 *** 需要根据需求增加数据源
void HostController::loadFromCache()
{
    QString configFile = getCacheFileName();
    if (QFile::exists(configFile)) {
        QSettings settings(configFile, QSettings::IniFormat);

        // ******* 从缓存加载所有字段
        m_hostKernelVersion = settings.value("hostKernelVersion", "").toString();                       // 0x0202-(0x00) 获取主机版本号(Kernel)
        m_hostLinkVersion = settings.value("hostLinkVersion", "").toString();                           // 0x0202-(0x01) 获取主机版本号(Link)
        m_hostAntaBoxVersion = settings.value("hostAntaBoxVersion", "").toString();                     // 0x0202-(0x02) 获取主机版本号(Anta)
        m_hostKernelBuildDate = settings.value("hostKernelBuildDate", "").toString();                   // 0x0203-(0x00) 获取主机编译日期(Kernel)
        m_hostLinkBuildDate = settings.value("hostLinkBuildDate", "").toString();                       // 0x0203-(0x01) 获取主机编译日期(Link)
        m_hostAntaBoxBuildDate = settings.value("hostAntaBoxBuildDate", "").toString();                 // 0x0203-(0x02) 获取主机编译日期(Anta)
        m_systemDatetime = settings.value("systemDatetime", "").toString();                             // 0x0204-(    ) 获取主机系统时间
        m_wirelessVolume = settings.value("wirelessVolume", "").toString();                             // 0x0205-(    ) 获取无线音量
        m_wiredVolume = settings.value("wiredVolume", "").toString();                                   // 0x0206-(    ) 获取有线音量
        m_antaBoxVolume = settings.value("antaBoxVolume", "").toString();                               // 0x0207-(    ) 获取天线盒音量
        m_speechMode = settings.value("speechMode", "").toString();                                     // 0x0208-(    ) 获取发言模式
        m_maxSpeechUnits = settings.value("maxSpeechUnits", "").toString();                             // 0x0209-(    ) 获取最大发言数量
        m_cameraTrackingProtocol = settings.value("cameraTrackingProtocol", "").toString();             // 0x020A-(    ) 获取摄像跟踪协议
        m_cameraTrackingAddress = settings.value("cameraTrackingAddress", "").toString();               // 0x020B-(    ) 获取摄像跟踪地址
        m_cameraTrackingBaudrate = settings.value("cameraTrackingBaudrate", "").toString();             // 0x020C-(    ) 获取摄像跟踪波特率
        m_wirelessAudioPath = settings.value("wirelessAudioPath", "").toString();                       // 0x020D-(    ) 获取无线音频路径
        m_unitCapacity = settings.value("unitCapacity", "").toString();                                 // 0x0210-(    ) 获取单元容量
        m_monitorAudioChannels = settings.value("monitorAudioChannels", "").toString();                 // 0x0211-(    ) 获取监测音频信道数量
        m_currentWirelessSpeakerId = settings.value("currentWirelessSpeakerId","").toString();          // 0x020E-(    ) 获取当前无线发言Id
        m_currentWiredSpeakerId = settings.value("currentWiredSpeakerId","").toString();                // 0x020F-(    ) 获取当前有线发言Id


        // ******* 发出所有相关信号
        emit signal_hostKernelVersion();                // 0x0202-(0x00) 获取主机版本号(Kernel)
        emit signal_hostLinkVersion();                  // 0x0202-(0x01) 获取主机版本号(Link)
        emit signal_hostAntaBoxVersion();               // 0x0202-(0x02) 获取主机版本号(Anta)
        emit signal_hostKernelBuildDate();              // 0x0203-(0x00) 获取主机编译日期(Kernel)
        emit signal_hostLinkBuildDate();                // 0x0203-(0x01) 获取主机编译日期(Link)
        emit signal_hostAntaBoxBuildDate();             // 0x0203-(0x02) 获取主机编译日期(Anta)
        emit signal_systemDatetime();                   // 0x0204-(    ) 获取主机系统时间
        emit signal_wirelessVolume();                   // 0x0205-(    ) 获取无线音量
        emit signal_wiredVolume();                      // 0x0206-(    ) 获取有线音量
        emit signal_antaBoxVolume();                    // 0x0207-(    ) 获取天线盒音量
        emit signal_speechMode();                       // 0x0208-(    ) 获取发言模式
        emit signal_maxSpeechUnits();                   // 0x0209-(    ) 获取最大发言数量
        emit signal_cameraTrackingProtocol();           // 0x020A-(    ) 获取摄像跟踪协议
        emit signal_cameraTrackingAddress();            // 0x020B-(    ) 获取摄像跟踪地址
        emit signal_cameraTrackingBaudrate();           // 0x020C-(    ) 获取摄像跟踪波特率
        emit signal_wirelessAudioPath();                // 0x020D-(    ) 获取无线音频路径
        emit signal_unitCapacity();                     // 0x0210-(    ) 获取单元容量
        emit signal_monitorAudioChannels();             // 0x0211-(    ) 获取监测音频信道数量
        emit signal_currentWirelessSpeakerId();         // 0x020E-(    ) 获取当前无线发言Id
        emit signal_currentWiredSpeakerId();            // 0x020F-(    ) 获取当前有线发言Id
    }
}

// 1.3 保存主机参数到缓存
void HostController::saveToCache()
{
    QString configFile = getCacheFileName();
    QSettings settings(configFile, QSettings::IniFormat);

    // ******* 下面添加需要保存的字段 *******
    settings.setValue("hostKernelVersion", m_hostKernelVersion);                    // 0x0202-(0x00) 获取主机版本号(Kernel)
    settings.setValue("hostLinkVersion", m_hostLinkVersion);                        // 0x0202-(0x01) 获取主机版本号(Link)
    settings.setValue("hostAntaBoxVersion", m_hostAntaBoxVersion);                  // 0x0202-(0x02) 获取主机版本号(Anta)
    settings.setValue("hostKernelBuildDate", m_hostKernelBuildDate);                // 0x0203-(0x00) 获取主机编译日期(Kernel)
    settings.setValue("hostLinkBuildDate", m_hostLinkBuildDate);                    // 0x0203-(0x01) 获取主机编译日期(Link)
    settings.setValue("hostAntaBoxBuildDate", m_hostAntaBoxBuildDate);              // 0x0203-(0x02) 获取主机编译日期(Anta)
    settings.setValue("systemDatetime", m_systemDatetime);                          // 0x0204-(    ) 获取主机系统时间
    settings.setValue("wirelessVolume", m_wirelessVolume);                          // 0x0205-(    ) 获取无线音量
    settings.setValue("wiredVolume", m_wiredVolume);                                // 0x0206-(    ) 获取有线音量
    settings.setValue("antaBoxVolume", m_antaBoxVolume);                            // 0x0207-(    ) 获取天线盒音量
    settings.setValue("speechMode", m_speechMode);                                  // 0x0208-(    ) 获取发言模式
    settings.setValue("maxSpeechUnits", m_maxSpeechUnits);                          // 0x0209-(    ) 获取最大发言数量
    settings.setValue("cameraTrackingProtocol", m_cameraTrackingProtocol);          // 0x020A-(    ) 获取摄像跟踪协议
    settings.setValue("cameraTrackingAddress", m_cameraTrackingAddress);            // 0x020B-(    ) 获取摄像跟踪地址
    settings.setValue("cameraTrackingBaudrate", m_cameraTrackingBaudrate);          // 0x020C-(    ) 获取摄像跟踪波特率
    settings.setValue("wirelessAudioPath", m_wirelessAudioPath);                    // 0x020D-(    ) 获取无线音频路径
    settings.setValue("unitCapacity", m_unitCapacity);                              // 0x0210-(    ) 获取单元容量
    settings.setValue("monitorAudioChannels", m_monitorAudioChannels);              // 0x0211-(    ) 获取监测音频信道数量
    settings.setValue("currentWirelessSpeakerId", m_currentWirelessSpeakerId);      // 0x020E-(    ) 获取当前无线发言Id
    settings.setValue("currentWiredSpeakerId", m_currentWiredSpeakerId);            // 0x020F-(    ) 获取当前有线发言Id


    // ******* 上面添加需要保存的字段 *******
    settings.sync();
}

// 1.4 判断主机参数是否已经缓存
bool HostController::isDataCached() const
{
    QString configFile = getCacheFileName();
    return QFile::exists(configFile);
}

/********************************调用串口发送数据***********************************/
// 2.1 调用串口发送数据
void HostController::sendToHost(const QByteArray &data)
{
    if (m_serialHandler  && m_serialHandler ->isOpen()) {
        m_serialHandler ->sendToPort(data);
    }
}


/*********************************发送指令的方法（单次与队列，使用参数与使用常量参数************************************/
// 通用主机信息查询函数， 从上位机软件发送指令到 m_address主机，执行opCode-command指令，可加载参数payload
// 3.1 通过参数构建帧查询（单次，已经内置m_address指定查询主机）
void HostController::query_hostInformation(uint8_t opCode, uint16_t command, const QByteArray &payload)
{
    QByteArray frame = m_protocolProcessor->buildFrame(
        m_address,          // 接收地址：使用当前HostController的地址
        0x80,               // 发送地址：固定为 0x80（表示本机）
        opCode,               // 操作类型：0x01
        command,             // 操作命令：0x0201（查询主机版本）
        payload //QByteArray()        // 无参数 payload
        );
    sendToHost(frame);
}

// 3.2 通过常量构建帧查询（单次，已经内置m_address指定查询主机）
void HostController::queryHostInfo(const HostQueryInfo& queryInfo)
{
    QByteArray frame = m_protocolProcessor->buildFrame(
        m_address,              // 接收地址：使用当前HostController的地址
        0x80,                   // 发送地址：固定为 0x80（表示本机）
        queryInfo.opCode,       // 操作类型
        queryInfo.command,      // 操作命令
        queryInfo.payload       // 参数
        );
    sendToHost(frame);
}

// 3.3 通过常量构建帧队列查询（批量）
void HostController::queryByCommandQueue()
{
    // 1. 先将所有指令加入队列
    for (int i = 1; i <= 4; i++) {
        QByteArray frame = m_protocolProcessor->buildFrame(
            i, 0x80, 0x01, 0x0201, QByteArray());
        m_serialHandler->enqueueCommand(frame);  // 仅入队，不触发发送
    }

    // 2. 显式启动发送过程
    m_serialHandler->startToSendCommandQueue();  // 现在开始按顺序发送队列中的指令
}

// 4.1 启动发送队列
void HostController::startSendingQueue()
{
    // 如果队列为空，直接返回
    if (m_sendQueue.isEmpty()) {
        qDebug() << "发送队列为空，无需发送";
        return;
    }

    // 如果超时定时器未创建，则创建它
    if (!m_timeoutTimer) {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, &HostController::handleTimeout);
    }

    // 发射指令队列变化信号
    emit signal_sendQueueChange();

    // 开始发送队列中的第一条指令
    sendNextQueuedCommand();
}

// 4.2 发送队列中的下一条指令
void HostController::sendNextQueuedCommand()
{
    if (!m_sendQueue.isEmpty()) {
        SendQueueItem item = m_sendQueue.head();

        // 使用delayMS延时发送指令
        QTimer::singleShot(item.delayMS, this, [this, item]() {
            QByteArray frame = m_protocolProcessor->buildFrame(
                item.address,
                0x80,           //（表示本机）
                item.queryInfo.opCode,
                item.queryInfo.command,
                item.queryInfo.payload
                );
            sendToHost(frame);

            // qDebug() << "["<< QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
            //          <<"] 队列发送指令: address=" << item.address
            //          <<", opCode=" << QString("0x%1").arg(item.queryInfo.opCode, 2, 16, QLatin1Char('0')).toUpper()
            //          <<", command=" << QString("0x%1").arg(item.queryInfo.command, 4, 16, QLatin1Char('0')).toUpper()
            //          <<", delay=" << item.delayMS << "ms"
            //          <<", 帧数据=" << frame.toHex(' ').toUpper();

            // 启用超时定时器，使用成员变量QUEUE_TIMEOUT_MS
            if(m_timeoutTimer){
                m_timeoutTimer->start(QUEUE_TIMEOUT_MS);
            }
        });
    } else {
        // 队列清空后的操作(打印提示，断开信号，删除定时器，打印队列完成提示信息/状态设置)
        // qDebug() << "队列指令发送完成";

        // 队列全部发送完后断开信号连接
        disconnect(this, &HostController::signal_responseReceived,
                   this, &HostController::handleResponseReceived);
        
        // 停止并删除超时定时器
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
            delete m_timeoutTimer;
            m_timeoutTimer = nullptr;
        }

        // 发射指令队列变化信号
        emit signal_sendQueueChange();

        // 打印队列完成提示信息
        if(!m_messageQueueSendCompleted.isEmpty()){
            qDebug() << m_messageQueueSendCompleted;

            // 如果是初始化完成，设置初始化标志
            if (m_messageQueueSendCompleted.contains("初始化完毕")) {
                m_isInitialized = true;
            }
            m_messageQueueSendCompleted.clear();
        } else {
            // qDebug() << "队列指令发送完成";
        }
    }
}

// 4.3 处理收到的响应
void HostController::handleResponseReceived(quint8 address, uint8_t opCode, uint16_t command)
{
    if(!m_sendQueue.isEmpty()){
        SendQueueItem currentItem = m_sendQueue.head();

        // 检查是否当前等待的响应（1.主机地址是否相同；2.操作命令是否相同。指令类型为0x04，请求之响应
        if(currentItem.address == address && currentItem.queryInfo.command == command && opCode == 0x04){
            // 响应成功,停止超时定时器
            if(m_timeoutTimer){
                m_timeoutTimer->stop();
            }

            // 从队列中移除已经处理的指令
            m_sendQueue.dequeue();

            // 发射指令队列变化信号
            emit signal_sendQueueChange();

            // 按照指令间隔时间处理下一条指令
            QTimer::singleShot(QUEUE_INTERVAL_MS, this, &HostController::sendNextQueuedCommand);
        } else { 
            // 非相匹配指令
            // qDebug() << "响应指令不匹配，收到的帧: address=" << address
            //         << ", opCode=0x" << QString::number(opCode, 16)
            //          << ", command=0x" << QString::number(command, 16);
        }
    }
}

// 4.4 队列发送过程中的处理超时
void HostController::handleTimeout()
{
    if(!m_sendQueue.isEmpty()){
        SendQueueItem currentItem = m_sendQueue.head();

        // 超时指令, 记入日志
        LogManager::instance()->addLog(QStringLiteral("指令（addr= %1, opCode= %2, cmd= %3）发送超时")
                                           .arg(currentItem.address, currentItem.queryInfo.opCode, currentItem.queryInfo.command), LogManager::LogLevel::WARNING); // 数据发送成功日志

        // 从队列中移除超时指令
        m_sendQueue.dequeue();

        // 发射指令队列变化信号
        emit signal_sendQueueChange();

        // 继续发送下一条指令
        sendNextQueuedCommand();
    }
}

void HostController::stopSendingQueue()
{
    if(!m_sendQueue.isEmpty()){
        m_sendQueue.clear();
        LogManager::instance()->addLog(QStringLiteral("指令队列已清空，停止发送指令"), LogManager::LogLevel::INFO); // 清空并停止指令发送队列

        // 发射指令队列变化信号
        emit signal_sendQueueChange();
    }
}

/*********************************具体主机功能，查询或者设置************************************/
// 1. 上位机软件初始化，启动时查询主机属性(多指令类型)
void HostController::initializeHostProperties()
{
    // ****** 开始测量响应时间****** //
    QElapsedTimer totalTimer;
    totalTimer.start();

    LogManager::instance()->addLog(QStringLiteral("执行初始化函数, 构建指令队列，初始化状态变量m_isInitialized = %1")
                                       .arg(m_isInitialized), LogManager::LogLevel::INFO); // 数据发送成功日志

    // 只有在未初始化时才执行初始化操作， 使用队列间隔时间变量QUEUE_INTERVAL_MS=10ms
    if (!m_isInitialized) {
        m_sendQueue.enqueue({m_address, const_hostKernelVersion,0});                                  // 0x0202-(0x00) 获取主机版本号(Kernel)
        m_sendQueue.enqueue({m_address, const_hostLinkVersion, QUEUE_INTERVAL_MS});                   // 0x0202-(0x01) 获取主机版本号(Link)
        m_sendQueue.enqueue({m_address, const_hostAntaBoxVersion, QUEUE_INTERVAL_MS});                // 0x0202-(0x02) 获取主机版本号(Anta)
        m_sendQueue.enqueue({m_address, const_hostKernelBuildDate, QUEUE_INTERVAL_MS});               // 0x0203-(0x00) 获取主机编译日期(Kernel)
        m_sendQueue.enqueue({m_address, const_hostLinkBuildDate, QUEUE_INTERVAL_MS});                 // 0x0203-(0x01) 获取主机编译日期(Link)
        m_sendQueue.enqueue({m_address, const_hostAntaBoxBuildDate, QUEUE_INTERVAL_MS});              // 0x0203-(0x02) 获取主机编译日期(Anta)
        m_sendQueue.enqueue({m_address, const_systemDatetime, QUEUE_INTERVAL_MS});                    // 0x0204-(    ) 获取主机系统时间
        m_sendQueue.enqueue({m_address, const_wirelessVolume, QUEUE_INTERVAL_MS});                    // 0x0205-(    ) 获取无线音量
        m_sendQueue.enqueue({m_address, const_wiredVolume, QUEUE_INTERVAL_MS});                       // 0x0206-(    ) 获取有线音量
        m_sendQueue.enqueue({m_address, const_antaBoxVolume, QUEUE_INTERVAL_MS});                     // 0x0207-(    ) 获取天线盒音量
        m_sendQueue.enqueue({m_address, const_speechMode, QUEUE_INTERVAL_MS});                        // 0x0208-(    ) 获取发言模式
        m_sendQueue.enqueue({m_address, const_maxSpeechUnits, QUEUE_INTERVAL_MS});                    // 0x0209-(    ) 获取最大发言数量
        m_sendQueue.enqueue({m_address, const_cameraTrackingProtocol, QUEUE_INTERVAL_MS});            // 0x020A-(    ) 获取摄像跟踪协议
        m_sendQueue.enqueue({m_address, const_cameraTrackingAddress, QUEUE_INTERVAL_MS});             // 0x020B-(    ) 获取摄像跟踪地址
        m_sendQueue.enqueue({m_address, const_cameraTrackingBaudrate, QUEUE_INTERVAL_MS});            // 0x020C-(    ) 获取摄像跟踪波特率
        m_sendQueue.enqueue({m_address, const_wirelessAudioPath, QUEUE_INTERVAL_MS});                 // 0x020D-(    ) 获取无线音频路径
        m_sendQueue.enqueue({m_address, const_unitCapacity, QUEUE_INTERVAL_MS});                      // 0x0210-(    ) 获取单元容量
        m_sendQueue.enqueue({m_address, const_monitorAudioChannels, QUEUE_INTERVAL_MS});              // 0x0211-(    ) 获取监测音频信道数量

        // m_sendQueue.enqueue({m_address, const_currentWirelessSpeakerId, QUEUE_INTERVAL_MS});          // 0x020E-(    ) 获取当前无线发言Id
        // m_sendQueue.enqueue({m_address,const_currentWiredSpeakerId,QUEUE_INTERVAL_MS});               // 0x020F-(    ) 获取当前有线发言Id

        // 设置队列完成提示信息
        m_messageQueueSendCompleted = "初始化完毕, m_isInitialized状态设置为真";

        // 连接响应接收信号到处理槽函数，如果响应操作命令与请求操作命令匹配，则无需超时等待
        connect(this, &HostController::signal_responseReceived,
                this, &HostController::handleResponseReceived);

        // 开始发送队列
        startSendingQueue();

    } else {
        qDebug() << "主机已经初始化，略过初始化。";
    }
}

// 2. 发送指令0x0212，请求监测通道信息
void HostController::requestMonitorChannelInfo()
{
    // 首先清空监测通道存储成员变量m_audioChannelData内容，以保证响应后能正确存储响应数据
    m_audioChannelData.clear();
    // 清空现有发送队列
    m_sendQueue.clear();

    // 确定实际使用的最大通道数
    int maxChannel = 19;
    if (!m_monitorAudioChannels.isEmpty() && m_monitorAudioChannels.toInt() > 0) {
        maxChannel = m_monitorAudioChannels.toInt() - 1;  // 转为索引形式（0-based）
    }

    // 构建发送指令帧，opCode =0x01, command = 0x0212, 
    // 构建发送指令帧，payload取值从0x00至maxChannel。如果m_monitorAudioChannels不等于0，则maxChannel = m_monitorAudioChannels的值（转为数字）；否则maxChannel= 默认值
    // 循环构建发送队列，根据payload的值将指令帧添加到发送队列
    for (int i = 0; i <= maxChannel ; ++i) {
        QByteArray payload(1, static_cast<char>(i));
        HostQueryInfo queryInfo(0x01, 0x0212, payload);
        SendQueueItem item0212 = {m_address, queryInfo, (i > 0) ? QUEUE_INTERVAL_MS : 0};       // 第一条指令延时为0，之后为QUEUE_INTERVAL_MS
        m_sendQueue.enqueue(item0212);
    }
    
    // 设置队列完成提示信息(这里并不打印)
    m_messageQueueSendCompleted = "监测通道信息读取完成";

    // 连接响应接收信号到处理槽函数，如果响应操作命令与请求操作命令匹配，则无需超时等待
    connect(this, &HostController::signal_responseReceived,
                this, &HostController::handleResponseReceived);

    // 启动发送指令
    startSendingQueue();
}

// 3. 发送指令0x0901，设置输出通道；发送指令0x0404，请求输出通道信息
void HostController::requestOutputChannelInfo()
{
    // 获取监测通道数据
    QVector<audioChannelInfo> monitorChannels = m_audioChannelData;

    // 如果没有监测通道数据，直接返回
    if (monitorChannels.isEmpty()) {
        qDebug() << "没有监测通道数据，请先获取监测通道信息";
        LogManager::instance()->addLog(QStringLiteral("没有监测通道数据，请先获取监测通道信息"), LogManager::LogLevel::WARNING); // 数据发送成功日志
        return;
    }

    // 清空现有发送队列
    m_sendQueue.clear();
    // 重置0x0404 响应计数器，（0x0404不带频点索引，在与0x0901协用时作为计数索引）
    m_AudioRSSIndex = 0;

    // 确定实际需要处理的通道数量
    int maxChannel = monitorChannels.size(); // 最多20个通道

    // 构建并添加指令到队列，总共maxChannel个通道。每个通道构建四组指令，每组指令由一个0x0901指令与一个0x0404组成
    // 首先构建0x0901的查询指令，指令由四个参数uid(2字节)，cid(1字节)，fid(1字节)，FREQ(4字节)组成：
    // uid固定预设为0x0001
    // 外层循环设定参数中的fid，从0x00，0x01，0x02，0x03，0x04，0x05，0x06，0x07，0x08，0x09，0x0A，0x0B，0x0C，0x0D，0x0E，0x0F，0x10，0x11，0x12，0x13，0x14
    // 设定参数中的FREQ，FREQ等于m_audioChannelData数据结构中的第fid个monitorFREQ
    // 内层循环设定参数中的cid 从0x01，0x02，0x03,0x04
    // 将一个0x0901指令加入队列后， delayMS = 20 ms；
    // 再构建一个0x0404指令。 0x0404指令为固定的指令，操作类型opCode=0x01，操作命令command=0x0404，无参数payload
    // 将一个0x0404指令加入队列，delayMS = 2000 ms
    for (int fid = 0; fid < maxChannel; ++fid) {
        if (fid >= monitorChannels.size()) {
            qDebug() << QString("警告: 频率索引 %1 超出监测通道数据范围").arg(fid);
            continue;
        }

        quint32 monitorFreq = monitorChannels[fid].monitorFREQ;
        
        for (int cid = 1; cid <= 4; ++cid) {
            QByteArray payload;
            // 单元两字节: 0x00 01
            payload.append(static_cast<char>(0x00));
            payload.append(static_cast<char>(0x01));
            // 通道一字节
            payload.append(static_cast<char>(cid));
            // 频率索引一字节
            payload.append(static_cast<char>(fid));
            // 频点四字节 (大端序)
            payload.append(static_cast<char>((monitorFreq >> 24) & 0xFF));
            payload.append(static_cast<char>((monitorFreq >> 16) & 0xFF));
            payload.append(static_cast<char>((monitorFreq >> 8) & 0xFF));
            payload.append(static_cast<char>(monitorFreq & 0xFF));

            // 构建0x0901指令，添加到队列，延时QUEUE_INTERVAL_MS = 20ms
            HostQueryInfo queryInfo(0x01, 0x0901, payload);
            m_sendQueue.enqueue({m_address, queryInfo, (fid == 0 && cid == 1) ? 0 : QUEUE_INTERVAL_MS});

            // 构建0x0404指令，添加到队列，延时QUEUE_DELAYED_MS = 2000ms
            // 55 0A F5 01 80 01 04 04 80 AA
            HostQueryInfo queryInfo0x0404(0x01, 0x0404);
            m_sendQueue.enqueue({m_address, queryInfo0x0404, QUEUE_DELAYED_MS});
        }
    }

    // 打印所有生成的指令用于检查
    qDebug() << "=== 生成的指令队列 ===";
    QQueue<SendQueueItem> tempQueue = m_sendQueue;  // 创建临时队列用于遍历
    int index = 0;
    while (!tempQueue.isEmpty()) {
        SendQueueItem item = tempQueue.dequeue();
        QByteArray frame = m_protocolProcessor->buildFrame(
            item.address,
            0x80,
            item.queryInfo.opCode,
            item.queryInfo.command,
            item.queryInfo.payload
        );
        qDebug() << "指令" << index << ": 地址=" << item.address
                 << ", 操作类型=0x" << QString::number(item.queryInfo.opCode, 16)
                 << ", 操作命令=0x" << QString::number(item.queryInfo.command, 16)
                 << ", 延时=" << item.delayMS << "ms"
                 << ", 帧数据=" << frame.toHex();
        index++;
    }
    qDebug() << "=== 指令队列结束 ===";

    // 设置队列完成提示信息
    m_messageQueueSendCompleted = "输出通道信息请求完成";

    // 连接响应接收信号到处理槽函数，如果响应操作命令与请求操作命令匹配，则无需超时等待
    connect(this, &HostController::signal_responseReceived,
                this, &HostController::handleResponseReceived);

    // 启动发送指令
    startSendingQueue();
}

// 4. 发送指令0x0601，查询“注册”单元信息
void HostController::detectingRegisteredUnits(quint32 max_UID)
{
    // 55 0C F3 01 80 01 06 01 00 01 86 AA
    // 检查队列是否正在使用
    if (!m_sendQueue.isEmpty()) {
        // 可以选择：
        // 1. 追加到现有队列
        // 2. 或者等待当前队列完成后再执行
        qDebug() << "队列正在使用中，将追加单元查询指令";
    }

    // 不清空队列，直接追加指令
    for (int uid = 0x0001; uid <= max_UID; ++uid) {
        QByteArray payload(2, 0);
        payload[0] = static_cast<char>((uid >> 8) & 0xFF);
        payload[1] = static_cast<char>(uid & 0xFF);

        HostQueryInfo queryInfo(0x01, 0x0601, payload);

        // 计算延时：如果队列为空，第一个指令延时为0
        int delay = (m_sendQueue.isEmpty() && uid == 0x0001) ? 0 : QUEUE_INTERVAL_MS;
        SendQueueItem item = {m_address, queryInfo, delay};
        m_sendQueue.enqueue(item);
    }

    // 如果队列之前为空，启动发送（否则已经在发送中）
    if (m_sendQueue.size() == (max_UID - 0x0001 + 1)) {
        m_messageQueueSendCompleted = QStringLiteral("单元信息查询完成，最大UID=%1").arg(max_UID, 4, 16, QLatin1Char('0'));

        connect(this, &HostController::signal_responseReceived,
                this, &HostController::handleResponseReceived);

        if (m_sendQueue.size() == (max_UID - 0x0001 + 1)) {
            startSendingQueue();
        }
    }
}

// 5. 发送指令0x0602， 通过查询单元别名，确认单元是否开机


// *********************************主机设置函数************************************
// 0x0104-(38 6C D3 00) 设置主机系统时间
void HostController::hostSet_systemDatetime(const QDateTime& dt)
{
    if (dt.isValid()) {
        // 基准日期：1970年1月1日 00:00:00 UTC
        QDateTime baseDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), QTimeZone::utc());

        // 将日期时间转换为时间戳
        quint32 timestamp = baseDateTime.secsTo(dt);

        // 将时间戳转换为4字节数据（大端序）
        QByteArray payload(4, 0);
        payload[0] = static_cast<char>((timestamp >> 24) & 0xFF);
        payload[1] = static_cast<char>((timestamp >> 16) & 0xFF);
        payload[2] = static_cast<char>((timestamp >> 8) & 0xFF);
        payload[3] = static_cast<char>(timestamp & 0xFF);

        // 构造并发送设置系统时间的命令
        HostQueryInfo queryInfo(0x01, 0x0104, payload);
        queryHostInfo(queryInfo);
    } else {
        qDebug() << "Invalid datetime!";
    }
}

// 0x0108-(    ) 设置发言模式
void HostController::hostSet_speechMode(const QString& para_speechMode)
{
    QByteArray payload(1, 0);
    if (para_speechMode == "FIFO"){
        payload[0]=0x00;
    }else if (para_speechMode == "Self Lock") {
        payload[0]=0x01;
    }

    // 构造并发送设置发言模式的命令
    HostQueryInfo queryInfo(0x01, 0x0108, payload);
    queryHostInfo(queryInfo);
}

// 0x0109-(    ) 设置最大发言数量
void HostController::hostSet_maxSpeechUnits(quint8 para_maxSpeechUnits)
{
    QByteArray payload(1, 0);

    // 构造并发送设置最大发言数量的命令
    payload[0]= para_maxSpeechUnits;
    HostQueryInfo queryInfo(0x01, 0x0109, payload);
    queryHostInfo(queryInfo);
}

// 0x0105-(    ) 设置无线音量
void HostController::hostSet_wirelessVolume(quint8 para_wirelessVolume)
{
    QByteArray payload(1, 0);

    // 构造并发送设置无线音量的命令
    payload[0]= para_wirelessVolume;
    HostQueryInfo queryInfo(0x01, 0x0105, payload);
    queryHostInfo(queryInfo);

    // 构建完整帧并打印用于调试
    // QByteArray frame = m_protocolProcessor->buildFrame(m_address, 0x80, queryInfo.opCode, queryInfo.command, queryInfo.payload);
    // qDebug() << "  Frame:" << frame.toHex(' ').toUpper();
}

// 0x0106-(    ) 设置有线音量
void HostController::hostSet_wiredVolume(quint8 para_wiredVolume)
{
    QByteArray payload(1, 0);

    // 构造并发送设置有线音量的命令
    payload[0]= para_wiredVolume;
    HostQueryInfo queryInfo(0x01, 0x0106, payload);
    queryHostInfo(queryInfo);
}

// 0x0107-(    ) 设置天线盒音量
void HostController::hostSet_antaBoxVolume(quint8 para_antaBoxVolume)
{
    QByteArray payload(1, 0);

    // 构造并发送设置天线盒音量的命令
    payload[0]= para_antaBoxVolume;
    HostQueryInfo queryInfo(0x01, 0x0107, payload);
    queryHostInfo(queryInfo);
}

// 0x010D-(    ) 设置无线音频路径
void HostController::hostSet_wirelessAudioPath(const QString& para_wirelessAudioPath)
{
    QByteArray payload(1, 0);
    if (para_wirelessAudioPath == "Host"){
        payload[0]=0x00;
    }else if (para_wirelessAudioPath == "Antenna Box") {
        payload[0]=0x01;
    }

    // 构造并发送设置无线音频路径的命令
    HostQueryInfo queryInfo(0x01, 0x010D, payload);
    queryHostInfo(queryInfo);
}

// 0x010B-(    ) 设置摄像跟踪地址
void HostController::hostSet_cameraTrackingAddress(quint16 para_cameraTrackingAddress)
{
    QByteArray payload(1, 0);

    // 构造并发送设置天线盒音量的命令
    payload[0]= para_cameraTrackingAddress;
    HostQueryInfo queryInfo(0x01, 0x010B, payload);
    queryHostInfo(queryInfo);
}

// 0x010A-(    ) 设置摄像跟踪协议
void HostController::hostSet_cameraTrackingProtocol(const QString& para_cameraTrackingProtocol)
{
    QByteArray payload(1, 0);
    if (para_cameraTrackingProtocol == "NONE"){
        payload[0]=0x00;
    }else if (para_cameraTrackingProtocol == "VISCA") {
        payload[0]=0x01;
    }else if (para_cameraTrackingProtocol == "PELCO_D") {
        payload[0]=0x02;
    }else if (para_cameraTrackingProtocol == "PELCO_P") {
        payload[0]=0x03;
    }

    // 构造并发送设置无线音频路径的命令
    HostQueryInfo queryInfo(0x01, 0x010A, payload);
    queryHostInfo(queryInfo);
}

// 0x010C-(    ) 设置摄像跟踪波特率
void HostController::hostSet_cameraTrackingBaudrate(quint32 para_cameraTrackingBaudrate)
{
    QByteArray payload(1, 0);
    if (para_cameraTrackingBaudrate == 2400){
        payload[0]=0x00;
    }else if (para_cameraTrackingBaudrate == 4800) {
        payload[0]=0x01;
    }else if (para_cameraTrackingBaudrate == 9600) {
        payload[0]=0x02;
    }else if (para_cameraTrackingBaudrate == 115200) {
        payload[0]=0x03;
    }

    // 构造并发送设置无线音频路径的命令
    HostQueryInfo queryInfo(0x01, 0x010C, payload);
    queryHostInfo(queryInfo);
}

// 0x0503-(    ) 删除单元配对
void HostController::unitSet_Remove(quint32 uid)
{
    // 55 0C F3 01 80 01 05 03 00 01 87 AA
    QByteArray payload(2, 0);

    // 将uid转换为两个字节（大端序）
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);  // 高字节
    payload[1] = static_cast<char>(uid & 0xFF);         // 低字节

    // 构造并发送删除单元配对命令
    HostQueryInfo queryInfo(0x01, 0x0503, payload);
    queryHostInfo(queryInfo);
}

// 0x0504-(    ) 打开单元发言
void HostController::unitSet_On(quint32 uid)
{
    // 55 0C F3 01 80 01 05 04 00 01 80 AA
    QByteArray payload(2, 0);

    // 将uid转换为两个字节（大端序）
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);  // 高字节
    payload[1] = static_cast<char>(uid & 0xFF);         // 低字节

    // 构造并发送删除单元配对命令
    HostQueryInfo queryInfo(0x01, 0x0504, payload);
    queryHostInfo(queryInfo);

}

// 0x0505-(    ) 关闭单元静音
void HostController::unitSet_Off(quint32 uid)
{
    // 55 0C F3 01 80 01 05 05 00 01 81 AA
    QByteArray payload(2, 0);

    // 将uid转换为两个字节（大端序）
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);  // 高字节
    payload[1] = static_cast<char>(uid & 0xFF);         // 低字节

    // 构造并发送删除单元配对命令
    HostQueryInfo queryInfo(0x01, 0x0505, payload);
    queryHostInfo(queryInfo);

}

// 0x0501-(    ) 设置单元属性（role, uStatus, uResponse）
void HostController::unitSet_Info(quint32 uid, quint32 uAddr, quint8 uRole, quint8 uType, quint8 uStatus, quint8 uResponse)
{
    // 55 14 EB 01 80 01 05 01 00 01 AF 60 95 18 30 20 00 00 D7 AA
    // 55 14 EB 01 80 01 05 01 (00 01) (AF 60 95 18) (30) (20) (00) (00) D7 AA

    QByteArray payload(10, 0);

    // 将uid转换为两个字节（大端序）
    payload[0] = static_cast<char>((uid >> 8) & 0xFF);   // UID高字节
    payload[1] = static_cast<char>(uid & 0xFF);          // UID低字节

    // 将uaddr转换为四个字节（大端序）
    payload[2] = static_cast<char>((uAddr >> 24) & 0xFF); // 地址最高字节
    payload[3] = static_cast<char>((uAddr >> 16) & 0xFF); // 地址次高字节
    payload[4] = static_cast<char>((uAddr >> 8) & 0xFF);  // 地址次低字节
    payload[5] = static_cast<char>(uAddr & 0xFF);         // 地址最低字节

    // uRole为一个字节
    payload[6] = static_cast<char>(uRole);

    // uType
    payload[7] = static_cast<char>(uType);

    // uStatus
    payload[8] = static_cast<char>(uStatus);

    // uResponse
    payload[9] = static_cast<char>(uResponse);


    // 构造并发送设置单元角色命令
    HostQueryInfo queryInfo(0x01, 0x0501, payload);
    queryHostInfo(queryInfo);

}


/*********************************保存响应信息至变量************************************/
// 供ProtocolProcesspr:: dispatchFrame()调用，给成员变量赋值并发射信号
// 0x0202-(0x00) 获取主机版本号(Kernel)
void HostController::save_hostKernelVersion( const QString&  version)
{
    m_hostKernelVersion = version;
    emit signal_hostKernelVersion();
    // qDebug() << "m_hostKernelVersion = " << m_hostKernelVersion;
}

// 0x0202-(0x01) 获取主机版本号(Link)
void HostController::save_hostLinkVersion( const QString&  version)
{
    m_hostLinkVersion = version;
    emit signal_hostLinkVersion();
    // qDebug() << "m_hostLinkVersion = " << m_hostLinkVersion;
}

// 0x0202-(0x02) 获取主机版本号(Anta)
void HostController::save_hostAntaBoxVersion( const QString&  version)
{
    m_hostAntaBoxVersion = version;
    emit signal_hostAntaBoxVersion();
    // qDebug() << "m_hostAntaBoxVersion = " << m_hostAntaBoxVersion;
}

// 0x0203-(0x00) 获取主机编译日期(Kernel)
void HostController::save_hostKernelBuildDate( const QString&  date)
{
    m_hostKernelBuildDate = date;
    emit signal_hostKernelBuildDate();
    // qDebug() << "m_hostKernelBuildDate = " << m_hostKernelBuildDate;
}

// 0x0203-(0x01) 获取主机编译日期(Link)
void HostController::save_hostLinkBuildDate( const QString&  date)
{
    m_hostLinkBuildDate = date;
    emit signal_hostLinkBuildDate();
    // qDebug() << "m_hostLinkBuildDate = " << m_hostLinkBuildDate;
}

// 0x0203-(0x02) 获取主机编译日期(Anta)
void HostController::save_hostAntaBoxBuildDate( const QString&  date)
{
    m_hostAntaBoxBuildDate = date;
    emit signal_hostAntaBoxBuildDate();
    // qDebug() << "m_hostAntaBoxBuildDate = " << m_hostAntaBoxBuildDate;
}

// 0x0204-(    ) 获取主机系统时间
void HostController::save_systemDatetime( const QString&  datetime)
{
    m_systemDatetime = datetime;
    emit signal_systemDatetime();
    // qDebug() << "m_systemDatetime = " << m_systemDatetime;
}

// 0x0205-(    ) 获取无线音量
void HostController::save_wirelessVolume( const QString&  volume)
{
    m_wirelessVolume = volume;
    emit signal_wirelessVolume();
    // qDebug() << "m_wirelessVolume = " << m_wirelessVolume;
}

// 0x0206-(    ) 获取有线音量
void HostController::save_wiredVolume( const QString&  volume)
{
    m_wiredVolume = volume;
    emit signal_wiredVolume();
    // qDebug() << "m_wiredVolume = " << m_wiredVolume;
}

// 0x0207-(    ) 获取天线盒音量
void HostController::save_antaBoxVolume( const QString&  volume)
{
    m_antaBoxVolume = volume;
    emit signal_antaBoxVolume();
    // qDebug() << "m_antaBoxVolume = " << m_antaBoxVolume;
}

// 0x0208-(    ) 获取发言模式
void HostController::save_speechMode( const QString&  mode)
{
    m_speechMode = mode;
    emit signal_speechMode();
    // qDebug() << "m_speechMode = " << m_speechMode;
}

// 0x0209-(    ) 获取最大发言数量
void HostController::save_maxSpeechUnits( const QString&  maxSpeaker)
{
    m_maxSpeechUnits = maxSpeaker;
    emit signal_maxSpeechUnits();
    // qDebug() << "m_maxSpeechUnits = " << m_maxSpeechUnits;
}

// 0x020A-(    ) 获取摄像跟踪协议
void HostController::save_cameraTrackingProtocol( const QString&  camProtocol)
{
    m_cameraTrackingProtocol = camProtocol;
    emit signal_cameraTrackingProtocol();
    // qDebug() << "m_cameraTrackingProtocol = " << m_cameraTrackingProtocol;
}

// 0x020B-(    ) 获取摄像跟踪地址
void HostController::save_cameraTrackingAddress( const QString&  camAddress)
{
    m_cameraTrackingAddress = camAddress;
    emit signal_cameraTrackingAddress();
    // qDebug() << "m_cameraTrackingAddress = " << m_cameraTrackingAddress;
}

// 0x020C-(    ) 获取摄像跟踪波特率
void HostController::save_cameraTrackingBaudrate( const QString&  camBaudrate)
{
    m_cameraTrackingBaudrate = camBaudrate;
    emit signal_cameraTrackingBaudrate();
    // qDebug() << "m_cameraTrackingBaudrate = " << m_cameraTrackingBaudrate;
}

// 0x020D-(    ) 获取无线音频路径
void HostController::save_wirelessAudioPath( const QString&  audioPath)
{
    m_wirelessAudioPath = audioPath;
    emit signal_wirelessAudioPath();
    // qDebug() << "m_wirelessAudioPath = " << m_wirelessAudioPath;
}

// 0x020E-(    ) 获取当前无线发言Id
void HostController::save_currentWirelessSpeakerId( const QString&  speakerId)
{
    m_currentWirelessSpeakerId = speakerId;
    emit signal_currentWirelessSpeakerId();
    // qDebug() << "m_currentWirelessSpeakerId = " << m_currentWirelessSpeakerId;
}

// 0x020F-(    ) 获取当前有线发言Id
void HostController::save_currentWiredSpeakerId( const QString&  speakerId)
{
    m_currentWiredSpeakerId = speakerId;
    emit signal_currentWiredSpeakerId();
    // qDebug() << "m_currentWiredSpeakerId = " << m_currentWiredSpeakerId;
}

// 0x0210-(    ) 获取单元容量
void HostController::save_unitCapacity( const QString&  unitCapacity)
{
    m_unitCapacity = unitCapacity;
    emit signal_unitCapacity();
    // qDebug() << "m_unitCapacity = " << m_unitCapacity;
}

// 0x0211-(    ) 获取监测音频信道数量
void HostController::save_monitorAudioChannels( const QString&  audioChannelCount)
{
    m_monitorAudioChannels = audioChannelCount;
    emit signal_monitorAudioChannels();
    // qDebug() << "m_monitorAudioChannels = " << m_monitorAudioChannels;
}

// 0x0401-(    ) 获取无线报文统计
void HostController::save_wirelessPacketStatistics( const QString&  packetStatistics)
{
    m_wirelessPacketStatistics = packetStatistics;
    emit signal_wirelessPacketStatistics();
    // qDebug() << "m_wirelessPacketStatistics = " << m_wirelessPacketStatistics;
}

// 0x0402-(    ) 获取有线报文统计
void HostController::save_wiredPacketStatistics( const QString&  packetStatistics)
{
    m_wiredPacketStatistics = packetStatistics;
    emit signal_wiredPacketStatistics();
    // qDebug() << "m_wiredPacketStatistics = " << m_wiredPacketStatistics;
}

// 0x0403-(    ) 获取天线盒报文统计
void HostController::save_antaBoxPacketStatistics( const QString&  packetStatistics)
{
    m_antaBoxPacketStatistics = packetStatistics;
    emit signal_antaBoxPacketStatistics();
    // qDebug() << "m_antaBoxPacketStatistics = " << m_antaBoxPacketStatistics;
}

// 0x0601-(    ) 获取“注册”单元信息
void HostController::save_unitInfo( const quint32 uid, quint32 uAddr, quint8 uRole, quint8 uType, quint8 uStatus, quint8 uResponse)
{
    // 创建新的单元信息结构体
    unitInformation unit;
    unit.uID = uid;
    unit.uAddr = uAddr;
    unit.uRole = uRole;
    unit.uType = uType;
    unit.uStatus = uStatus;
    unit.uResponse = uResponse;

    // 检查是否已存在相同 uID 的单元，如果存在则更新，否则添加新单元
    bool found = false;
    for (int i = 0; i < m_unitInfo.size(); ++i) {
        if (m_unitInfo[i].uID == unit.uID) {
            m_unitInfo[i] = unit;
            found = true;
            break;
        }
    }

    if (!found) {
        m_unitInfo.append(unit);
    }

    emit signal_unitInfo();
}

// 0x0602-(    ) 获取单元别名
void HostController::save_unitAlias( quint16 uID, const QString& alias)
{
    // 插入或更新单元别名映射记录
    m_unitAliasMap.insert(uID, alias);
    emit signal_unitAlias();
    // qDebug() << "m_unitAliasMap[" << uID << "] = " << alias;
}

// 0x0606-(    ) 获取无线单元状态信息
void HostController::save_unitWirelessStatus( const QString&  status)
{
    m_unitWirelessStatus = status;
    emit signal_unitWirelessStatus();
    // qDebug() << "m_unitWirelessStatus = " << m_unitWirelessStatus;
}

// 0x0801-(    ) 获取会议名称
void HostController::save_meetingName( const QString&  name)
{
    m_meetingName = name;
    emit signal_meetingName();
    // qDebug() << "m_meetingName = " << m_meetingName;
}

// 0x0212-(    ) 获取监测音频信道信息
void HostController::save_monitorAudioChannelInfo(const audioChannelInfo& info)
{
    QMutexLocker locker(&m_audioChannelDataMutex);

    m_audioChannelData.append(info);

    // 发射信号通知界面数据已更新
    emit signal_audioChannelInfo();
    // qDebug() << "发射 monitor 信号";
}

// 0x0404-(    ) 获取音频输出通道RSSI
void HostController::save_outputAudioChannelInfoRSSI(const QByteArray rssiData)
{
    // 使用互斥锁保护访问m_audioChannelData
    QMutexLocker locker(&m_audioChannelDataMutex);

    // 按顺序存储，使用m_AudioRSSIndex作为索引计数器
    // 0x0404响应计数器。计数值应该在0-79， 0-3对应频点索引00，通道1-4； 4-7对应频点索引01，通道1-4；8-11对应频点索引03，通道1-4；依此类推对应80次指令的值。
    if (m_AudioRSSIndex >= 0 ) {
        // 0x0404与0x0901协作使用： 首先计算当前应该存储到哪个索引位置（fid，cid）
        int fid = (m_AudioRSSIndex) / 4;  // 每4个响应对应一个通道索引
        int cid = (m_AudioRSSIndex) % 4; // 在当前通道中的偏移(0-3对应通道1-4)

        // 0x0404与0x0901协作使用，仅不为零（或者cid）对应通道值有用
        qint8 rssi = static_cast<qint8>(rssiData.at(cid+1));

        switch(cid) {
            case 0:
                m_audioChannelData[fid].outputRSSI1 = rssi;
                break;
            case 1:
                m_audioChannelData[fid].outputRSSI2 = rssi;
                break;
            case 2:
                m_audioChannelData[fid].outputRSSI3 = rssi;
                break;
            case 3:
                m_audioChannelData[fid].outputRSSI4 = rssi;
                break;
        }

        qDebug() << "0x0404 接收响应：" << rssiData;
        m_AudioRSSIndex++;
        qDebug() << "0x0404 计数器：" << m_AudioRSSIndex;

        emit signal_audioChannelInfo();
        qDebug() << "发射 RSSI 信号";

    } else {
        // 普通0x0404中四个通道值都有用，从rssiData中提取4个通道的RSSI值
        // 普通0x0404，m_AudioRSSIndex为-1，并不递增
        qint8 rssi1 = static_cast<qint8>(rssiData.at(0));
        qint8 rssi2 = static_cast<qint8>(rssiData.at(1));
        qint8 rssi3 = static_cast<qint8>(rssiData.at(2));
        qint8 rssi4 = static_cast<qint8>(rssiData.at(3));
        m_wirelessAudioChannel = QString("%1,%2,%3,%4")
                                    .arg(rssi1)
                                    .arg(rssi2)
                                    .arg(rssi3)
                                    .arg(rssi4);
    }
}

// 0x0901-(    ) 获取音频输出通道FREQ
void HostController::save_outputAudioChannelInfoFREQ(const quint8 fid, quint8 cid, quint32 freq)
{
    // 使用互斥锁保护访问m_audioChannelData
    QMutexLocker locker(&m_audioChannelDataMutex);

    // 根据返回fid， cid及freq， 查找匹配索引的记录并更新freq给outputFREQ
    switch(cid) {
    case 1:
        m_audioChannelData[fid].outputFREQ1 = freq;
        break;
    case 2:
        m_audioChannelData[fid].outputFREQ2 = freq;
        break;
    case 3:
        m_audioChannelData[fid].outputFREQ3 = freq;
        break;
    case 4:
        m_audioChannelData[fid].outputFREQ4 = freq;
        break;
    }
    // 发射信号通知界面数据已更新
    emit signal_audioChannelInfo();
    qDebug() << "发射 FREQ 信号";
}

// 0x0405-(    ) 获取无线管理信道RSSI
void HostController::save_wirelessMgmtChannel( const QString&  mgmtChannel)
{
    qDebug() << "待添加";
}

/*********************************信号槽函数，提供属性给QML（不能使用内联函数的放这里）************************************/
// 0x0212-(    ) 获取监测音频信道信息, 0x0404-(    ) 获取无线音频信道RSSI. 将音频通道数据转换为QVariantList格式，用于QML列表控件绑定
QVariantList HostController::m_audioChannelDataToQVariantList() const
{
    QVariantList list;
    for (const auto& channelInfo : m_audioChannelData) {
        QVariantMap channelMap;
        channelMap["index"] = channelInfo.index;
        channelMap["monitorFREQ"] = static_cast<quint32>(channelInfo.monitorFREQ);
        channelMap["monitorRSSI"] = channelInfo.monitorRSSI;
        channelMap["outputFREQ1"] = static_cast<quint32>(channelInfo.outputFREQ1);
        channelMap["outputFREQ2"] = static_cast<quint32>(channelInfo.outputFREQ2);
        channelMap["outputFREQ3"] = static_cast<quint32>(channelInfo.outputFREQ3);
        channelMap["outputFREQ4"] = static_cast<quint32>(channelInfo.outputFREQ4);
        channelMap["outputRSSI1"] = channelInfo.outputRSSI1;
        channelMap["outputRSSI2"] = channelInfo.outputRSSI2;
        channelMap["outputRSSI3"] = channelInfo.outputRSSI3;
        channelMap["outputRSSI4"] = channelInfo.outputRSSI4;

        list.append(channelMap);
    }
    return list;
    qDebug() << "存储到列表m_audioChannelDataToQVariantList";
}

// 0x0602-(    ) 获取单元别名. 将单元信息数据转为QVariantList格式，用于QML列表控件绑定
QVariantList HostController::m_unitInfoToVariantList() const
{
    QVariantList result;
    for (const auto& unit : m_unitInfo) {
        QVariantMap unitMap;
        unitMap["uID"] = unit.uID;
        unitMap["uAddr"] = unit.uAddr;
        unitMap["uRole"] = unit.uRole;
        unitMap["uType"] = unit.uType;
        unitMap["uStatus"] = unit.uStatus;
        unitMap["uResponse"] = unit.uResponse;

        // 根据身份值获取身份描述
        switch (unit.uRole) {
        case 0x10:
            unitMap["roleName"] = "列席";
            break;
        case 0x20:
            unitMap["roleName"] = "贵宾";
            break;
        case 0x30:
            unitMap["roleName"] = "主席";
            break;
        }

        // 根据类型值获取类型描述
        switch (unit.uType) {
        case 0x02:
            unitMap["typeName"] = "有线座麦";
            break;
        case 0x20:
            unitMap["typeName"] = "无线座麦";
            break;
        case 0x22:
            unitMap["typeName"] = "有线无线融合座麦";
            break;
        case 0x40:
            unitMap["typeName"] = "无线手持";
            break;
        }

        // 根据类型值获取类型描述
        switch (unit.uStatus) {
        case 0x01:
            unitMap["unitStatus"] = "在线";
            break;
        case 0x02:
            unitMap["unitStatus"] = "离线";
            break;
        }

        // 根据类型值获取类型描述
        switch(unit.uResponse){
        case 0x01:
            unitMap["unitResponse"] = "已响应";
            break;
        case 0x02:
            unitMap["unitResponse"] = "未响应";
            break;
        }

        result.append(unitMap);
    }
    return result;
}

// 0x0602-(    ) 获取单元别名. 将单元别名映射转为 QVariantMap 的函数
QVariantMap HostController::m_unitAliasMapToVariantMap() const
{
    QVariantMap result;
    for (auto it = m_unitAliasMap.constBegin(); it != m_unitAliasMap.constEnd(); ++it) {
        qDebug() << "Key:" << it.key() << "Value:" << it.value();
        result.insert(QString::number(it.key()), it.value());
    }
    return result;
}

/*********************************测试函数************************************/
// 添加以下函数实现
void HostController::printAudioChannelData() const
{
    // QMutexLocker locker(&m_audioChannelDataMutex); // 添加互斥锁保护

    qDebug() << "=== 音频通道数据 ===";
    qDebug() << "总通道数:" << m_audioChannelData.size();

    for (int i = 0; i < m_audioChannelData.size(); ++i) {
        const audioChannelInfo& channel = m_audioChannelData[i];
        qDebug() << "通道" << i << ":";
        qDebug() << "  索引号:" << channel.index;
        qDebug() << "  监测频率:" << channel.monitorFREQ << "Hz";
        qDebug() << "  监测信号强度:" << channel.monitorRSSI << "dBm";
        qDebug() << "  输出通道1频率:" << channel.outputFREQ1 << "Hz";
        qDebug() << "  输出通道1信号强度:" << channel.outputRSSI1 << "dBm";
        qDebug() << "  输出通道2频率:" << channel.outputFREQ2 << "Hz";
        qDebug() << "  输出通道2信号强度:" << channel.outputRSSI2 << "dBm";
        qDebug() << "  输出通道3频率:" << channel.outputFREQ3 << "Hz";
        qDebug() << "  输出通道3信号强度:" << channel.outputRSSI3 << "dBm";
        qDebug() << "  输出通道4频率:" << channel.outputFREQ4 << "Hz";
        qDebug() << "  输出通道4信号强度:" << channel.outputRSSI4 << "dBm";
    }
    qDebug() << "=== 数据结束 ===";
}

void HostController::testAudioChannelDataDisplay()
{
    // 清空现有数据
    m_audioChannelData.clear();

    // 为 m_audioChannelData 赋固定值用于测试界面显示
    // 假设有最多8个音频通道用于测试
    for (int i = 0; i < 10; ++i) {
        audioChannelInfo channelInfo;
        channelInfo.index = i;
        channelInfo.monitorFREQ = 500000000 + i * 1000000;  // 500MHz起，每通道间隔1MHz
        channelInfo.monitorRSSI = -60 - i * 2;              // 从-60dBm开始递减
        channelInfo.outputFREQ1 = 510000000 + i * 1000000;  // 输出频率1
        channelInfo.outputFREQ2 = 511000000 + i * 1000000;  // 输出频率2
        channelInfo.outputFREQ3 = 512000000 + i * 1000000;  // 输出频率3
        channelInfo.outputFREQ4 = 513000000 + i * 1000000;  // 输出频率4
        channelInfo.outputRSSI1 = -55 - i;                  // 输出RSSI1
        channelInfo.outputRSSI2 = -56 - i;                  // 输出RSSI2
        channelInfo.outputRSSI3 = -57 - i;                  // 输出RSSI3
        channelInfo.outputRSSI4 = -58 - i;                  // 输出RSSI4

        m_audioChannelData.append(channelInfo);
    }

    // 发射信号通知界面数据已更新
    emit signal_audioChannelInfo();

    // 可选：打印数据以验证
    printAudioChannelData();
}
