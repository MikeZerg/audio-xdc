import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: root

    property var unitData
    property var unitModel

    signal showDetail()

    height: 80
    radius: Theme.radiusL
    color: Theme.surface
    border.color: Theme.borderLine
    border.width: 1

    // 发言状态指示灯颜色
    function speakingColor() {
        if (!unitData.isOnline) return Theme.disabled
        if (unitData.isSpeaking) return Theme.success
        if (unitData.isOpened) return Theme.accent1
        return Theme.borderLineL
    }

    // 低电量警告
    function batteryIconColor() {
        if (!unitData.isWireless) return "transparent"
        if (unitData.isLowBattery) return Theme.error
        if (unitData.batteryPercent > 50) return Theme.success
        return Theme.warning
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // 发言状态指示灯
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: speakingColor()
        }

        // 类型图标
        Rectangle {
            width: 40
            height: 40
            radius: 20
            color: unitData.isWireless ? Qt.rgba(0.14, 0.49, 0.56, 0.2) : Qt.rgba(0.53, 0.37, 0.60, 0.15)

            Text {
                anchors.centerIn: parent
                text: unitData.isWireless ? "📡" : "🎤"
                font.pixelSize: 20
            }
        }

        // 单元信息
        ColumnLayout {
            spacing: 4

            RowLayout {
                spacing: 8

                Text {
                    text: unitData.unitIdHex
                    font.bold: true
                    font.pixelSize: Theme.fontSize14
                    color: Theme.mTextHim
                }

                Text {
                    text: unitData.typeName
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSize12
                }

                Rectangle {
                    visible: unitData.identity !== 0x10  // 非代表身份
                    width: identityText.width + 12
                    height: 18
                    radius: 9
                    color: unitData.identity === 0x30 ? Qt.rgba(1.0, 0.6, 0.0, 0.15) : Qt.rgba(0.3, 0.7, 0.3, 0.15)

                    Text {
                        id: identityText
                        anchors.centerIn: parent
                        text: unitData.identityName
                        color: unitData.identity === 0x30 ? Theme.warning : Theme.success
                        font.pixelSize: 10
                    }
                }
            }

            Text {
                text: unitData.alias || "未命名"
                color: Theme.text
                font.pixelSize: Theme.fontSize12
                elide: Text.ElideRight
                Layout.maximumWidth: 200
            }

            Text {
                text: unitData.statusText
                color: unitData.isSpeaking ? Theme.success : Theme.disabled
                font.pixelSize: 11
            }
        }

        Item { Layout.fillWidth: true }

        // 无线单元状态显示
        GridLayout {
            visible: unitData.isWireless
            columns: 2
            rows: 2
            columnSpacing: 8
            rowSpacing: 4
            Layout.alignment: Qt.AlignRight

            // 电池图标
            Item {
                Layout.preferredWidth: 22
                Layout.preferredHeight: 11
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.leftMargin: -5

                Rectangle {
                    anchors.fill: parent
                    radius: 2
                    border.color: batteryIconColor()
                    border.width: 1
                    color: "transparent"

                    // 电池头
                    Rectangle {
                        width: 2
                        height: 3
                        radius: 0.5
                        color: batteryIconColor()
                        anchors {
                            right: parent.right
                            rightMargin: -2
                            verticalCenter: parent.verticalCenter
                        }
                    }

                    // 电池电量
                    Rectangle {
                        width: (parent.width - 4) * (unitData.batteryPercent / 100)
                        height: parent.height - 4
                        radius: 1
                        color: batteryIconColor()
                        anchors {
                            left: parent.left
                            leftMargin: 2
                            verticalCenter: parent.verticalCenter
                        }
                    }
                    // 电池充电图标
                    Text {
                        id:inChargeIcon
                        anchors.centerIn: parent
                        text: String.fromCharCode(126)
                        font.family: "Webdings"
                        font.pixelSize: 13
                        color: Theme.error
                        visible: unitData.chargingStatus === 1
                    }
                }
            }

            // 电池百分比文本
            Text {
                text: unitData.batteryPercent + " %"
                color: unitData.isLowBattery ? Theme.error : Theme.text
                font.pixelSize: 10
                font.family: Theme.fontFamily
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.preferredWidth: 35
            }

            // 信号图标
            Image {
                source: {
                    var rssi = unitData.rssi
                    var level = Math.floor((rssi + 100) / 12)
                    level = Math.max(0, Math.min(5, level))
                    switch(level) {
                        case 5: return "qrc:/image/app-signal5.svg"
                        case 4: return "qrc:/image/app-signal41.svg"
                        case 3: return "qrc:/image/app-signal3.svg"
                        case 2: return "qrc:/image/app-signal2.svg"
                        default: return "qrc:/image/app-signal1.svg"
                    }
                }
                sourceSize.width: 24
                sourceSize.height: 14
                fillMode: Image.PreserveAspectFit
                smooth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.preferredWidth: 24
                Layout.preferredHeight: 14
            }

            // 信号强度文本
            Text {
                text: unitData.rssi + " dBm"
                color: Theme.text
                font.pixelSize: 10
                font.family: Theme.fontFamily
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.preferredWidth: 35
            }
        }


        // 操作按钮
        ColumnLayout {
            spacing: 4

            CustomButton {
                id: btnOpenClose
                Layout.preferredWidth: 60
                Layout.preferredHeight: 22

                text: unitData.isOpened ? "关闭" : "打开"
                font.pixelSize: 10
                highlighted: !unitData.isOpened
                enabled: unitData.isOnline
                onClicked: {
                    if (unitData.isOpened)
                        unitModel.closeUnit(unitData.unitId)
                    else
                        unitModel.openUnit(unitData.unitId)
                }
            }

            CustomButton {
                id: btnUnitDetails
                Layout.preferredWidth: 60
                Layout.preferredHeight: 22

                text: "详情"
                font.pixelSize: 10
                flat: true
                onClicked: root.showDetail()
            }
        }
    }

    // 鼠标悬停效果
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.showDetail()

        onEntered: parent.border.color = Theme.borderLineL
        onExited: parent.border.color = Theme.borderLine
    }
}
