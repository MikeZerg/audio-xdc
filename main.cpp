#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtCharts/QtCharts>

#include "code/hardware/ConnectionAdapter.h"
#include "code/hardware/ConnectionFactory.h"
#include "code/core/HostManager.h"
#include "code/core/MeetingManager.h"
#include "code/library/LogManager.h"

int main(int argc, char *argv[])
{
    // 设置环境变量，允许 QML 中的 XMLHttpRequest 读取本地文件
    qputenv("QML_XHR_ALLOW_FILE_READ", "1");

    QGuiApplication app(argc, argv);

    // 设置应用程序图标
    QIcon icon(":/image/app-icon.svg");
    app.setWindowIcon(icon);

    // 设置非原生样式以支持自定义控件属性
    QQuickStyle::setStyle("Fusion");

    // ======== 注册硬件层组件 ========
    // 注册枚举类型以便在 QML 中使用
    qmlRegisterType<ConnectionFactory>("Audio.Hardware", 1, 0, "ConnectionFactory");
    qmlRegisterUncreatableMetaObject(ConnectionAdapter::staticMetaObject,"Audio.Hardware", 1, 0, "ConnectionType",
                                     "ConnectionType 是连接类型枚举值，不能创建实例");

    // ======== 注册日志管理器 ========
    // 将 LogLevel 枚举注册到 QML（必须在设置上下文属性之前）
    qmlRegisterUncreatableType<LogManager>("Audio.Log", 1, 0, "LogLevel", "LogLevel 是枚举类型，不能创建实例");
    // 将 LogManager 注册为 QML 单例模块，可在任意 QML 文件中使用
    qmlRegisterSingletonInstance("Audio.Log", 1, 0, "LogManager", LogManager::instance());


    QQmlApplicationEngine engine;

    // 0. 将 LogManager 作为上下文属性注入 QML 引擎
    engine.rootContext()->setContextProperty("logManager", LogManager::instance());

    // 1. 创建硬件工厂实例（单例，全局管理所有主机状态）
    ConnectionFactory *factory = new ConnectionFactory(&app);
    engine.rootContext()->setContextProperty("connectionFactory", factory);

    // 2. 创建主机管理器实例（单例），直接传入ConnectionFactory实例
    HostManager *hostManager = new HostManager(factory, &app);
    engine.rootContext()->setContextProperty("hostManager", hostManager);

    // 3. 创建会议管理器实例并建立关联
    MeetingManager *meetingManager = new MeetingManager(&app);
    meetingManager->setHostManager(hostManager); // 关键步骤：注入 HostManager
    engine.rootContext()->setContextProperty("meetingManager", meetingManager);


    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("audioxdc", "Main");

    return app.exec();
}
