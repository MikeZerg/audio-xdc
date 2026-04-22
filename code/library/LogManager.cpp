// LogManager.cpp
#include "LogManager.h"

// 返回单例实例（线程安全的静态局部变量）
LogManager* LogManager::instance() {
    static LogManager instance;
    return &instance;
}

// 构造函数：初始化默认模式为 DebugMode，便于开发期排查问题
LogManager::LogManager(QObject* parent) : QObject(parent), m_mode(DebugMode) {
    addLog("日志系统初始化完成，当前处于调试模式 (DebugMode)", INFO);
}

// 切换日志模式
void LogManager::setLogMode(LogMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        // 记录模式切换事件，这条日志在任何模式下都会显示
        addLog(mode == NormalMode ? "已切换到日常模式 (屏蔽收发数据)" : "已切换到调试模式 (记录收发数据)", INFO);
    }
}

// 获取当前日志模式
LogManager::LogMode LogManager::currentLogMode() const {
    return m_mode;
}

// 设置记录过滤关键词
void LogManager::setFilterKeywords(const QStringList& keywords) {
    if (m_filterKeywords != keywords) {
        m_filterKeywords = keywords;
        emit filterKeywordsChanged();
        // 关键词改变时，触发一次 logsChanged 让 UI 重新评估显示
        emit logsChanged(); 
    }
}

// 获取记录过滤关键词
QStringList LogManager::filterKeywords() const {
    return m_filterKeywords;
}

// 检查日志内容是否匹配过滤关键词
bool LogManager::matchesFilter(const QString& message) const {
    if (m_filterKeywords.isEmpty()) return true; // 无关键词则全部显示
    
    for (const QString& keyword : m_filterKeywords) {
        if (message.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}


// 日志模式过滤逻辑：决定某条日志是否应该被处理
bool LogManager::shouldLog(LogLevel level) const {
    if (m_mode == NormalMode) {
        // 日常模式：只允许记录低于 DATA_RX 级别的日志（即 INFO, WARNING, ERROR）
        return level < DATA_RX;
    }
    // 调试模式：放行所有级别的日志
    return true;
}

// 基础日志记录实现
void LogManager::addLog(const QString& message, LogLevel level) {
    if (!shouldLog(level)) return; // 如果不符合当前模式的过滤要求，直接丢弃

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logLevelStr;
    
    // 根据级别生成简短的标签
    switch (level) {
        case INFO: logLevelStr = "INFO"; break;
        case WARNING: logLevelStr = "WARN"; break;
        case ERROR: logLevelStr = "ERR"; break;
        case DATA_RX: logLevelStr = "RX"; break;
        case DATA_TX: logLevelStr = "TX"; break;
        default: logLevelStr = "UNK"; break;
    }
    
    QString logLine = QString("[%1] [%2] %3").arg(timestamp, logLevelStr, message);

    // 性能优化：DATA 级别的日志只写文件，不放入 m_logs 列表，防止 QML UI 因高频刷新而卡顿
    if (level < DATA_RX) {
        m_logs.append(logLine);
        emit logsChanged();
    }

    writeToFile(logLine);
}

// 获取内存日志列表
QStringList LogManager::logs() const {
    return m_logs;
}

// 清空内存日志列表
void LogManager::clearLogs()
{
    m_logs.clear();
    emit logsChanged();
}

// 将日志行追加写入磁盘文件
void LogManager::writeToFile(const QString& logLine) {
    QFile file("logs.txt");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logLine << "\n";
        file.close();
    }
}

// 带上下文的日志记录实现
void LogManager::addLogWithContext(const QString& message, LogLevel level, const char* file, int line, const char* function) {
    if (!shouldLog(level)) return; // 同样先进行过滤检查

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logLevelStr;
    switch (level) {
        case INFO: logLevelStr = "INFO"; break;
        case WARNING: logLevelStr = "WARN"; break;
        case ERROR: logLevelStr = "ERR"; break;
        case DATA_RX: logLevelStr = "RX"; break;
        case DATA_TX: logLevelStr = "TX"; break;
        default: logLevelStr = "UNK"; break;
    }
    
    // 提取纯文件名，兼容 Windows (\) 和 Linux (/) 路径分隔符
    QString fileName = QString(file).section('/', -1).section('\\', -1);
    
    // 格式化输出：[时间] [级别] [文件名:行号 函数名] 消息内容
    QString logLine = QString("[%1] [%2] [%3:%4 %5] %6")
                          .arg(timestamp, logLevelStr, fileName, QString::number(line), function, message);

    // 同样，DATA 级别不更新 UI 列表，仅存入文件
    if (level < DATA_RX) {
        m_logs.append(logLine);
        emit logsChanged();
    }

    writeToFile(logLine);
}
