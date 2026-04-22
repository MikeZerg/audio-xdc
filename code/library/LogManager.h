// core/Library/LogManager.h

#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QStringList>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

// 日志管理器：负责应用程序的全局日志记录、文件存储及 UI 展示
class LogManager : public QObject {
    Q_OBJECT
    // 注册属性，方便 QML 直接绑定日志列表
    Q_PROPERTY(QStringList logs READ logs NOTIFY logsChanged)
    // 注册过滤关键词列表
    Q_PROPERTY(QStringList filterKeywords READ filterKeywords WRITE setFilterKeywords NOTIFY filterKeywordsChanged)


public:
    // 日志内容级别枚举
    enum LogLevel {
        INFO = 0,     // 普通信息：状态变更、业务逻辑节点
        WARNING = 1,  // 警告信息：非致命错误、参数修正
        ERROR = 2,    // 错误信息：连接失败、解析异常
        DATA_RX = 3,  // 接收数据：原始 Hex 数据流（调试用）
        DATA_TX = 4   // 发送数据：原始 Hex 数据流（调试用）
    };
    Q_ENUM(LogLevel)

    // 日志记录模式（过滤器）枚举
    enum LogMode {
        NormalMode = 0, // 日常模式：仅记录 INFO, WARNING, ERROR，屏蔽高频数据流
        DebugMode = 1   // 调试模式：记录所有级别，包括完整的收发数据
    };
    Q_ENUM(LogMode)

    // 获取单例实例
    static LogManager* instance();

    // QML 可调用的接口：切换日志模式
    Q_INVOKABLE void setLogMode(LogMode mode);
    // QML 可调用的接口：获取当前日志模式
    Q_INVOKABLE LogMode currentLogMode() const;
    // 设置过滤关键词
    Q_INVOKABLE void setFilterKeywords(const QStringList& keywords);
    // 获取过滤关键词
    QStringList filterKeywords() const;

    // 基础日志记录接口
    Q_INVOKABLE void addLog(const QString& message, LogLevel level = INFO);
    // 清空内存中的日志列表（UI 显示用）
    Q_INVOKABLE void clearLogs();

    // 内部使用：带上下文（文件名、行号、函数名）的日志记录
    void addLogWithContext(const QString& message, LogLevel level, const char* file, int line, const char* function);

    // 获取当前内存中的日志列表
    QStringList logs() const;

signals:
    // 当日志列表更新时发出信号，驱动 QML UI 刷新
    void logsChanged();
    // 当过滤关键词改变时发出信号
    void filterKeywordsChanged();

private:
    explicit LogManager(QObject* parent = nullptr);
    // 将日志行追加写入到 logs.txt 文件
    void writeToFile(const QString& logLine);
    // 核心过滤逻辑：根据当前模式判断是否应该记录该级别的日志
    bool shouldLog(LogLevel level) const;
    // 检查日志内容是否匹配过滤关键词
    bool matchesFilter(const QString& message) const;

    LogMode m_mode = DebugMode; // 当前日志模式，默认开启调试以便开发追踪
    QStringList m_logs;         // 内存中的日志列表，仅存储非 DATA 级别日志以保护 UI 性能
    QStringList m_filterKeywords; // 过滤关键词列表

};

// 定义便捷宏，自动捕获调用位置并转发到 LogManager
#define LOG_INFO(msg) LogManager::instance()->addLogWithContext(msg, LogManager::INFO, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg) LogManager::instance()->addLogWithContext(msg, LogManager::WARNING, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) LogManager::instance()->addLogWithContext(msg, LogManager::ERROR, __FILE__, __LINE__, __FUNCTION__)
#define LOG_RX(msg) LogManager::instance()->addLogWithContext(msg, LogManager::DATA_RX, __FILE__, __LINE__, __FUNCTION__)
#define LOG_TX(msg) LogManager::instance()->addLogWithContext(msg, LogManager::DATA_TX, __FILE__, __LINE__, __FUNCTION__)

#endif // LOGMANAGER_H