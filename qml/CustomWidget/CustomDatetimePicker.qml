import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dateTimeDialog
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: "📅 选择日期和时间"

    // === 可调属性 ===
    property int dialogWidth: 320
    property int dialogHeight: 340
    property int customSpinWidth: 80

    width: dialogWidth
    height: dialogHeight
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: parent.width
    y: (parent.height - height) / 2
    padding: 0

    enter: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 20
            to: parent.width + 20 - root.width
            easing.type: Easing.OutCubic
            duration: 350
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 20 - root.width
            to: parent.width + 20
            easing.type: Easing.InCubic
            duration: 200
        }
    }


    property date selectedDateTime: new Date()
    signal dateTimeConfirmed(date selectedDate)

    // 背景样式
    background: Rectangle {
        color: Theme.panel
        radius: Theme.radiusS
        border.color: Theme.borderLine
        border.width: 0
    }

    // 标题栏样式
    header: Rectangle {
        height: 40
        color: Theme.surface
        radius: Theme.radiusL

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10

            Text {
                text: "📅 选择日期和时间"
                font.pixelSize: 12
                color: Theme.text
                Layout.fillWidth: true
            }

            CustomButton {
                text: "✕"
                Layout.preferredHeight: 22
                Layout.preferredWidth: 22
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: "transparent"
                onClicked: dateTimeDialog.reject()
            }
        }
    }

    ColumnLayout {
        spacing: 15
        anchors.fill: parent
        anchors.margins: 15

        // 日期选择区域
        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true

            Text {
                text: "选择日期"
                font.pixelSize: 11
                color: Theme.text
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignHCenter

                CustomSpin {
                    id: yearSpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 2020
                    to: 2045
                    step: 1
                    value: new Date().getFullYear()
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }

                CustomSpin {
                    id: monthSpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 1
                    to: 12
                    step: 1
                    value: new Date().getMonth() + 1
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }

                CustomSpin {
                    id: daySpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 1
                    to: 31
                    step: 1
                    value: new Date().getDate()
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }
            }
        }

        // 时间选择区域
        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true

            Text {
                text: "选择时间"
                font.pixelSize: 11
                font.family: Theme.fontFamily
                color: Theme.text
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignHCenter

                CustomSpin {
                    id: hourSpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 0
                    to: 23
                    step: 1
                    value: new Date().getHours()
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }

                CustomSpin {
                    id: minuteSpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 0
                    to: 59
                    step: 1
                    value: new Date().getMinutes()
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }

                CustomSpin {
                    id: secondSpin
                    spinWidth: customSpinWidth
                    spinHeight: 24
                    from: 0
                    to: 59
                    step: 1
                    value: new Date().getSeconds()
                    fontSize: 10
                    fontColor: Theme.textMenu
                    backgroundColor: Theme.surface
                    numberBackgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    leftBorderWidth: 0.5
                    centerBorderWidth: 0.5
                    rightBorderWidth: 0.5
                    Layout.preferredWidth: customSpinWidth
                    onValueModified: updateDateTime()
                }
            }
        }

        // 当前选择显示区域
        ColumnLayout {
            spacing: 8
            Layout.fillWidth: true

            Text {
                text: "当前时间"
                font.pixelSize: 11
                color: Theme.text
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                spacing: 12

                CustomTextField {
                    id: selectedTimeField
                    text: Qt.formatDateTime(selectedDateTime, "yyyy-MM-dd HH:mm:ss")
                    textColor: Theme.text
                    font.pixelSize: 10
                    Layout.preferredWidth: customSpinWidth *2
                    Layout.preferredHeight: 24
                    backgroundColor: Theme.surface
                    borderColor: Theme.borderLine
                    placeholderColor: Theme.textDim
                    borderRadius: Theme.radiusS
                    selectByMouse: true
                    readOnly: true
                }

                CustomButton {
                    text: "重置"
                    textColor: Theme.text
                    font.pixelSize: 10
                    Layout.preferredWidth: customSpinWidth
                    Layout.preferredHeight: 24
                    backgroundColor: "transparent"
                    hoverColor: Theme.surfaceSelected
                    pressedColor: Theme.surfaceSelected
                    borderColor: Theme.borderLine
                    borderWidth: 0.5
                    onClicked: setCurrentTime()
                }
            }
        }

        // 弹簧，把按钮推到底部
        Item { Layout.fillHeight: true }
    }

    // 底部按钮栏
    footer: Rectangle {
        height: 45
        color: Theme.surface
        radius: Theme.radiusL

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 10

            Item { Layout.fillWidth: true }

            CustomButton {
                text: "取消"
                implicitWidth: 70
                implicitHeight: 24
                font.pixelSize: 10
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: Theme.borderLine
                borderWidth: 0.5
                textColor: Theme.text
                onClicked: dateTimeDialog.reject()
            }

            CustomButton {
                text: "确定"
                implicitWidth: 70
                implicitHeight: 24
                font.pixelSize: 10

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                borderColor: Theme.borderLine
                borderWidth: 0.5
                textColor: Theme.text
                onClicked: dateTimeDialog.accept()
            }
        }
    }

    Component.onCompleted: {
        setCurrentTime()
    }

    function updateDateTime() {
        var daysInMonth = new Date(yearSpin.value, monthSpin.value, 0).getDate()
        if (daySpin.value > daysInMonth) {
            daySpin.value = daysInMonth
        }

        selectedDateTime = new Date(
            yearSpin.value,
            monthSpin.value - 1,
            daySpin.value,
            hourSpin.value,
            minuteSpin.value,
            secondSpin.value
        )

        selectedTimeField.text = Qt.formatDateTime(selectedDateTime, "yyyy-MM-dd HH:mm:ss")
    }

    function setCurrentTime() {
        var now = new Date()
        yearSpin.value = now.getFullYear()
        monthSpin.value = now.getMonth() + 1
        daySpin.value = now.getDate()
        hourSpin.value = now.getHours()
        minuteSpin.value = now.getMinutes()
        secondSpin.value = now.getSeconds()
    }

    function clearSelection() {
        yearSpin.value = 2000
        monthSpin.value = 1
        daySpin.value = 1
        hourSpin.value = 0
        minuteSpin.value = 0
        secondSpin.value = 0
    }

    function setDateTime(dateTime) {
        yearSpin.value = dateTime.getFullYear()
        monthSpin.value = dateTime.getMonth() + 1
        daySpin.value = dateTime.getDate()
        hourSpin.value = dateTime.getHours()
        minuteSpin.value = dateTime.getMinutes()
        secondSpin.value = dateTime.getSeconds()
    }

    onAccepted: {
        dateTimeConfirmed(selectedDateTime)
    }

    onRejected: {
        console.log("选择取消")
    }
}