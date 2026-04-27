#include "HostManager.h"
#include "../hardware/ConnectionFactory.h"
#include "../library/LogManager.h"
#include "xdcController.h"
#include "afhController.h"
#include <QDebug>
#include <QtGlobal>
#include <QThread>
#include <QQmlEngine>


HostManager::HostManager(ConnectionFactory* factory, QObject *parent)
    : QObject(parent)
    , m_factory(factory)
    , m_timeoutMs(3000)      // 使用默认值初始化
    , m_maxRetries(3)    // 使用默认值初始化
    , m_selectedHostAddress(0) // [新增] 初始化选中地址为 0
    , m_selectedHostMap(new QQmlPropertyMap(this)) // [新增] 初始化映射表
{
    LOG_INFO("HostManager 初始化完成");

    // 初始化 selectedHostMap 的默认值
    m_selectedHostMap->insert("address", "N/A");
    m_selectedHostMap->insert("isActive", false);

    // 标记为QML上下文属性（可选）
    QQmlEngine::setObjectOwnership(m_selectedHostMap, QQmlEngine::CppOwnership);

    if (!m_factory) {
        LOG_WARNING("ConnectionFactory 实例为空");
    }
    connectToFactorySignals();    // 初始连接适配器信号
}

HostManager::~HostManager()
{
    LOG_INFO(QString("HostManager 析构开始 - 控制器数量: %1").arg(m_controllers.size()));
    disconnectFromFactorySignals();

    // 清理所有控制器实例
    for (auto it = m_controllers.constBegin(); it != m_controllers.constEnd(); ++it) {
        if (it.value()) {
            it.value()->deleteLater();
        }
    }
    m_controllers.clear();

    // 清理主机信息
    m_hostInfoMap.clear();

    LOG_INFO("HostManager 析构完成");
}

// ===== factory信号连接方法 =====
void HostManager::connectToFactorySignals()
{
    if (m_factory) {
        connect(m_factory, &ConnectionFactory::factoryDataReceived,
                this, &HostManager::onFactoryDataReceived);
        connect(m_factory, &ConnectionFactory::errorOccurred,
                this, &HostManager::onFactoryErrorOccurred);

        LOG_INFO("成功连接 Factory 信号");
    }
}

void HostManager::disconnectFromFactorySignals()
{
    if (m_factory) {
        disconnect(m_factory, &ConnectionFactory::factoryDataReceived,
                   this, &HostManager::onFactoryDataReceived);
        disconnect(m_factory, &ConnectionFactory::errorOccurred,
                   this, &HostManager::onFactoryErrorOccurred);

        LOG_INFO("已断开 Factory 信号连接");
    }
}

bool HostManager::isConnected() const
{
    return m_factory ? m_factory->isFactoryConnected() : false;
}


// ===== 核心数据发送接口 =====
bool HostManager::sendRawData(const QByteArray &data)
{
    if (data.isEmpty())
    {
        LOG_WARNING("发送数据为空");
        emit errorOccurred("发送数据为空");
        return false;
    }

    if (!m_factory || !m_factory->isFactoryConnected())
    {
        LOG_WARNING("适配器未连接，无法发送数据");
        emit errorOccurred("适配器未连接，无法发送数据");
        return false;
    }

    bool success = m_factory->sendData(data);
    if (success) {
        LOG_TX(QString("发送原始数据 - 长度: %1 字节").arg(data.size()));
    } else {
        LOG_ERROR("适配器发送数据失败");
        emit errorOccurred("适配器发送数据失败");
    }

    return success;
}

bool HostManager::sendProtocolFrame(uint8_t destAddr, uint8_t srcAddr, uint8_t opCode, uint16_t command, const QByteArray &payload)
{
    if (!m_factory || !m_factory->isFactoryConnected()) {
        LOG_WARNING("适配器未连接，无法发送协议帧");
        emit errorOccurred("适配器未连接，无法发送协议帧");
        return false;
    }

    QByteArray frame = ProtocolProcessor::buildFrame(destAddr, srcAddr, opCode, command, payload);
    if (frame.isEmpty()) {
        LOG_ERROR("协议帧构建失败");
        emit errorOccurred("协议帧构建失败");
        return false;
    }

    LOG_TX(QString("发送协议帧 - Dest:0x%1 Src:0x%2 Op:0x%3 Cmd:0x%4 Payload: %5")
               .arg(destAddr, 2, 16, QChar('0'))
               .arg(srcAddr, 2, 16, QChar('0'))
               .arg(opCode, 2, 16, QChar('0'))
               .arg(command, 4, 16, QChar('0'))
               .arg(payload.toHex(' ').toUpper()));

    return sendRawData(frame);
}


// ===== 工厂信号处理槽函数-接收数据 =====
// 工厂发生错误
void HostManager::onFactoryErrorOccurred(const QString &error)
{
    emit errorOccurred(error);
    LOG_WARNING(QString("Factory 错误: %1").arg(error));
}
// 工厂接收数据，路由数据
void HostManager::onFactoryDataReceived(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    // 添加数据到接收缓冲区
    m_receivedData.append(data);

    qDebug().noquote() << QString("HostManager：接收到原始数据，长度：(%1)字节，缓冲区大小：(%2)字节")
                              .arg(data.size())
                              .arg(m_receivedData.size());
    LOG_RX(QString("接收原始数据 - 长度: %1 字节, 缓冲区: %2 字节").arg(data.size()).arg(m_receivedData.size()));

    // ==== 立即提取模式：收到新数据立即尝试提取帧 ====
    bool extractedFrames = false;

    // 循环提取直到缓冲区中没有完整帧
    while (true) {
        // 使用ProtocolProcessor提取完整帧
        QList<QByteArray> frames = ProtocolProcessor::extractFramesFromBuffer(m_receivedData);

        if (frames.isEmpty()) {
            // 没有提取到完整帧，等待下一次数据接收
            break;
        }

        extractedFrames = true;

        // 处理每个提取到的完整帧
        foreach (const QByteArray& frame, frames) {
            ProtocolProcessor::ParsedFrameData parsedData;

            // 解析帧数据
            if (ProtocolProcessor::parseFrame(frame, parsedData)) {
                // 更新主机的最后通信时间
                updateHostLastSeen(parsedData.srcAddr);
                // 分发处理解析帧

                if (isMeetingCommand(parsedData.command)) {
                    emit meetingCommandResponse(parsedData.srcAddr, parsedData.command, parsedData.payload);
                    LOG_INFO(QString("会议命令路由 - 主机: %1, 命令: 0x%2")
                                 .arg(formatAddressHex(parsedData.srcAddr))
                                 .arg(parsedData.command, 4, 16, QChar('0')));
                }
                else if (isHostDetectingCommand(parsedData.command)) {
                    DeviceType::Type deviceType = DeviceType::xdc236;  // 默认会议主机
                    if (!parsedData.payload.isNull() && !parsedData.payload.isEmpty()) {
                        // 有参数时，使用第一个字节作为设备类型枚举值
                        quint8 typeByte = static_cast<quint8>(parsedData.payload[0]);
                        deviceType = DeviceType::fromByte(typeByte);    // 使用枚举结构的辅助函数

                    }  // else: 无参数时保持默认值 xdc236
                    
                    // UPSERT 模式：创建或更新主机信息
                    upsertHostInfo(parsedData.srcAddr, HostStatus::Connected, deviceType);
                    
                    // 发出信号通知探测完成
                    emit detectedHostsResponseReceived(parsedData.srcAddr);
                    emit hostInfoChanged(parsedData.srcAddr);

                    LOG_INFO(QString("收到探测响应 - 地址: %1, 类型: %2")
                                 .arg(formatAddressHex(parsedData.srcAddr), DeviceType::typeName(deviceType)));
                } else {
                    // 主机操控类型指令，正常转发到Controller
                    forwardFrameToHostController(parsedData);
                }

            } else {
                emit errorOccurred("协议帧解析失败");
                LOG_WARNING(QString("帧解析失败 - 数据: %1").arg(frame.toHex(' ').toUpper()));
            }
        }
    }

    // 如果缓冲区过大，清理无效数据（防止内存泄漏）
    if (m_receivedData.size() > 1024 * 10) { // 10KB限制
        LOG_WARNING(QString("接收缓冲区过大(%1字节)，自动清理").arg(m_receivedData.size()));
        m_receivedData.clear();
    }

    // 调试信息：显示是否成功提取帧
    if (extractedFrames) {
        LOG_INFO(QString("数据处理完成 - 缓冲区剩余: %1 字节").arg(m_receivedData.size()));
    }
}

// 转发接收到的主机操控类数据给Host Controllers
void HostManager::forwardFrameToHostController(const ProtocolProcessor::ParsedFrameData &data)
{
    // 基于目标地址路由到对应的 HostController
    uint8_t targetAddress = data.srcAddr;

    // 👇 检查主机地址存在
    HostInfo* info = getHostInfoPtr(targetAddress);
    if (!info) {
        handleUnknownAddress(targetAddress, data);
        LOG_WARNING(QString("未知主机地址，无法路由 - 地址: 0x%1").arg(targetAddress, 2, 16, QChar('0')).toUpper());
        return;
    }

    // 👇 检查主机控制器存在
    QObject* controllerObj = m_controllers.value(targetAddress, nullptr);
    if (!controllerObj) {
        handleUnknownAddress(targetAddress, data);
        LOG_WARNING(QString("主机未创建控制器，无法路由 - 地址: 0x%1").arg(targetAddress, 2, 16, QChar('0')).toUpper());
        return;
    }

    // 👇 根据设备类型，调用对应的控制器，执行接收数据函数，并发射信号
    // ==== XDC236 会议主机 ====
    if (info->deviceType == DeviceType::xdc236) {
        auto xdcCtrl = qobject_cast<XdcController*>(controllerObj);
        if (xdcCtrl) {
            xdcCtrl->handleIncomingFrame(data);
            emit hostFrameParsed(targetAddress, data);
            LOG_INFO(QString("数据路由到 XDC 主机 - 地址: %1").arg(formatAddressHex(targetAddress)));
        } else {
            LOG_WARNING(QString("控制器类型转换失败 - 地址: %1, 期望: XdcController, 实际: %2").arg(formatAddressHex(targetAddress),controllerObj->metaObject()->className()));
        }
    }

    // ==== DS240 跳频主机（预留位置）====
    else if (info->deviceType == DeviceType::DS240) {
        LOG_WARNING(QString("DS240控制器尚未实现 - 地址：%1, 设备类型：DS240 (0x02)").arg(formatAddressHex(targetAddress)));
    }

    // ==== 功放（未来扩展）====
    else if (info->deviceType == DeviceType::PowerAmplifier) {
        LOG_WARNING(QString("PowerAmplifier控制器尚未实现 - 地址：%1, 设备类型：PowerAmplifier(0x03)").arg(formatAddressHex(targetAddress)));
    }

    // ==== 无线麦克风（未来扩展）====
    else if (info->deviceType == DeviceType::WirelessMic) {
        LOG_WARNING(QString("WirelessMicr控制器尚未实现 - 地址：%1, 设备类型：WirelessMicr(0x04)").arg(formatAddressHex(targetAddress)));
    }

    // ==== 未知或其他类型（降级处理）====
    else {
        // 尝试用 XdcController 处理未知类型
        auto xdcCtrl = qobject_cast<XdcController*>(controllerObj);
        if (xdcCtrl) {
            xdcCtrl->handleIncomingFrame(data);
            emit hostFrameParsed(targetAddress, data);
            LOG_WARNING(QString("未知设备类型，使用XDC控制器 - 地址：%1, 类型：%2").arg(formatAddressHex(targetAddress), info->deviceType));
        } else {
            LOG_WARNING(QString("未知设备类型且无法转换 - 地址: 0x%1, 类型: %2").arg(formatAddressHex(targetAddress), info->deviceType));
        }
    }
}

void HostManager::handleUnknownAddress(uint8_t address, const ProtocolProcessor::ParsedFrameData &data)
{
    // 暂时不处理未知地址，仅记录日志
    LOG_WARNING(QString("收到未知地址帧，忽略处理 - 地址: %1").arg(formatAddressHex(address)));
}


// ===== 主机管理接口实现 =====
// 0. 探测主机地址 - 探测地址范围，主机是否连线(对于探测到的地址， 存储到成员变量，并且地址状态设置为Connected)
void HostManager::detectHosts(uint8_t maxAddress)
{

    // detectHosts()将向串口发送探测指令，探测指定地址的主机是否连线，并返回连线主机地址。
    // 发送协议：帧头  长度  长度反码   接收地址  发送地址  操作类型  操作命令  参数  校验和  帧尾
    // 发送指令：55  0A  F5  01  80  01  0201  83  AA；
    // 返回指令：55  0A  F5  80  01  04  0201  86  AA；
    // 循环发送给接收地址：01、02、03、04、...，maxAddress. 如果onFactoryDataReceived()接收解析到0x0201指令，即将地址
    // 存到HostInfo并设置为connected状态

    // 👇 第 1 步：检查硬件连接
    if (!m_factory || !m_factory->isFactoryConnected()) {
        LOG_WARNING("硬件未连接，无法探测主机");
        emit errorOccurred("硬件未连接");
        return;
    }


    // 👇 第 2 步：清空旧的主机信息（确保从零开始）
    removeAllHosts();

    LOG_INFO(QString("开始探测主机 - 地址范围: 0x01 - %1").arg(formatAddressHex(maxAddress)));
    // 👇 第 3 步：快速发送所有探测指令（不等响应）
    for (uint8_t address = 0x01; address <= maxAddress; address++ ) {
        if (sendProtocolFrame(address, 0x80, 0x01, 0x0201)) {
            LOG_INFO(QString("成功发送探测指令 - 地址: %1").arg(formatAddressHex(address)));
        } else {
            LOG_WARNING(QString("发送探测指令失败 - 地址: %1").arg(formatAddressHex(address)));
        }
        // 短暂的发送间隔（避免串口缓冲区溢出）
        QThread::msleep(50);
    }
    LOG_INFO(QString("已发送全部[%1]个主机探测指令，等待主机响应...").arg(maxAddress));
}

// 1.1 创建主机实例（如若地址已被探测到，为指定地址创建主机实例；HostStatus::Ready）
bool HostManager::createHost(uint8_t hostAddress)
{
    HostInfo* info = getHostInfoPtr(hostAddress);

    // 检查该地址是否连线已经被探测到
    if (!info || info->status == HostStatus::None) {
        LOG_WARNING(QString("主机未探测到或不在线，无法创建控制器 - 地址: %1").arg(formatAddressHex(hostAddress)));
        return false;
    }

    // 检查是否已经存在对应的控制器
    if (m_controllers.contains(hostAddress)) {
        LOG_WARNING(QString("主机控制器已存在 - 地址: %1").arg(formatAddressHex(hostAddress)));
        return true; // 已存在也算成功
    }

    // 👇 从 HostInfo 中读取设备类型
    DeviceTypeEnum deviceType = info->deviceType;
    
    // 👇 根据设备类型创建不同的 Controller
    QObject* controller = createControllerByType(hostAddress, deviceType);
    if (!controller) {
        LOG_ERROR(QString("创建控制器失败 - 地址: %1, 类型: %2").arg(formatAddressHex(hostAddress),DeviceType::typeName(deviceType)));
        return false;
    }

    // 👇 存储到独立容器
    m_controllers[hostAddress] = controller;

    // 更新设备类型和状态
    info->status = HostStatus::Ready;
    emit hostInfoChanged(hostAddress);
    LOG_INFO(QString("成功创建主机控制器 - 地址: %1, 类型: %2").arg(formatAddressHex(hostAddress),DeviceType::typeName(deviceType)));

    return true;
}

// 1.2 创建单主机控制器实例
QObject* HostManager::createControllerByType(uint8_t hostAddress, DeviceTypeEnum type)
{
    switch (type) {
    case DeviceType::xdc236:
        LOG_INFO(QString("创建XDC236控制器 - 地址: %1").arg(formatAddressHex(hostAddress)));
        return new XdcController(hostAddress, this);

    case DeviceType::DS240:
        LOG_INFO(QString("创建DS240控制器 - 地址: %1").arg(formatAddressHex(hostAddress)));
        return new AfhController(hostAddress, this);

    default:
        LOG_WARNING(QString("未知设备 - 地址: %1，使用默认XDC236控制器 - 地址: %2").arg(static_cast<int>(type)).arg(formatAddressHex(hostAddress)));
        return new XdcController(hostAddress, this);
    }
}

// 2. 移除主机实例（如若指定地址主机实例存在且为活动主机，则停用再删除实例；如若实例存在但非活动，删除实例;HostStatus::None)
bool HostManager::removeHost(uint8_t hostAddress)
{
    auto it = m_hostInfoMap.find(hostAddress);
    if (it == m_hostInfoMap.end() || !m_controllers.contains(hostAddress)) {
        LOG_WARNING(QString("主机不存在，不能移除 - 地址: %1").arg(formatAddressHex(hostAddress)));
        return false;
    }

    // 如果正在删除的是活动主机，先取消活动状态
    if (m_selectedHostAddress  == hostAddress) {
        uint8_t oldAddress = m_selectedHostAddress ;
        m_selectedHostAddress  = 0;
        updateSelectedHostMap();
        emit selectedHostChanged(oldAddress, 0);

        LOG_INFO(QString("移除当前选中主机 - 地址: %1").arg(formatAddressHex(hostAddress)));
    }

    // 清除控制器（智能指针会自动释放）
    // 👇 移除并销毁控制器
    QObject* controller = m_controllers.take(hostAddress);
    if (controller) {
        controller->deleteLater();  // 安全删除
    }
    it->status = HostStatus::Connected;  // 降级为Connected状态, 保留地址

    emit hostInfoChanged(hostAddress);

    LOG_INFO(QString("成功移除主机控制器 - 地址: %1").arg(formatAddressHex(hostAddress)));
    return true;
}

// 3. 移除所有主机实例（停止活动主机并释放所有控制器实例）
void HostManager::removeAllHosts()
{
    LOG_INFO("开始移除所有主机控制器...");

    // 👇 第 1 步：复制所有需要移除的控制器地址列表
    QList<uint8_t> addressesToRemove;
    for (auto it = m_controllers.constBegin(), end = m_controllers.constEnd(); it != end; ++it) {
        addressesToRemove.append(it.key());
    }

    if (addressesToRemove.isEmpty()) {
        LOG_INFO("没有需要移除的主机控制器");
        return;
    }
    LOG_INFO(QString("准备移除[%1个]主机控制器").arg(addressesToRemove.size()));

    // 👇 第 2 步：直接通过重置变量取消选中主机实例
    if (m_selectedHostAddress != 0) {
        uint8_t oldAddr = m_selectedHostAddress;
        m_selectedHostAddress = 0;
        updateSelectedHostMap();
        emit selectedHostChanged(oldAddr, 0);
    }

    // 👇 第 3 步：逐个移除所有控制器（复用 removeHost 逻辑）
    int removedCount = 0;
    for (uint8_t address : std::as_const(addressesToRemove)) {
        if (removeHost(address)) {
            removedCount++;
        }
    }
    LOG_INFO(QString("移除完成，共删除[%1个]控制器实例").arg(removedCount));
}

// 4. 选择主机（如若地址主机已经实例化，选择该实例；如果没有实例化则创建实例，再选择）
bool HostManager::selectHost(uint8_t hostAddress) {
    // [修复] 检查主机是否存在（是否被探测到）
    if (!getHostInfoPtr(hostAddress)) {
        LOG_WARNING(QString("选择失败: 主机 %1 不存在").arg(formatAddressHex(hostAddress)));
        return false;
    }

    // [逻辑调整] 如果还没创建实例，是否自动创建？
    // 按照你的流程：应该由右键 createHost 触发。
    // 但如果用户直接左键点了，为了体验好，这里还是建议自动创建，或者至少返回 true 并记录日志
    if (!m_controllers.contains(hostAddress)) {
        LOG_INFO(QString("左键选中未就绪主机，自动执行 createHost - 地址: %1").arg(formatAddressHex(hostAddress)));
        if (!createHost(hostAddress)) return false;
    }

    uint8_t oldAddr = m_selectedHostAddress;
    if (oldAddr == hostAddress) return true; 

    m_selectedHostAddress = hostAddress;
    updateSelectedHostMap();
    emit selectedHostChanged(oldAddr, hostAddress);

    LOG_INFO(QString("已选中主机: %1").arg(formatAddressHex(hostAddress)));
    return true;
}

// 5. 多主机管理（主机集合信息，返回集合信息，如若不存在返回 0x00）
// 5.1 获取所有连接的Host Address
QVariantList HostManager::getConnectedHostList() const
{
    QVariantList list;

    for (auto it = m_hostInfoMap.constBegin(); it != m_hostInfoMap.constEnd(); ++it) {
        // 只返回状态不是None的主机
        if (it->status != HostStatus::None) {
            // ✅ 返回整数地址，而不是完整 map
            list.append(static_cast<int>(it.key()));
            // qDebug().noquote() << "  添加地址:" << QString::number(it.key(), 16).toUpper();
        }
    }

    // qDebug().noquote() << "返回地址列表:" << list;
    return list;
}

// 5.2 获取已就绪的主机列表
QVariantList HostManager::getReadyHostList() const {
    QVariantList list;
    for (const auto& info : m_hostInfoMap) {
        if (info.status == HostStatus::Ready) {
            list.append(info.toVariantMap());
        }
    }
    return list;
}

// 5.3 通过地址获取指定的HostInfo数据结构
QVariantMap HostManager::getAllHostMap(uint8_t hostAddress) const
{
    const HostInfo* info = getHostInfoPtr(hostAddress);
    if (info) {
        return info->toVariantMap();  // 返回完整的主机信息
    }

    // 主机不存在时，创建默认的 HostInfo 对象
    HostInfo defaultInfo;
    defaultInfo.address = hostAddress;
    defaultInfo.aliasName = QString("Host-%1").arg(hostAddress);
    // status 默认为 HostStatus::None

    return defaultInfo.toVariantMap();
}

// 6. 单主机管理（通过地址或者特定变量管理单主机）
// 6.1.1 获取 HostInfo指针
HostInfo* HostManager::getHostInfoPtr(uint8_t address)
{
    auto it = m_hostInfoMap.find(address);
    if (it != m_hostInfoMap.end()) {
        return &(it.value());
    }
    return nullptr;
}

// 6.1.2 只读版本（const版本）
const HostInfo* HostManager::getHostInfoPtr(uint8_t address) const
{
    auto it = m_hostInfoMap.find(address);
    if (it != m_hostInfoMap.end()) {
        return &(it.value());
    }
    return nullptr;
}

// 6.2.1 更新单主机信息（UPSERT 模式：已存在则更新，不存在则创建）
void HostManager::upsertHostInfo(uint8_t address, HostStatus status, DeviceTypeEnum deviceType)
{
    // 查找是否已存在该地址的主机信息
    auto it = m_hostInfoMap.find(address);

    if (it != m_hostInfoMap.end()) {
        // 情况 1：主机信息已存在，直接更新状态
        HostStatus oldStatus = it->status;
        it->status = status;
        it->lastSeen = QDateTime::currentDateTime();

        if (deviceType != DeviceType::Unknown) {
            it->deviceType = deviceType;
        }

        LOG_INFO(QString("更新主机信息 - 地址: %1, 旧状态: %2, 新状态: %3, 类型: %4")
                    .arg(formatAddressHex(address),
                        HostStatusDefs::toString(static_cast<int>(oldStatus)),
                        HostStatusDefs::toString(static_cast<int>(status)),
                        DeviceType::typeName(deviceType)));

        // 状态变化时发出信号
        if (oldStatus != status) {
            emit hostInfoChanged(address);
        }
    }
    else {
        // 情况 2：主机信息不存在，创建新记录
        HostInfo newInfo;
        newInfo.address = address;
        newInfo.status = status;
        newInfo.deviceType = deviceType;
        newInfo.lastSeen = QDateTime::currentDateTime();

        // 设置默认值
        newInfo.aliasName = QString("Host-%1").arg(address, 2, 16, QChar('0')).toUpper();

        m_hostInfoMap[address] = newInfo;

        emit hostInfoChanged(address);

        LOG_INFO(QString("创建主机 - 地址: %1, 状态: %2, 类型: %3")
                    .arg(formatAddressHex(address),
                        HostStatusDefs::toString(static_cast<int>(status)),
                        DeviceType::typeName(deviceType)));

    }
}

// 6.2.2 更新单主机最后通信时间
void HostManager::updateHostLastSeen(uint8_t address)
{
    auto it = m_hostInfoMap.find(address);
    if (it != m_hostInfoMap.end()) {
        it->lastSeen = QDateTime::currentDateTime();
        // 可以选择是否发出信号，因为只是更新时间，状态未变
        // emit hostInfoChanged(address);  // 如果需要实时更新时间可以取消注释
    }
}

// 6.2.3 更新选中主机映射表 (供 QML 绑定)
void HostManager::updateSelectedHostMap() {
    if (!m_selectedHostMap) return;

    if (m_selectedHostAddress != 0) {
        const HostInfo* info = getHostInfoPtr(m_selectedHostAddress);
        if (info) {
            // [情况 A] 有真实数据：全量更新所有字段
            m_selectedHostMap->insert("address", info->addressHex());
            m_selectedHostMap->insert("aliasName", info->aliasName);
            m_selectedHostMap->insert("serialNumber", info->serialNumber);
            m_selectedHostMap->insert("deviceTypeName", info->deviceTypeName());
            m_selectedHostMap->insert("status", static_cast<int>(info->status));
            m_selectedHostMap->insert("statusText", info->statusText());
            m_selectedHostMap->insert("isConnected", info->isConnected());
            m_selectedHostMap->insert("isReady", info->isReady());
            m_selectedHostMap->insert("isActive", true); // 标记为“已选中”
            m_selectedHostMap->insert("lastSeenTime", info->lastSeenTime());
            return;
        }
    }

    // [情况 B] 无选中主机：提供一套完整的“模拟/默认”数据
    // 注意：这里的 Key 必须和上面完全一致，确保 QML 绑定的属性永远存在
    m_selectedHostMap->insert("address", "--");
    m_selectedHostMap->insert("aliasName", tr("未选择主机"));
    m_selectedHostMap->insert("serialNumber", "");
    m_selectedHostMap->insert("deviceTypeName", "---");
    m_selectedHostMap->insert("status", 0); // 对应 HostStatus::None
    m_selectedHostMap->insert("statusText", tr("等待选择"));
    m_selectedHostMap->insert("isConnected", false);
    m_selectedHostMap->insert("isReady", false);
    m_selectedHostMap->insert("isActive", false); // 关键：UI 据此禁用按钮,标记为“已选中”
    m_selectedHostMap->insert("lastSeenTime", "");
}

// 6.3 移除单主机控制器实例（不删除主机信息）
void HostManager::removeController(uint8_t address)
{
    if (m_controllers.contains(address)) {
        QObject* controller = m_controllers.take(address);
        if (controller) {
            controller->deleteLater();
            LOG_INFO(QString("移除主机控制器 - 地址: %1").arg(formatAddressHex(address)));
        }
    }
}

// 6.4 获取单主机控制器实例
QObject* HostManager::getController(uint8_t address) const
{
    return m_controllers.value(address, nullptr);
}

// =============== 其它模块特别需求 ===============
// 获取可用主机列表（供会议模块使用）
QVariantList HostManager::getAvailableHostsForMeeting() const {
    // 目前逻辑等同于 Ready 列表
    return getReadyHostList();
}

/* 关键流程说明：
1. detectHosts() 调用 sendProtocolFrame(address, 0x80, 0x01, 0x0201) 发送探测指令
2. 目标主机响应，ConnectionFactory 收到数据
3. onFactoryDataReceived() 被触发，解析到命令码 0x0201
4. m_detectHostResponse[address] = true 被设置
5. waitForDetectHostResponse() 的轮询循环检测到 m_detectHostResponse[address] 为 true
6. waitForDetectHostResponse() 返回 true
7. detectHosts() 中 if (waitForDetectHostResponse(...)) 条件成立
8. detectedHostAddress.append(address) 执行，地址被保存. 这是一个间接的过程：waitForDetectHostResponse() 负责检测状态并返回结果，detectHosts() 根据返回结果决定是否保存地址。
*/

