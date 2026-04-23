import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Popup {
    id: root

    property var meetingManager: null

    signal checkinCreated(int durationMinutes, bool allowLate, bool allowResend)

    property int durationMinutes: 10
    property bool allowLateCheckin: true
    property bool allowResendCheckin: true
    property bool onlyWiredCheckin: false

    width: parent.width * 0.4
    height: parent.height
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: parent.width
    y: (parent.height - height) / 2
    padding: 0
    modal: true

    enter: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 15
            to: parent.width + 15 - root.width
            easing.type: Easing.OutCubic
            duration: 350
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 15 - root.width
            to: parent.width + 15
            easing.type: Easing.InCubic
            duration: 200
        }
    }

    background: Rectangle {
        color: Theme.background
    }

    function resetForm() {
        durationMinutes = 5
        allowLateCheckin = true
        allowResendCheckin = true
        onlyWiredCheckin = false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "创建签到"
                font.bold: true
                font.pixelSize: 16
                font.family: Theme.fontFamily
                color: Theme.text
            }
            Item { Layout.fillWidth: true }
            CustomButton {
                text: "✕"
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                Layout.rightMargin: 4
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: "transparent"
                onClicked: root.close()
            }
        }

        Rectangle {
            height: 1
            Layout.fillWidth: true
            color: Theme.borderLine
        }

        // 直接使用 ColumnLayout，无需滚动组件
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "签到时效 (分钟)"; color: Theme.text }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    CustomSlider {
                        id: durationSlider
                        from: 1
                        to: 30
                        stepSize: 1
                        value: durationMinutes
                        onValueChanged: durationMinutes = value
                        Layout.fillWidth: true
                        progressHeight: 6
                        handleSize: 14
                        handleRadius: 2
                    }
                    Text {
                        text: durationMinutes + "分钟"
                        color: Theme.textMenu
                        Layout.preferredWidth: 40
                    }
                }
            }

            ColumnLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "签到选项"; color: Theme.text }

                CustomCheckBox {
                    text: "允许迟到签到"
                    checked: allowLateCheckin
                    onCheckedChanged: allowLateCheckin = checked
                }

                CustomCheckBox {
                    text: "允许重发签到事件"
                    checked: allowResendCheckin
                    onCheckedChanged: allowResendCheckin = checked
                }

                CustomCheckBox {
                    text: "仅有线单元签到"
                    checked: onlyWiredCheckin
                    onCheckedChanged: onlyWiredCheckin = checked
                    enabled: false
                }
            }
        }

        // 弹簧，把按钮推到底部
        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderLine
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CustomButton {
                text: "取消"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                onClicked: root.close()
            }

            CustomButton {
                id: btnCreateCheckin
                text: "开始签到"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: {
                    checkinCreated(durationMinutes, allowLateCheckin, allowResendCheckin)
                    root.close()
                }
            }
        }
    }
}