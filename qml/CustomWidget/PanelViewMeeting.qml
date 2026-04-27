// qml/CustomWidget/PanelViewMeeting.qml
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

    property var meetingData: ({})

    // 安全获取会议状态
    property bool isCompleted: {
        if (!meetingData) return false
        var status = meetingData.status
        return status === "Ended" || status === 2
    }

    property bool isInProgress: {
        if (!meetingData) return false
        var status = meetingData.status
        return status === "InProgress" || status === 1
    }

    property bool isNotStarted: {
        if (!meetingData) return false
        var status = meetingData.status
        return status === "NotStarted" || status === 0
    }

    property var selectedHosts: []

    function hasCurrentCheckIn() {
        if (!meetingData) return false
        if (!meetingData.currentCheckIn) return false
        return meetingData.currentCheckIn.isActive === true
    }

    function hasCurrentVoting() {
        if (!meetingData) return false
        if (!meetingData.currentVoting) return false
        return meetingData.currentVoting.isActive === true
    }

    function hasCheckInHistory() {
        if (!meetingData) return false
        var history = meetingData.checkInHistory
        return history && Array.isArray(history) && history.length > 0
    }

    function hasVotingHistory() {
        if (!meetingData) return false
        var history = meetingData.votingHistory
        return history && Array.isArray(history) && history.length > 0
    }

    function safeArray(arr) {
        return (arr && Array.isArray(arr)) ? arr : []
    }

    // 获取可用的会议主机列表
    function getAvailableMeetingHosts() {
        var readyHosts = []
        if (!hostManager) return readyHosts

        var connectedList = hostManager.connectedHostList
        if (!connectedList) return readyHosts

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
        if (hostSelector) {
            hostSelector.availableHosts = hosts
        }
        if (meetingData && meetingData.hostAddresses) {
            selectedHosts = meetingData.hostAddresses
            if (hostSelector) {
                hostSelector.selectedHosts = selectedHosts
            }
        }
    }

    function startMeeting() {
        if (!meetingManager) {
            console.error("meetingManager 未找到")
            return
        }
        if (selectedHosts && selectedHosts.length > 0) {
            meetingManager.setMeetingHosts(selectedHosts)
        }
        meetingManager.startScheduledMeeting(meetingData.mid)
        root.close()
    }

    function formatDateTime(str) {
        if (!str) return "N/A"
        var d = new Date(str)
        if (isNaN(d.getTime())) return "N/A"

        var year = d.getFullYear()
        var month = d.getMonth() + 1
        var day = d.getDate()
        var hours = d.getHours()
        var minutes = d.getMinutes()

        return year + "-" + month + "-" + day + " " +
               hours.toString().padStart(2, '0') + ":" +
               minutes.toString().padStart(2, '0')
    }

    function formatDuration(secs) {
        if (!secs) return "0分钟"
        var m = Math.floor(secs / 60)
        return m < 60 ? m + "分钟" : Math.floor(m / 60) + "小时" + (m % 60) + "分钟"
    }

    background: Rectangle {
        color: Theme.background
    }

    // 主布局：从上到下
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 12

        // ==================== 标题栏 ====================
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: isCompleted ? "会议详情（已完成）" : (isInProgress ? "会议详情（进行中）" : "会议详情")
                font.pixelSize: 16
                font.bold: true
                color: Theme.text
                Layout.fillWidth: true
            }
            CustomButton {
                text: "✕"
                width: 28
                height: 28
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                onClicked: root.close()
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Theme.borderLine }

        // ==================== 会议基本信息 ====================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "📋 基本信息"
                color: Theme.text
                font.bold: true
                font.pixelSize: 12
            }

            // 第一行：会议主题
            RowLayout {
                Layout.fillWidth: true
                Text { text: "会议主题:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                Text { text: (meetingData && meetingData.subject) || "未命名"; color: Theme.text; Layout.fillWidth: true; font.bold: true }
            }

            // 第二行：开始时间 + 结束时间（2列）
            RowLayout {
                Layout.fillWidth: true
                // spacing: 20
                Text { text: "会议时间:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                Text {
                    text: formatDateTime(meetingData ? meetingData.startTime : null) + " - " +formatDateTime(meetingData ? meetingData.endTime : null);
                    color: Theme.text; Layout.fillWidth: true }
                // Item { Layout.fillWidth: true }
            }

            // 第三行：会议时长 + 参会人数（2列）
            RowLayout {
                Layout.fillWidth: true
                spacing: 20
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "会议时长:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    Text { text: formatDuration(meetingData ? meetingData.durationSecs : 0); color: Theme.text; Layout.fillWidth: true }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "参会人数:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    Text { text: (meetingData && meetingData.expectedParticipants) || 0; color: Theme.text; Layout.fillWidth: true }
                }
            }

            // 第四行：会议状态 + 关联主机（2列）
            RowLayout {
                Layout.fillWidth: true
                spacing: 20
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "会议状态:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    RowLayout {
                        spacing: 6
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: isCompleted ? Theme.textDim : (isInProgress ? Theme.success : Theme.warning)
                        }
                        Text {
                            text: isCompleted ? "已结束" : (isInProgress ? "进行中" : "未开始")
                            color: isCompleted ? Theme.textDim : (isInProgress ? Theme.success : Theme.warning)
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "关联主机:"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    Text {
                        text: {
                            var hosts = meetingData ? meetingData.hostAddresses : null
                            if (!hosts || hosts.length === 0) return "未关联"
                            var addrStrs = []
                            for (var i = 0; i < hosts.length; i++) {
                                addrStrs.push("0x" + hosts[i].toString(16).toUpperCase().padStart(2, '0'))
                            }
                            return addrStrs.join(", ")
                        }
                        color: Theme.text
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }

            // 会议描述
            Text {
                text: "会议描述:"
                color: Theme.textDim
                font.pixelSize: 11
                Layout.topMargin: 4
            }
            Text {
                text: (meetingData && meetingData.description) || "无描述";
                color: Theme.textDim;
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 11
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: Theme.borderLine; opacity: 0.3 }

        // ==================== 关联主机选择（仅未开始的预定会议显示）====================
        ColumnLayout {
            Layout.fillWidth: true
            visible: isNotStarted
            spacing: 6

            Text {
                text: "🔌 关联主机"
                color: Theme.text
                font.bold: true
                font.pixelSize: 12
            }

            CustomMultipleCheckBox {
                id: hostSelector
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                title: "关联主机（可重新选择）"
                availableHosts: getAvailableMeetingHosts()
                selectedHosts: (meetingData && meetingData.hostAddresses) || []
                onSelectionChanged: function(selectedAddresses) {
                    selectedHosts = selectedAddresses
                    console.log("[PanelView] 选中的主机地址:", selectedHosts)
                }
            }
        }

        Rectangle {
            height: 1
            Layout.fillWidth: true
            color: Theme.borderLine
            opacity: 0.3
            visible: (isCompleted && (hasCheckInHistory() || hasVotingHistory())) || (isInProgress && (hasCurrentCheckIn() || hasCurrentVoting()))
        }

        // ==================== 可滚动区域（签到/投票/当前事件）====================
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: parent.width
                spacing: 10

                // 签到记录（仅已完成会议且存在签到记录时显示）
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: isCompleted && hasCheckInHistory()
                    spacing: 6

                    Text {
                        text: "📋 签到记录"
                        color: Theme.success
                        font.bold: true
                        font.pixelSize: 12
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        interactive: false
                        spacing: 4
                        model: safeArray(meetingData ? meetingData.checkInHistory : null)

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 35
                            color: Qt.rgba(0,0,0,0.05)
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8

                                Text {
                                    text: "签到时间: " + formatDateTime(modelData.startTime)
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "实到: " + (modelData.checkedInCount || 0) + "人"
                                    color: Theme.success
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }
                }

                // 投票记录（仅已完成会议且存在投票记录时显示）
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: isCompleted && hasVotingHistory()
                    spacing: 6

                    Text {
                        text: "🗳️ 投票记录"
                        color: Theme.warning
                        font.bold: true
                        font.pixelSize: 12
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        interactive: false
                        spacing: 4
                        model: safeArray(meetingData ? meetingData.votingHistory : null)

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 50
                            color: Qt.rgba(0,0,0,0.05)
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "投票主题: " + (modelData.subject || "投票")
                                        color: Theme.text
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "总票数: " + (modelData.totalVotes || 0)
                                        color: Theme.warning
                                        font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: modelData.options && modelData.options.length > 0
                                    Repeater {
                                        model: modelData.options || []
                                        delegate: RowLayout {
                                            spacing: 5
                                            Rectangle {
                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: (Theme.votingColorPalette && Theme.votingColorPalette[index]) ? Theme.votingColorPalette[index] : Theme.textDim
                                            }
                                            Text {
                                                text: modelData.label + ": " + (modelData.value || 0) + "票"
                                                color: Theme.textDim
                                                font.pixelSize: 9
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 当前签到状态（进行中会议）
                Rectangle {
                    Layout.fillWidth: true
                    height: 70
                    color: Theme.surface
                    radius: 4
                    border.color: Theme.borderLine
                    visible: isInProgress && hasCurrentCheckIn()

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "📋 当前签到"; color: Theme.accent1; font.bold: true; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Text { text: "进行中"; color: Theme.success; font.pixelSize: 10 }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "已签到: " + (meetingData.currentCheckIn?.checkedInCount || 0) + "/" + (meetingData.expectedParticipants || 0); color: Theme.text; font.pixelSize: 10 }
                            Item { Layout.fillWidth: true }
                            Text { text: "签到率: " + ((meetingData.currentCheckIn?.checkedInCount || 0) / (meetingData.expectedParticipants || 1) * 100).toFixed(1) + "%"; color: Theme.accent1; font.pixelSize: 10 }
                        }
                    }
                }

                // 当前投票状态（进行中会议）
                Rectangle {
                    Layout.fillWidth: true
                    height: 70
                    color: Theme.surface
                    radius: 4
                    border.color: Theme.borderLine
                    visible: isInProgress && hasCurrentVoting()

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "🗳️ 当前投票"; color: Theme.accent1; font.bold: true; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Text { text: "进行中"; color: Theme.success; font.pixelSize: 10 }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "已投票: " + (meetingData.currentVoting?.totalVotes || 0) + "/" + (meetingData.expectedParticipants || 0); color: Theme.text; font.pixelSize: 10 }
                            Item { Layout.fillWidth: true }
                            Text { text: "投票率: " + ((meetingData.currentVoting?.totalVotes || 0) / (meetingData.expectedParticipants || 1) * 100).toFixed(1) + "%"; color: Theme.accent1; font.pixelSize: 10 }
                        }
                    }
                }

                Item { Layout.preferredHeight: 5 }
            }
        }

        // ==================== 底部按钮 ====================
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
                id: btnStartMeeting
                text: "开始会议"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26

                backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                visible: isNotStarted
                onClicked: startMeeting()
            }
        }
    }

    Component.onCompleted: {
        refreshAvailableHosts()
        console.log("[PanelViewMeeting] 初始化完成")
    }
}
