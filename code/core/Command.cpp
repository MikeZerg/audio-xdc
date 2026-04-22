#include "Command.h"
#include "HostController.h"
#include <QDebug>

Command::Command(uint16_t commandCode, HostController *controller, QObject *parent)
    : QObject(parent)
    , m_commandCode(commandCode)
    , m_status(Created)
    , m_controller(controller)
{
    qDebug() << "Command created for code:" << QString::number(m_commandCode, 16);
}

Command::~Command()
{
    qDebug() << "Command destroyed for code:" << QString::number(m_commandCode, 16);
}

uint16_t Command::getCommandCode() const
{
    return m_commandCode;
}

Command::CommandStatus Command::getStatus() const
{
    return m_status;
}

void Command::setStatus(CommandStatus newStatus)
{
    if (m_status != newStatus) {
        m_status = newStatus;
        
        switch (m_status) {
        case Completed:
            emit commandCompleted(this);
            break;
        case Failed:
            emit commandFailed(this, "Command failed");
            break;
        case TimedOut:
            emit commandTimedOut(this);
            break;
        default:
            break;
        }
    }
}

QDateTime Command::getSendTime() const
{
    return m_sendTime;
}

void Command::setSendTime(const QDateTime &time)
{
    m_sendTime = time;
}

ProtocolProcessor::ParsedFrameData Command::getRequestFrame() const
{
    return m_requestFrame;
}

void Command::setRequestFrame(const ProtocolProcessor::ParsedFrameData &frame)
{
    m_requestFrame = frame;
}

// 设置主机控制器
void Command::setController(HostController *controller)
{
    m_controller = controller;
}

// 获取主机控制器
HostController* Command::getController() const
{
    return m_controller;
}

// 发送命令帧
bool Command::sendCommandFrame(const ProtocolProcessor::ParsedFrameData &frame)
{
    if (!m_controller) {
        qWarning() << "No controller set for command" << QString::number(m_commandCode, 16);
        return false;
    }
    
    // 构建帧数据
    QByteArray frameData = ProtocolProcessor::buildFrame(
        frame.destAddr,
        frame.srcAddr,
        frame.opCode,
        frame.command,
        frame.payload
    );
    
    // 发送命令
    return m_controller->sendCommand(frameData);
}

bool Command::isResponseMatching(const ProtocolProcessor::ParsedFrameData &responseFrame) const
{
    // 默认实现：通过命令码匹配
    return (responseFrame.command == m_commandCode);
}

void Command::handleTimeout()
{
    qWarning() << "Command timed out:" << QString::number(m_commandCode, 16);
    setStatus(TimedOut);
}

int Command::getTimeoutInterval() const
{
    return 3000; // 默认3秒超时
}
