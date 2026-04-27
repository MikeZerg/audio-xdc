// // qml/CustomWidget/CustomTreeMenuData.qml
import QtQuick 2.15

QtObject {
    // 功能按钮数据
    property var functionButtons: [
        {id: "F1", name: "欢迎主页", icon:"qrc:/image/app-home.svg", hoverIcon:"qrc:/image/app-homehover.svg"},
        {id: "F2", name: "主机管理", icon:"qrc:/image/app-host.svg", hoverIcon:"qrc:/image/app-hosthover.svg"},
        {id: "F3", name: "会议与活动", icon:"qrc:/image/app-meeting.svg", hoverIcon:"qrc:/image/app-meetinghover.svg"},
        {id: "F4", name: "日志与调试", icon:"qrc:/image/app-unit.svg", hoverIcon:"qrc:/image/app-unithover.svg"},
        {id: "F5", name: "关于", icon:"qrc:/image/app-about.svg", hoverIcon:"qrc:/image/app-abouthover.svg"}
    ]

    // 树状菜单数据（按功能分组，只能设置两层）
    property var menuItems: {
        "F1": [
            {id: "F1-10", name: "欢迎主页", level: 1, page:"qml/startup_product.qml"}
        ],
        "F2": [
            {id: "F2-10", name: "连接设置", level: 1, page:"qml/host_connection.qml"},
            {id: "F2-20", name: "主机管理", level: 1, page:"qml/host_detecting.qml"},
            {id: "F2-30", name: "融合主机 - 设置&管理", level: 1, page:"qml/host_settings_xdc.qml"},
            {id: "F2-31", name: "无线信号测试", level: 2, parentId: "F2-30", page:"qml/host_rssi_xdc.qml"},
            {id: "F2-32", name: "融合主机单元", level: 2, parentId: "F2-30", page:"qml/host_unit_xdc.qml"},
            {id: "F2-40", name: "跳频主机 - 设置&管理", level: 1, page:"qml/host_settings_ds.qml"},
            {id: "F2-41", name: "跳频信号监测", level: 2, parentId: "F2-40", page:"qml/host_rssi_ds.qml"},
            {id: "F2-50", name: "单元管理", level: 1, page:""},
            {id: "F2-51", name: "单元状态", level: 2, parentId: "F2-50", page:""},
            {id: "F2-52", name: "工作单元监测", level: 2, parentId: "F2-50", page:""}
        ],
        "F3": [
            {id: "F3-10", name: "日程安排", level: 1, page:"qml/meeting_calendar.qml"},
            {id: "F3-11", name: "会议与活动", level: 2, parentId: "F3-10", page:"qml/meeting_console.qml"},
            {id: "F3-12", name: "实时会议", level: 2, parentId: "F3-10", page:"qml/meeting_active.qml"},
            {id: "F3-13", name: "投票记录", level: 2, parentId: "F3-10", page:"qml/meeting_voting.qml"},
            {id: "F3-14", name: "签到记录", level: 2, parentId: "F3-10", page:"qml/meeting_checkin.qml"},
            {id: "F3-15", name: "投票记录", level: 2, parentId: "F3-10", page:"qml/meeting_voting.qml"}

        ],
        "F4": [
            {id: "F4-10", name: "日志与调试", level: 1, page:"qml/app_logs.qml"},
            {id: "F4-11", name: "系统设置", level: 2, page:"qml/app_settings.qml"}
        ],

        "F5": [
            {id: "F5-10", name: "帮助", level: 1, page:"qml/app_help.qml"}
        ]
    }
}
