import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: hostUnitRoot
    width: 210
    height: 130

    // ========== 只接收地址作为参数 ==========
    required property int hostAddress

    // 模型单元被选中
    property bool isSelected: false // 选中状态属性
    signal selected(int address)    // 通知被选中的地址信号

    // ========== 通过地址查询完整信息 ==========
    property var hostInfo: hostManager.getAllHostMap(hostAddress)

    // ========== 直接从 hostInfo 获取所有属性 ==========
    property string addressStr: hostInfo ? (hostInfo.addressHex || "0x00") : "0x00"
    property string hostName: hostInfo ? hostInfo.deviceTypeName : DeviceType.typeName(DeviceType.Type.xdc236)
    property int deviceTypeValue: hostInfo ? (hostInfo.deviceType || 0) : 0
    property string aliasName: hostInfo ? (hostInfo.aliasName || "") : ""
    property string serialNumber: hostInfo ? (hostInfo.serialNumber || "----") : "----"
    property int statusValue: hostInfo ? (hostInfo.status || 0) : 0
    property string statusText: hostInfo ? (hostInfo.statusText || "未知") : "未知"
    property bool isConnected: hostInfo ? (hostInfo.isConnected || false) : false
    property bool isReady: hostInfo ? (hostInfo.isReady || false) : false
    property bool isActive: hostInfo ? (hostInfo.isActive || false) : false
    property string lastSeenTime: hostInfo ? (hostInfo.lastSeen || "--:--:--") : "--:--:--"
    property var customData: hostInfo ? (hostInfo.customData || {}) : ({})

    // 状态颜色
    property color statusColor: {
        switch(statusValue) {
            case 0: return "#B3B3B3";  // None - 灰色
            case 1: return "#87CEFA";  // Connected - 暗蓝色 (较低亮度, 较低饱和度)
            case 2: return "#4169E1";  // Ready - 中蓝色 (中等亮度, 中等饱和度)
            case 3: return "#2121FF";  // Active - 亮蓝色 (高亮度, 高饱和度)
            case 4: return "#ED5736";  // Busy - 红色
            case 5: return "#9D2933";  // Error - 深红
            default: return "#B3B3B3";
        }
    }

    // ========== 自定义右键菜单 ==========
    Menu {
        id: contextMenu
        x: parent.width / 2
        y: parent.height / 2

        implicitWidth: 160

        // 背景
        background: Rectangle {
            color: Theme.mPanel
            border.color: Theme.mBorderLine
            border.width: 1
            radius: 2
        }

        // 完全自定义菜单项
        contentItem: Column {
            spacing: 2
            padding: 4

            // 设为就绪
            Item {
                width: contextMenu.width - 8
                height: 28

                Rectangle {
                    id: createHostBg
                    anchors.fill: parent
                    color: createHostMa.containsMouse && isConnected && !isReady && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) :
                           "transparent"
                    radius: 1
                }

                Text {
                    text: "设为就绪"
                    color: isConnected && !isReady && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: isConnected && !isReady && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: createHostMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: isConnected && !isReady && hostAddress !== 0

                    onClicked: {
                        contextMenu.close()
                        if (hostManager.createHost(hostAddress)) {
                            console.log("成功创建主机实例：0x" + hostAddress.toString(16).toUpperCase())
                        } else {
                            console.log("创建:0x" + hostAddress.toString(16).toUpperCase() + "主机实例失败")
                        }
                    }
                }
            }

            // 设置为活动主机
            Item {
                width: contextMenu.width - 8
                height: 28

                Rectangle {
                    id: activateHostBg
                    anchors.fill: parent
                    color: activateHostMa.containsMouse && (isConnected || isReady) && !isActive && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) :
                           "transparent"
                    radius: 1
                }

                Text {
                    text: "设置为活动主机"
                    color: (isConnected || isReady) && !isActive && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: (isConnected || isReady) && !isActive && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: activateHostMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: (isConnected || isReady) && !isActive && hostAddress !== 0

                    onClicked: {
                        contextMenu.close()
                        if (hostManager.activateHost(hostAddress)) {
                            console.log("成功设置:0x" + hostAddress.toString(16).toUpperCase() + "为活动主机")
                        } else {
                            console.log("设置:0x" + hostAddress.toString(16).toUpperCase() + "为活动主机失败")
                        }
                    }
                }
            }

            // 停用主机
            Item {
                width: contextMenu.width - 8
                height: 28

                Rectangle {
                    id: stopHostBg
                    anchors.fill: parent
                    color: stopHostMa.containsMouse && isActive && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) :
                           "transparent"
                    radius: 1
                }

                Text {
                    text: "停用主机"
                    color: isActive && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: isActive && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: stopHostMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: isActive && hostAddress !== 0

                    onClicked: {
                        contextMenu.close()
                        if (hostManager.stopHost(hostAddress)) {
                            console.log("成功停用主机：0x" + hostAddress.toString(16).toUpperCase())
                        } else {
                            console.log("停用主机：0x" + hostAddress.toString(16).toUpperCase() + "失败")
                        }
                    }
                }
            }

            // 移除主机
            Item {
                width: contextMenu.width - 8
                height: 28

                Rectangle {
                    id: removeHostBg
                    anchors.fill: parent
                    color: removeHostMa.containsMouse && isReady && !isActive && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) :
                           "transparent"
                    radius: 1
                }

                Text {
                    text: "移除主机"
                    color: isReady && !isActive && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: isReady && !isActive && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: removeHostMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: isReady && !isActive && hostAddress !== 0

                    onClicked: {
                        contextMenu.close()
                        if (hostManager.removeHost(hostAddress)) {
                            console.log("成功移除主机实例：0x" + hostAddress.toString(16).toUpperCase())
                        } else {
                            console.log("移除主机实例：0x" + hostAddress.toString(16).toUpperCase() + "失败")
                        }
                    }
                }
            }

            // 分隔线
            Rectangle {
                width: contextMenu.width - 8
                height: 1
                color: Theme.mBorderLine
                opacity: 0.3
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // 刷新主机信息
            Item {
                width: contextMenu.width - 8
                height: 28

                Rectangle {
                    id: refreshHostBg
                    anchors.fill: parent
                    color: refreshHostMa.containsMouse && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) :
                           "transparent"
                    radius: 1
                }

                Text {
                    text: "刷新主机信息"
                    color: hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: refreshHostMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: hostAddress !== 0

                    onClicked: {
                        contextMenu.close()
                        console.log("刷新主机信息：0x" + hostAddress.toString(16).toUpperCase())
                        hostInfo = hostManager.getAllHostMap(hostAddress)
                    }
                }
            }
        }
    }

    // ========== 主布局：上中下三部分 ==========
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 3

        // ---------- 上部 (浅色) ----------
        CustomRectangle {
            id:topSection
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.mPanel
            radius: 3
            angle1: true
            angle2: true
            angle3: false
            angle4: false

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 16

                Text {
                    text: hostName
                    color: Theme.mTextHim
                    font.pixelSize: 11
                    font.bold: true
                    opacity: 0.9
                }

                Item { Layout.fillWidth: true }  // 弹性撑开

                // 右上角圆形 - 用于显示选中状态
                Rectangle {
                    id: selectionIndicator
                    Layout.preferredHeight: 12
                    Layout.preferredWidth: 12
                    radius: selectionIndicator.width / 2
                    color: "#00AF69"
                    border.width: 1
                    border.color: "#A0D0D0"
                    visible: isSelected
                    opacity: isSelected ? 0.9 : 0.0

                    // 添加动画效果
                    Behavior on color {
                        ColorAnimation { duration: 300 }
                    }

                    // 添加渐显渐隐动画
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 500
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }

        // ---------- 中部 (深色) 主要部分 ----------
        Rectangle {
            id: middleSection
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.mBackground

            // 中部的鼠标交互（主要交互区域）
            MouseArea {
                id: middleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onEntered: {
                    hostImage.scale = 1.15
                    middleSection.color = "#202020"
                }

                onExited: {
                    hostImage.scale = 1.0
                    middleSection.color = Theme.mBackground
                }

                onPressed: function(mouse) {
                    // console.log("中部鼠标按下 - 按钮:", mouse.button, "地址:", hostAddress)
                    if (mouse.button === Qt.LeftButton) {
                        if (hostAddress !== 0) {
                            // console.log("中部左键点击，地址:", "0x" + hostAddress.toString(16).toUpperCase())
                            selected(hostAddress)
                        }
                    } else if (mouse.button === Qt.RightButton) {
                        if (hostAddress !== 0) {
                            contextMenu.popup()
                        }
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 8
                anchors.bottomMargin: 10
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 10

                Image {
                    id: hostImage
                    width: 44
                    height: 26
                    source: {
                        if (isActive) return "qrc:/image/app-host-isActive.svg"
                        else if(isReady) return "qrc:/image/app-host-isReady.svg"
                        else if (isConnected) return "qrc:/image/app-host-isConnected.svg"
                        else return "qrc:/image/app-host-isNone.svg"
                    }
                    sourceSize.width: 44
                    sourceSize.height: 26
                    fillMode: Image.PreserveAspectFit
                    opacity: 1

                    Behavior on scale {
                        NumberAnimation { duration: 250 }
                    }
                }

                // 中间文本区域 (从下到上三个文本标签)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6

                    Item { Layout.preferredHeight: 1; Layout.fillWidth: true }

                    // 状态标签及文本
                    RowLayout {
                        Label {
                            text: "状态："
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.mText
                        }

                        Text {
                            text: {
                                switch(statusValue) {
                                    case 0: return "未知";
                                    case 1: return "已连接";
                                    case 2: return "已就绪";
                                    case 3: return "活动中";
                                    case 4: return "忙碌";
                                    case 5: return "错误";
                                    default: return "未知";
                                }
                            }
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.mText
                            Layout.leftMargin: 4
                        }
                    }

                    // 单元数量显示
                    RowLayout {
                        Label {
                            text: "序列号： "
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.mText
                        }
                        Text {
                            text: serialNumber
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.mText
                        }
                    }
                }

                Text {
                    text: addressStr
                    color: Theme.mText
                    font.pixelSize: 13
                    font.bold: true
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        // ---------- 下部 (浅色) ----------
        CustomRectangle {
            id: bottomSection
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.mPanel
            radius: 3
            angle1: false
            angle2: false
            angle3: true
            angle4: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                spacing: 16
                opacity: 0.8

                Text {
                    text: "连接时间：" + (lastSeenTime || "--:--:--")
                    color: Theme.mTextHim
                    font.pixelSize: 10
                    font.bold: false
                }
            }
        }
    }

    // ========== 监听主机信息变化 ==========
    Connections {
        target: hostManager
        function onHostInfoChanged(address) {
            if (address === hostUnitRoot.hostAddress || address === 0) {
                // 强制重新获取数据
                hostInfo = hostManager.getAllHostMap(hostUnitRoot.hostAddress)
                console.log("主机信息更新: 地址=" + addressStr + ", 新状态=" + statusText)
            }
        }
    }

    // 调试输出
    Component.onCompleted: {
        // console.log("CustomHostUnit 创建: 地址=" + addressStr +
        //            ", 名称=" + hostName +
        //            ", 状态=" + statusText)
    }

    onHostInfoChanged: {
        // console.log("CustomHostUnit 数据刷新: 地址=" + addressStr + ", 状态=" + statusText)
    }
}
