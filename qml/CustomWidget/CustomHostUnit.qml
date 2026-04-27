// qml/CustomWidget/CustomHostUnit.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: hostUnitRoot
    width: 210
    height: 130

    // ========== 只接收地址作为参数 ==========
    required property int hostAddress

    // 模型单元被选中信号
    signal selected(int address)    // 通知被选中的地址信号

    // ========== 通过地址查询完整信息 ==========
    property var hostInfo: ({})  // 初始化为空对象

    // 刷新主机信息的函数
    function refreshHostInfo() {
        if (hostAddress !== 0) {
            hostInfo = hostManager.getAllHostMap(hostAddress)
            console.log("刷新主机信息: 地址=0x" + hostAddress.toString(16) +
                       ", 状态=" + (hostInfo ? hostInfo.statusText : "未知"))
        }
    }

    // 组件完成时和地址变化时获取数据
    Component.onCompleted: {
        refreshHostInfo()
    }

    onHostAddressChanged: {
        refreshHostInfo()
    }

    // ========== 显式声明所有 UI 需要的属性 ==========
    property string addressStr: hostInfo ? (hostInfo.addressHex || "0x00") : "0x00"
    property string hostName: hostInfo ? (hostInfo.deviceTypeName || "Unknown") : "Unknown"
    property int statusValue: hostInfo ? (hostInfo.status || 0) : 0

    property string statusText: hostInfo ? (hostInfo.statusText || "未知") : "未知"
    property string lastSeenTime: hostInfo ? (hostInfo.lastSeen || "--:--:--") : "--:--:--"

    property string serialNumber: hostInfo ? (hostInfo.serialNumber || "----") : "----"
    property bool isConnected: (statusValue === 1)
    property bool isReady: (statusValue === 2)
    property bool isBusy: (statusValue === 3)

    // [修复] 只保留一个 isSelected 定义，使用整型地址比较
    property bool isSelected: {
        // 使用 selectedHostAddressInt 确保是数字类型
        var selectedAddr = hostManager.selectedHostAddressInt
        return selectedAddr === hostAddress
    }

    // 状态颜色
    property color statusColor: {
        switch(statusValue) {
            case 0: return "#B3B3B3";  // None
            case 1: return "#87CEFA";  // Connected (探测到但未实例化)
            case 2: return "#4169E1";  // Ready (已实例化)
            case 3: return "#ED5736";  // Busy
            case 4: return "#9D2933";  // Error
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

            // 1. 设为就绪（创建主机实例）
            Item {
                width: contextMenu.width - 8
                height: 28
                Rectangle {
                    id: createHostBg
                    anchors.fill: parent
                    color: createHostMa.containsMouse && isConnected && !isReady && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) : "transparent"
                    radius: 1
                }
                Text {
                    text: "设为就绪 (创建实例)"
                    color: isConnected && !isReady && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: isConnected && !isReady && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    id: createHostMa
                    anchors.fill: parent; hoverEnabled: true
                    enabled: (statusValue === 1)
                    onClicked: {
                        contextMenu.close()
                        console.log("右键菜单：设为就绪 - 地址 0x" + hostAddress.toString(16))
                        var success = hostManager.createHost(hostAddress)
                        if (success) {
                            console.log("创建成功，自动选中主机")
                            hostManager.selectHost(hostAddress)
                        }
                    }
                }
            }

            // 2. 移除主机
            Item {
                width: contextMenu.width - 8
                height: 28
                Rectangle {
                    id: removeHostBg
                    anchors.fill: parent
                    color: removeHostMa.containsMouse && isReady && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) : "transparent"
                    radius: 1
                }
                Text {
                    text: "移除主机 (销毁实例)"
                    color: isReady && hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: isReady && hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    id: removeHostMa
                    anchors.fill: parent; hoverEnabled: true
                    enabled: isReady && hostAddress !== 0
                    onClicked: {
                        contextMenu.close()
                        console.log("右键菜单：移除主机 - 地址 0x" + hostAddress.toString(16))
                        hostManager.removeHost(hostAddress)
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

            // 3. 刷新主机信息
            Item {
                width: contextMenu.width - 8; height: 28
                Rectangle {
                    id: refreshHostBg
                    anchors.fill: parent
                    color: refreshHostMa.containsMouse && hostAddress !== 0 ?
                           Qt.rgba(Theme.mProgressBar.r, Theme.mProgressBar.g, Theme.mProgressBar.b, 0.2) : "transparent"
                    radius: 1
                }
                Text {
                    text: "刷新主机信息"
                    color: hostAddress !== 0 ? Theme.mText : Qt.darker(Theme.mText, 1.5)
                    opacity: hostAddress !== 0 ? 1.0 : 0.4
                    font.pixelSize: 11
                    anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    id: refreshHostMa
                    anchors.fill: parent; hoverEnabled: true; enabled: hostAddress !== 0
                    onClicked: {
                        contextMenu.close()
                        refreshHostInfo()
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
            id: topSection
            Layout.fillWidth: true; Layout.preferredHeight: 24
            angle1: true; angle2: true; angle3: false; angle4: false
            color: Theme.mPanel
            radius: 3

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12
                spacing: 16

                Text {
                    text: hostName
                    color: Theme.mTextHim; font.pixelSize: 11; font.bold: true; opacity: 0.9
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Item { Layout.fillWidth: true }  // 弹性撑开

                // 右上角圆形 - 用于显示选中状态
                Rectangle {
                    id: selectionIndicator
                    Layout.preferredHeight: 12; Layout.preferredWidth: 12
                    radius: selectionIndicator.width / 2
                    color: "#00AF69"
                    border.width: 1; border.color: "#A0D0D0"
                    visible: isSelected
                    opacity: isSelected ? 0.9 : 0.0
                    Behavior on color { ColorAnimation { duration: 300 } }
                    Behavior on opacity { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
                }
            }
        }

        // ---------- 中部 (深色) 主要部分 ----------
        Rectangle {
            id: middleSection
            Layout.fillWidth: true; Layout.fillHeight: true
            color: Theme.mBackground

            // 中部的鼠标交互（主要交互区域）
            MouseArea {
                id: middleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onEntered: { hostImage.scale = 1.15; middleSection.color = "#202020" }
                onExited: { hostImage.scale = 1.0; middleSection.color = Theme.mBackground }

                // 右键弹出菜单
                onPressed: function(mouse) {
                    // 右键菜单保持不变
                    if (mouse.button === Qt.RightButton) {
                        if (hostAddress !== 0) contextMenu.popup()
                    }
                }

                // 左键双击事件
                onDoubleClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        if (hostAddress !== 0) {
                            console.log("左键双击主机: 0x" + hostAddress.toString(16))
                            selected(hostAddress)
                        }
                    }
                }
            }

            RowLayout {
                anchors.fill: parent; anchors.topMargin: 8; anchors.bottomMargin: 10
                anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 10

                Image {
                    id: hostImage
                    width: 44
                    height: 26
                    source: {
                        if(isReady) return "qrc:/image/app-host-isReady.svg"
                        else if (isConnected) return "qrc:/image/app-host-isConnected.svg"
                        else if (isBusy) return "qrc:/image/app-host-isBusy.svg"
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

                // 中间文本区域
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                    Item { Layout.preferredHeight: 1; Layout.fillWidth: true }

                    // 状态标签及文本
                    RowLayout {
                        Label {
                            text: "状态："
                            font.pixelSize: 10; font.bold: true; color: Theme.mText
                        }
                        Text {
                            text: statusText
                            font.pixelSize: 10; font.bold: true; color: Theme.mText; Layout.leftMargin: 4
                        }
                    }

                    // 单元序列号
                    RowLayout {
                        Label {
                            text: "序列号： "
                            font.pixelSize: 10; font.bold: true; color: Theme.mText
                        }
                        Text {
                            text: serialNumber
                            font.pixelSize: 10; font.bold: true; color: Theme.mText
                            elide: Text.ElideRight
                            maximumLineCount: 1
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
            Layout.fillWidth: true; Layout.preferredHeight: 24
            angle1: false; angle2: false; angle3: true; angle4: true
            color: Theme.mPanel
            radius: 3

            // 最后连接时间
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; spacing: 16; opacity: 0.8
                Text {
                    text: "连接时间：" + (lastSeenTime || "--:--:--")
                    color: Theme.mTextHim; font.pixelSize: 10; font.bold: false
                }
            }
        }
    }

    // ========== 监听主机信息变化 ==========
    Connections {
        target: hostManager

        function onHostInfoChanged(address) {
            // 如果更新的地址是当前卡片，刷新数据
            if (address === hostUnitRoot.hostAddress || address === 0) {
                refreshHostInfo()
                console.log("主机信息更新: 地址=" + addressStr + ", 新状态=" + statusText)
            }
        }

        function onSelectedHostChanged(oldAddr, newAddr) {
            // 通知 isSelected 属性重新计算
            if (oldAddr === hostAddress || newAddr === hostAddress) {
                isSelectedChanged()
                console.log("选中状态变化: 地址=0x" + hostAddress.toString(16) +
                           ", isSelected=" + isSelected)
            }
        }
    }
}