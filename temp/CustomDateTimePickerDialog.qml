import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
//import QtQuick.Controls.Material 2.15

Dialog {
    id: dateTimeDialog
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel       // 隐藏标准按钮
    title: "📅 选择日期和时间"
    width: 300
    height: 320
    // x: (parent.width - width) / 2
    // y: (parent.height - height) / 2

    property date selectedDateTime: new Date()
    signal dateTimeConfirmed(date selectedDate)

    ColumnLayout {
        spacing: 10
        anchors.fill: parent
        anchors.bottomMargin: 10

        // 日期选择区域
        GroupBox {
            title: "选择日期"
            font.pixelSize: 10
            Layout.fillWidth: true

            GridLayout {
                columns: 3
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                anchors.horizontalCenter: parent.horizontalCenter

                SpinBox {
                    id: yearSpin
                    from: 2020
                    to: 2045
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    value: new Date().getFullYear()
                    onValueChanged: updateDateTime()

                    textFromValue: function(value) {
                        return value.toString();
                    }
                }

                SpinBox {
                    id: monthSpin
                    from: 1
                    to: 12
                    value: new Date().getMonth() + 1
                    onValueChanged: updateDateTime()
                }

                SpinBox {
                    id: daySpin
                    from: 1
                    to: 31
                    value: new Date().getDate()
                    onValueChanged: updateDateTime()
                }
            }
        }

        // 时间选择区域
        GroupBox {
            title: "选择时间"
            font.pixelSize: 10
            Layout.fillWidth: true

            GridLayout {
                columns: 3
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                anchors.horizontalCenter: parent.horizontalCenter

                SpinBox {
                    id: hourSpin
                    from: 0
                    to: 23
                    value: new Date().getHours()
                    onValueChanged: updateDateTime()
                }

                SpinBox {
                    id: minuteSpin
                    from: 0
                    to: 59
                    value: new Date().getMinutes()
                    onValueChanged: updateDateTime()
                }

                SpinBox {
                    id: secondSpin
                    from: 0
                    to: 59
                    value: new Date().getSeconds()
                    onValueChanged: updateDateTime()
                }
            }
        }

        // 当前选择显示
        GroupBox {
            title: "当前时间"
            font.pixelSize: 10
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                Label {
                    id: selectedLabel
                    width: 140
                    //anchors.verticalCenter: parent.verticalCenter
                    text: Qt.formatDateTime(selectedDateTime, "yyyy-MM-dd hh:mm:ss")
                    font.pixelSize: 12
                    font.weight: 12
                }
                Item {
                    Layout.preferredWidth: 108
                }
                Button {
                    text: "重置"
                    font.pixelSize: 10
                    font.weight: 10
                    font.family: "微软雅黑"
                    palette.buttonText: "#1D4D66"   // 设置按钮文字颜色

                    Layout.preferredWidth: 48
                    //anchors.verticalCenter: parent.verticalCenter

                    onClicked: setCurrentTime()
                }
            }
        }
    }

    // 组件完成时设置按钮样式
    Component.onCompleted: {
        setCurrentTime();

        // 设置确定按钮样式
        var okButton = standardButton(Dialog.Ok);
        if (okButton) {
            okButton.palette.button = "#FCFCFC";
            okButton.palette.buttonText = "#1D4D66";
            okButton.font.pixelSize = 10;
            okButton.font.weight = 10;
            okButton.font.family = "微软雅黑";
            okButton.text = "确定";
            okButton.implicitWidth = 70;
            okButton.implicitHeight = 30;
        }

        // 设置取消按钮样式
        var cancelButton = standardButton(Dialog.Cancel);
        if (cancelButton) {
            cancelButton.palette.button = "#FCFCFC";
            cancelButton.palette.buttonText = "#1D4D66";
            cancelButton.font.pixelSize = 10;
            cancelButton.font.weight = 10;
            cancelButton.font.family = "微软雅黑";
            cancelButton.text = "取消";
            cancelButton.implicitWidth = 70;
            cancelButton.implicitHeight = 30;
        }
    }

    // 更新日期时间
    function updateDateTime() {
        var daysInMonth = new Date(yearSpin.value, monthSpin.value, 0).getDate();
        if (daySpin.value > daysInMonth) {
            daySpin.value = daysInMonth;
        }

        selectedDateTime = new Date(
            yearSpin.value,
            monthSpin.value - 1,
            daySpin.value,
            hourSpin.value,
            minuteSpin.value,
            secondSpin.value
        );

        selectedLabel.text = Qt.formatDateTime(selectedDateTime, "yyyy-MM-dd hh:mm:ss");
    }

    // 设置为当前时间
    function setCurrentTime() {
        var now = new Date();
        yearSpin.value = now.getFullYear();
        monthSpin.value = now.getMonth() + 1;
        daySpin.value = now.getDate();
        hourSpin.value = now.getHours();
        minuteSpin.value = now.getMinutes();
        secondSpin.value = now.getSeconds();
    }

    // 清空选择
    function clearSelection() {
        yearSpin.value = 2000;
        monthSpin.value = 1;
        daySpin.value = 1;
        hourSpin.value = 0;
        minuteSpin.value = 0;
        secondSpin.value = 0;
    }

    // 设置初始值
    function setDateTime(dateTime) {
        yearSpin.value = dateTime.getFullYear();
        monthSpin.value = dateTime.getMonth() + 1;
        daySpin.value = dateTime.getDate();
        hourSpin.value = dateTime.getHours();
        minuteSpin.value = dateTime.getMinutes();
        secondSpin.value = dateTime.getSeconds();
    }

    // 对话框按钮处理
    onAccepted: {
        dateTimeConfirmed(selectedDateTime);
    }

    onRejected: {
        console.log("选择取消");
    }
}
