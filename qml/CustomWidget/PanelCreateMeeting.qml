// qml/CustomWidget/PanelCreateMeeting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
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

    signal meetingCreated(var meetingData)

    property string meetingSubject: ""
    property string meetingDescription: ""
    property string startTime: Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm")
    property int durationMinutes: 30
    property int expectedParticipants: 10
    property var selectedHosts: []

    // 获取可用的会议主机列表（Ready 状态的主机）
    function getAvailableMeetingHosts() {
        var readyHosts = []
        if (!hostManager) return readyHosts

        var connectedList = hostManager.connectedHostList
        for (var i = 0; i < connectedList.length; i++) {
            var addr = connectedList[i]
            var hostInfo = hostManager.getAllHostMap(addr)
            if (hostInfo && hostInfo.isReady === true) {
                readyHosts.push({
                    address: addr,
                    name: hostInfo.deviceTypeName || "主机 " + hostInfo.addressHex,
                    addressHex: hostInfo.addressHex || ("0x" + addr.toString(16).toUpperCase().padStart(2, '0'))
                })
            }
        }
        return readyHosts
    }

    function refreshAvailableHosts() {
        var hosts = getAvailableMeetingHosts()
        hostSelector.availableHosts = hosts
        console.log("[Panel] 刷新可用主机列表，数量:", hosts.length)
    }

    function resetForm() {
        meetingSubject = ""
        meetingDescription = ""
        startTime = Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm")
        durationMinutes = 30
        expectedParticipants = 10
        selectedHosts = []
        refreshAvailableHosts()
    }

    CustomDatetimePicker {
        id: dateTimePicker
        dialogWidth: root.width
        dialogHeight: 380
        customSpinWidth: root.width / 4
        onDateTimeConfirmed: function(selectedDate) {
            startTime = Qt.formatDateTime(selectedDate, "yyyy-MM-dd HH:mm")
        }
    }

    background: Rectangle {
        color: Theme.background
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "创建新会议"
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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "会议主题 *"; color: Theme.text }
                CustomTextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    height: 36
                    text: meetingSubject
                    onTextChanged: meetingSubject = text
                }
            }

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "会议描述"; color: Theme.text }
                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    color: Theme.background
                    border.color: Theme.borderLine
                    border.width: 1
                    radius: Theme.radiusS
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: 2
                        text: meetingDescription
                        onTextChanged: meetingDescription = text
                        color: Theme.text
                        wrapMode: Text.Wrap
                        selectByMouse: true
                    }
                }
            }

            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "开始时间 *"; color: Theme.text }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    CustomTextField {
                        id: startTimeField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        text: startTime
                        readOnly: true
                        onPressed: {
                            dateTimePicker.open()
                        }
                    }
                    CustomButton {
                        text: "选择"
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 26
                        borderWidth: 0.5
                        onClicked: {
                            dateTimePicker.open()
                        }
                    }
                }
            }

            // 预定时长
            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "预定时长 (分钟)"; color: Theme.text }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    CustomSlider {
                        id: durationSlider
                        Layout.fillWidth: true
                        from: 15
                        to: 480
                        stepSize: 15
                        value: durationMinutes
                        onIntValueChanged: {
                            durationMinutes = intValue
                            // console.log("[Panel] 预定时长变化:", durationMinutes)
                        }
                    }
                    Text {
                        text: durationMinutes + "分钟"
                        color: Theme.textMenu
                        Layout.preferredWidth: 40
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            // 参会人数
            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Text { text: "参会人数"; color: Theme.text }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    CustomSlider {
                        id: participantsSlider
                        Layout.fillWidth: true
                        from: 1
                        to: 100
                        stepSize: 1
                        value: expectedParticipants
                        onIntValueChanged: {
                            expectedParticipants = intValue
                        }
                    }
                    Text {
                        text: expectedParticipants + "人"
                        color: Theme.textMenu
                        Layout.preferredWidth: 40
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            CustomMultipleCheckBox {
                id: hostSelector
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                title: "关联主机"
                availableHosts: getAvailableMeetingHosts()
                selectedHosts: root.selectedHosts
                onSelectionChanged: function(selectedAddresses) {
                    root.selectedHosts = selectedAddresses
                    console.log("[PanelCreate] 选中的主机地址:", selectedHosts)
                }
            }
        }

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
                id: btnCreateMeeting
                text: "创建会议"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26

                backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                enabled: meetingSubject.trim() !== ""
                onClicked: {
                    if (meetingSubject.trim() === "") return;
                    if (!meetingManager) {
                        // console.error("[Panel] 致命错误：全局 meetingManager 未找到！");
                        return;
                    }

                    var startDateTime = new Date(startTime)

                    // [关键修复] 确保 selectedHosts 是数组
                    var hostsArray = []
                    if (root.selectedHosts && root.selectedHosts.length > 0) {
                        // 如果 selectedHosts 已经是数组，直接使用；否则创建新数组
                        if (Array.isArray(root.selectedHosts)) {
                            hostsArray = root.selectedHosts
                        } else {
                            hostsArray = [root.selectedHosts]
                        }
                        meetingManager.setMeetingHosts(hostsArray)
                    } else {
                        meetingManager.setMeetingHosts([])
                    }

                    // 创建计划会议
                    meetingManager.createScheduledMeeting(
                        meetingSubject,
                        meetingDescription,
                        startDateTime,
                        durationMinutes,
                        expectedParticipants
                    );
                    var success = meetingManager.saveMeetingsToJson();
                    root.close();
                }
            }
        }
    }

    Connections {
        target: hostManager
        enabled: typeof hostManager !== 'undefined' && hostManager !== null

        function onHostInfoChanged(address) {
            refreshAvailableHosts()
        }

        function onSelectedHostChanged(oldAddress, newAddress) {
            refreshAvailableHosts()
        }

        function onDetectedHostsResponseReceived(address) {
            refreshAvailableHosts()
        }
    }

    Component.onCompleted: {
        refreshAvailableHosts()
        console.log("[Panel] 创建会议面板初始化完成")
    }
}
