cmake_minimum_required(VERSION 3.16)

project(audio-xdc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    Qml
    Xml
    SerialPort
    Charts
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appaudio-xdc
    main.cpp
)

qt_add_qml_module(appaudio-xdc
    URI audioxdc
    VERSION 1.0
    QML_FILES
        Main.qml
        Theme.qml

    RESOURCES
        qmldir      # 样式文件Colors.qml单例生效的关键
        Image.qrc
        data/meeting_data.json

    QML_FILES
        # 自定义控件
        qml/CustomWidget/CustomTreeMenu.qml
        qml/CustomWidget/CustomTreeMenuData.qml
        qml/CustomWidget/CustomIcon.qml
        qml/CustomWidget/CustomCombox.qml
        qml/CustomWidget/CustomTextField.qml
        qml/CustomWidget/CustomSpin.qml
        qml/CustomWidget/CustomButton.qml
        qml/CustomWidget/CustomRectangle.qml
        qml/CustomWidget/CustomDatetimePicker.qml
        qml/CustomWidget/CustomRadioButton.qml
        QML_FILES qml/CustomWidget/CustomCheckBox.qml
        qml/CustomWidget/CustomMultipleCheckBox.qml
        qml/CustomWidget/CustomSlider.qml

        # 启动界面
        qml/startup_product.qml

        # 主机管理界面
        qml/host_connection.qml
        qml/host_detection.qml # 临时，可以删除
        qml/host_detecting.qml

        # XDC主机界面
        qml/host_info_xdc.qml
        qml/host_settings_xdc.qml
        qml/host_rssi_xdc.qml
        # XDC 单元界面
        qml/host_unit_xdc.qml
        qml/app_logs_test.qml
        # 自定义主机组件（集成设备模块）
        qml/CustomWidget/CustomHostUnit.qml
        qml/CustomWidget/CustomHostCard.qml     # 临时，可以删除
        # 自定义单元组件（集成设备模块）
        qml/CustomWidget/CustomUnitCard.qml
        qml/CustomWidget/CustomUnitDetailPanel.qml

        # DS主机界面
        qml/host_info_ds.qml
        qml/host_settings_ds.qml
        qml/host_rssi_ds.qml
        # 自定义主机组件（集成设备模块）
        qml/CustomWidget/CustomAFHUnit.qml

        #XDC236会议管理界面
        qml/meeting_management.qml
        qml/meeting_console.qml
        qml/CustomWidget/PanelCreateMeeting.qml
        qml/CustomWidget/PanelCreateCheckin.qml
        qml/CustomWidget/PanelCreateVoting.qml
        qml/meeting_active.qml
        qml/meeting_checkin.qml
        qml/meeting_voting.qml


        # QML日志
        qml/app_logs.qml
        qml/app_settings.qml
        qml/TestCharts.qml

    SOURCES
        # 会议管理器
        code/core/MeetingManager.h
        code/core/MeetingManager.cpp

        # AFH跳频主机控制器
        code/core/afhController.h
        code/core/afhController.cpp

        # XDC236主机控制器
        code/core/xdcController.h
        code/core/xdcController.cpp
        # XDC236单元数据模型
        code/core/UnitModel.h
        code/core/UnitModel.cpp

        # 通信协议
        code/core/ProtocolProcessor.cpp
        code/core/ProtocolProcessor.h

        # 公用主机集管理器
        code/core/HostController.cpp
        code/core/HostController.h
        code/core/HostManager.cpp
        code/core/HostManager.h

        # 连接工厂，提供公用硬件接口方式
        code/hardware/ConnectionFactory.cpp
        code/hardware/ConnectionFactory.h
        code/hardware/ConnectionAdapter.cpp
        code/hardware/ConnectionAdapter.h

        # 底层硬件接口
        code/hardware/SerialPortAdapter.cpp
        code/hardware/SerialPortAdapter.h
        code/hardware/TCPSocketAdapter.cpp
        code/hardware/TCPSocketAdapter.h

        # 日志工具
        code/library/LogManager.cpp
        code/library/LogManager.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appaudio-xdc PROPERTIES
#   MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appaudio-xdc
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appaudio-xdc
    PRIVATE
    # Qt核心模块
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2

    # 图表模块
    Qt6::Charts

    # 数据处理模块
    Qt6::Xml
    Qt6::SerialPort
)

include(GNUInstallDirs)
install(TARGETS appaudio-xdc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
