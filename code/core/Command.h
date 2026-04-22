#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <cstdint>

#include "ProtocolProcessor.h"

// 命令基类
class HostController;

class Command : public QObject
{
    Q_OBJECT

public:
    // 命令状态枚举
    enum CommandStatus {
        Created = 0,    // 已创建
        Sent = 1,       // 已发送
        Waiting = 2,    // 等待响应
        Completed = 3,  // 执行完成
        Failed = 4,     // 执行失败
        TimedOut = 5    // 超时
    };
    Q_ENUM(CommandStatus)

    explicit Command(uint16_t commandCode, HostController *controller = nullptr, QObject *parent = nullptr);
    virtual ~Command();
    
    // 设置主机控制器
    void setController(HostController *controller);
    
    // 获取主机控制器
    HostController* getController() const;

    // 获取命令码
    uint16_t getCommandCode() const;
    
    // 获取命令状态
    CommandStatus getStatus() const;
    
    // 设置命令状态
    void setStatus(CommandStatus newStatus);
    
    // 获取发送时间
    QDateTime getSendTime() const;
    
    // 设置发送时间
    void setSendTime(const QDateTime &time);
    
    // 获取请求帧数据
    ProtocolProcessor::ParsedFrameData getRequestFrame() const;
    
    // 设置请求帧数据
    void setRequestFrame(const ProtocolProcessor::ParsedFrameData &frame);
    
    // 执行命令 - 发送请求（由子类实现）
    virtual bool execute(const ProtocolProcessor::ParsedFrameData &frame) = 0;
    
    // 处理响应 - 接收响应（由子类实现）
    virtual bool handleResponse(const ProtocolProcessor::ParsedFrameData &frame) = 0;
    
    // 发送命令帧（供子类调用）
    bool sendCommandFrame(const ProtocolProcessor::ParsedFrameData &frame);
    
    // 检查响应是否匹配请求（默认实现）
    virtual bool isResponseMatching(const ProtocolProcessor::ParsedFrameData &responseFrame) const;
    
    // 超时处理（默认实现）
    virtual void handleTimeout();
    
    // 获取超时时间（毫秒，默认3秒）
    virtual int getTimeoutInterval() const;
    
signals:
    // 命令执行完成信号
    void commandCompleted(Command *command);
    
    // 命令失败信号
    void commandFailed(Command *command, const QString &errorMessage);
    
    // 命令超时信号
    void commandTimedOut(Command *command);
    
    // 请求数据准备好信号
    void requestReady(const ProtocolProcessor::ParsedFrameData &requestFrame);
    
protected:
    uint16_t m_commandCode;                // 命令码
    CommandStatus m_status;                // 命令状态
    QDateTime m_sendTime;                  // 发送时间
    ProtocolProcessor::ParsedFrameData m_requestFrame; // 请求帧数据
    HostController* m_controller;          // 主机控制器指针
};

#endif // COMMAND_H
