// host_info_xdc.qml - XDC236 主机属性显示组件
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

Item {
    id: xdcinfo_root
    anchors.fill: parent

    // 对外暴露的属性
    property int hostAddress: 0
    property var controller: hostAddress > 0 ? hostManager.getController(hostAddress) : null

    // 辅助属性用于强制刷新显示
    property bool forceUpdate: false

    // 获取各个设置对象（使用 view 前缀）
    property var hostProps: controller ? controller.viewHardwareInfo : null
    property var volumeSettings: controller ? controller.viewVolumeSettings : null
    property var speechSettings: controller ? controller.viewSpeechSettings : null
    property var cameraSettings: controller ? controller.viewCameraSettings : null

    // 监听地址变化，重新获取控制器
    onHostAddressChanged: {
        controller = hostAddress > 0 ? hostManager.getController(hostAddress) : null
        // 刷新显示
        forceUpdate = !forceUpdate
    }

    Rectangle {
        id: propertyRect
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.borderLine
        border.width: 0
        radius: 2

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            // 当前选中的主机标题
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 0

                Label {
                    text: "当前主机：XDC236 融合会议主机，地址："
                    color: Theme.text
                    font.pixelSize: 12
                    font.bold: true
                    Layout.preferredWidth: 220
                }
                Label {
                    text: "0x" + (hostAddress ? hostAddress.toString(16).toUpperCase().padStart(2, '0') : "00")
                    color: Theme.textMenu
                    font.pixelSize: 12
                    font.bold: true
                    Layout.preferredWidth: 60
                    opacity: 0.8
                }
                Item { Layout.fillWidth: true }
            }

            // 系统时间网格 - 0x0204
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "系统时间："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: {
                        if (controller && controller.viewSystemDateTime) {
                            var dt = controller.viewSystemDateTime
                            var dummy = xdcinfo_root.forceUpdate
                            return Qt.formatDateTime(dt, "yyyy-MM-dd hh:mm:ss")
                        }
                        return "--"
                    }
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }
                // 同步时间按钮
                CustomButton {
                    text: "同步PC时间"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 24
                    visible: controller !== null
                    onClicked: {
                        if (controller) {
                            var now = new Date()
                            var utcTime = Math.floor(now.getTime() / 1000)
                            controller.setSystemDateTime(utcTime)
                            console.log("设置系统时间:", now.toLocaleString())
                        }
                    }
                }
                Item { Layout.columnSpan: 3 }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 版本与版本时间网格 - 0x0202/0x0203
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "内核版本："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Row {
                    Layout.preferredWidth: 80 * 2
                    spacing: 5
                    Label {
                        text: (hostProps && hostProps.kernelVersion) ? (hostProps.kernelVersion + " /") : "-- /"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                    Label {
                        text: (hostProps && hostProps.kernelDate) ? hostProps.kernelDate : "--"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                }

                Label {
                    text: "LINK 版本："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Row {
                    Layout.preferredWidth: 80 * 2
                    spacing: 5
                    Label {
                        text: (hostProps && hostProps.linkVersion) ? (hostProps.linkVersion + " /") : "-- /"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                    Label {
                        text: (hostProps && hostProps.linkDate) ? hostProps.linkDate : "--"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                }

                Label {
                    text: "天线盒版本："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Row {
                    Layout.preferredWidth: 80 * 2
                    spacing: 5
                    Label {
                        text: (hostProps && hostProps.antaVersion) ? (hostProps.antaVersion + " /") : "-- /"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                    Label {
                        text: (hostProps && hostProps.antaDate) ? hostProps.antaDate : "--"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: Font.Normal
                    }
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 发言模式信息网格 - 0x0208 / 0x0209
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "发言模式："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: {
                        if (speechSettings) {
                            var mode = speechSettings.speechMode
                            if (mode === 0) return "先进先出 (FIFO)"
                            if (mode === 1) return "自锁模式 (SelfLock)"
                        }
                        return "--"
                    }
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "最大发言数："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (speechSettings && speechSettings.maxSpeechCount !== undefined) ?
                          speechSettings.maxSpeechCount + " / " + speechSettings.hardwareMaxSpeech : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 音量信息网格 - 0x0205/0x0206/0x0207
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "无线音量："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (volumeSettings && volumeSettings.wirelessVolume !== undefined) ?
                          (volumeSettings.wirelessVolume + 1) + " / " + volumeSettings.wirelessMaxVolume : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "有线音量："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (volumeSettings && volumeSettings.wiredVolume !== undefined) ?
                          (volumeSettings.wiredVolume + 1) + " / " + volumeSettings.wiredMaxVolume : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "天线盒音量："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (volumeSettings && volumeSettings.antennaVolume !== undefined) ?
                          (volumeSettings.antennaVolume + 1) + " / " + volumeSettings.antennaMaxVolume : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 无线音频路径网格 - 0x020D
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "无线音频路径："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: {
                        if (speechSettings) {
                            var path = speechSettings.wirelessAudioPath
                            if (path === 0) return "主机"
                            if (path === 1) return "天线盒"
                        }
                        return "--"
                    }
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                // 当前发言ID显示
                Label {
                    text: "当前无线发言ID："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (speechSettings && speechSettings.currentWirelessSpeakerId) ?
                          "0x" + speechSettings.currentWirelessSpeakerId.toString(16).toUpperCase().padStart(4, '0') : "0x0000"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "当前有线发言ID："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (speechSettings && speechSettings.currentWiredSpeakerId) ?
                          "0x" + speechSettings.currentWiredSpeakerId.toString(16).toUpperCase().padStart(4, '0') : "0x0000"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 单元容量网格 - 0x0210
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "单元容量："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (speechSettings && speechSettings.maxUnitCapacity !== undefined) ?
                              speechSettings.maxUnitCapacity : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Item { Layout.preferredWidth: 80 }
                Item { Layout.preferredWidth: 80 * 2 }
                Item { Layout.preferredWidth: 80 }
                Item { Layout.preferredWidth: 80 * 2 }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
            }

            // 摄像机协议信息网格 - 0x020A/0x020B/0x020C
            GridLayout {
                columns: 6
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true
                Layout.leftMargin: 30

                Label {
                    text: "摄像机协议："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: {
                        if (cameraSettings) {
                            var protocol = cameraSettings.protocol
                            if (protocol === 0) return "无"
                            if (protocol === 1) return "VISCA"
                            if (protocol === 2) return "PELCO-D"
                            if (protocol === 3) return "PELCO-P"
                        }
                        return "--"
                    }
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "摄像机地址："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: (cameraSettings && cameraSettings.address) ?
                          cameraSettings.address : "--"
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }

                Label {
                    text: "波特率："
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80
                }
                Label {
                    text: {
                        if (cameraSettings) {
                            var baudrate = cameraSettings.baudrate
                            if (baudrate === 0) return "2400 bps"
                            if (baudrate === 1) return "4800 bps"
                            if (baudrate === 2) return "9600 bps"
                            if (baudrate === 3) return "115200 bps"
                        }
                        return "-- bps"
                    }
                    color: Theme.text
                    font.pixelSize: 11
                    font.weight: Font.Normal
                    Layout.preferredWidth: 80 * 2
                }
            }
        }

        // 监听控制器信号，强制刷新显示
        Connections {
            target: controller
            enabled: controller !== null

            // 使用现有的分组信号
            function onHardwareInfoChanged() {
                xdcinfo_root.forceUpdate = !xdcinfo_root.forceUpdate
            }
            function onVolumeSettingsChanged() {
                xdcinfo_root.forceUpdate = !xdcinfo_root.forceUpdate
            }
            function onSpeechSettingsChanged() {
                xdcinfo_root.forceUpdate = !xdcinfo_root.forceUpdate
            }
            function onCameraSettingsChanged() {
                xdcinfo_root.forceUpdate = !xdcinfo_root.forceUpdate
            }
            function onSystemDateTimeUpdated(address, dateTime) {
                xdcinfo_root.forceUpdate = !xdcinfo_root.forceUpdate
            }
        }
    }
}
