import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"


ColumnLayout {
    id: meeting_active_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // 直接使用全局注入的 meetingManager，不定义局部 property 以免遮蔽
    property bool hasActiveMeeting: false

    Timer {
        id: meetingTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (meetingManager) {
                var m = meetingManager.currentMeeting
                if (m && m.status !== 1) {
                    stop()
                    checkActiveMeeting()
                }
            }
        }
    }

    // 兜底定时器：防止信号丢失导致的状态不同步
    Timer {
        interval: 2000
        repeat: true
        running: true
        onTriggered: {
            if (meetingManager) {
                var m = meetingManager.currentMeeting
                // 如果 C++ 说在进行中，但 UI 还没显示，强制刷新
                if (m && m.status === 1 && !hasActiveMeeting) {
                    console.log("[Auto-Sync] 检测到会议进行中，刷新 UI")
                    checkActiveMeeting()
                }
            }
        }
    }

    // 获取当前会议，返回bool值结果
    function getCurrentMeeting() {
        return meetingManager ? (meetingManager.currentMeeting || {}) : {}
    }

    function checkActiveMeeting() {
        if (!meetingManager) return false
        var status = getCurrentMeeting().status
        hasActiveMeeting = (status === 1) // 1 = InProgress

        if (hasActiveMeeting && !meetingTimer.running) meetingTimer.start()
        if (!hasActiveMeeting && meetingTimer.running) meetingTimer.stop()

        return hasActiveMeeting
    }

    // 格式化日期时间
    function formatDateTime(dateTime) {
        if (!dateTime) return "N/A"
        var date = new Date(dateTime)
        return isNaN(date.getTime()) ? "N/A" : date.toLocaleString()
    }

    // 格式化计时器
    function formatElapsedTime(startTime) {
        if (!startTime) return "0分钟"
        var now = new Date()
        var start = new Date(startTime)
        if (isNaN(start.getTime())) return "0分钟"

        var diffMs = now - start
        if (diffMs < 0) return "0分钟"

        var totalSeconds = Math.floor(diffMs / 1000)
        var hours = Math.floor(totalSeconds / 3600)
        var minutes = Math.floor((totalSeconds % 3600) / 60)

        if (hours > 0) {
            return hours + "小时" + minutes + "分钟"
        }
        return minutes + "分钟"
    }

    // 获取总参会人数
    function getTotalParticipants() {
        if (meetingManager && meetingManager.getTotalParticipantCount) {
            return meetingManager.getTotalParticipantCount() || 0
        }
        return 0
    }

    // 获取签到计数
    function getCheckedInCount() {
        var meeting = getCurrentMeeting()
        var checkin = meeting.currentCheckIn || {}
        return checkin.results ? Object.keys(checkin.results).length : 0
    }

    // 获取签到百分
    function getCheckInPercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getCheckedInCount() / total * 100).toFixed(1)
    }

    // 获取投票计数
    function getVotedCount() {
        var meeting = getCurrentMeeting()
        var voting = meeting.currentVoting || {}
        return voting.results ? Object.keys(voting.results).length : 0
    }

    // 获取投票百分比
    function getVotePercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getVotedCount() / total * 100).toFixed(1)
    }

    // 获取会议关联主机（可以与其创建会议活动，不影响单独联机操控主机）
    function getFormattedHosts() {
        if (!meetingManager || !meetingManager.meetingHosts) return "未关联"
        var hosts = meetingManager.meetingHosts
        if (hosts.length === 0) return "未关联"
        return hosts.map(addr => "0x" + addr.toString(16).toUpperCase().padStart(2, '0')).join(", ")
    }

    // 打开创建签到事件面板
    function openCreateCheckinPanel() {
        var component = Qt.createComponent("CustomWidget/PanelCreateCheckin.qml")
        if (component.status === Component.Ready) {
            var panel = component.createObject(meeting_active_page, { parent: meeting_active_page })
            panel.checkinCreated.connect(function(duration) {
                if (meetingManager) meetingManager.createCheckInEvent(duration)
            })
            panel.open()
        }
    }

    // 打开创建投票事件面板
    function openCreateVotingPanel() {
        var component = Qt.createComponent("CustomWidget/PanelCreateVoting.qml")
        if (component.status === Component.Ready) {
            var panel = component.createObject(meeting_active_page, { parent: meeting_active_page })
            panel.votingCreated.connect(function(subject, duration, voteType, customOptions) {
                if (meetingManager) {
                    if (voteType === "referendum") {
                        meetingManager.createReferendumVoting(duration)
                    } else {
                        meetingManager.createCustomVoting(duration, customOptions)
                    }
                }
            })
            panel.open()
        }
    }

    Connections {
        target: meetingManager
        enabled: meetingManager !== null

        function onSignal_meetingUpdated() {
            console.log("[ActivePage] 收到更新信号")
            checkActiveMeeting()
        }
    }

    Component.onCompleted: {
        console.log("[ActivePage] 加载完成, Manager:", meetingManager !== null)
        checkActiveMeeting()
    }

    // 页面标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25
        Text {
            text: "会议控制台"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }
    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 10 }    // 占位行

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.borderLine
    }
    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 40 }    // 占位行


    // --- 无会议状态 ---
    ColumnLayout {
        //visible: !hasActiveMeeting
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 20

        Text {
            text: "当前没有进行中的会议\n请在【会议管理】页面点击开始"
            color: Theme.textDim
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignCenter
        }
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    // --- 有会议状态 ---
    ColumnLayout {
        //visible: hasActiveMeeting
        Layout.fillWidth: true
        spacing: 15

        // 1. 会议信息卡片
        Rectangle {
            Layout.fillWidth: true
            height: 240
            color: Theme.surface
            radius: Theme.radiusL
            border.color: Theme.borderLine
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Rectangle { width: 10; height: 10; radius: 5; color: Theme.success }
                    Text { text: "会议进行中"; color: Theme.success; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "结束会议"
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: 100
                        onClicked: if(meetingManager) meetingManager.endCurrentMeeting()
                        background: Rectangle { color: parent.hovered ? Theme.error : Qt.darker(Theme.error, 1.3); radius: Theme.radiusS }
                        contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 8

                    Text { text: "会议ID:"; color: Theme.textDim }
                    Text { text: getCurrentMeeting().mid || "N/A"; color: Theme.text; Layout.fillWidth: true }

                    Text { text: "会议主题:"; color: Theme.textDim }
                    Text { text: getCurrentMeeting().subject || "未命名"; color: Theme.text; Layout.fillWidth: true }

                    Text { text: "开始时间:"; color: Theme.textDim }
                    Text { text: formatDateTime(getCurrentMeeting().startTime); color: Theme.text }

                    Text { text: "已进行:"; color: Theme.textDim }
                    Text { text: formatElapsedTime(getCurrentMeeting().startTime); color: Theme.accent1 }

                    Text { text: "参会人数:"; color: Theme.textDim }
                    Text { text: getTotalParticipants() + "人"; color: Theme.text }

                    Text { text: "关联主机:"; color: Theme.textDim }
                    Text { text: getFormattedHosts(); color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight }
                }
            }
        }

        // 2. 控制按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 15
            CustomButton {
                text: "📝 发起签到"
                textColor: Theme.text
                implicitHeight: 36
                Layout.fillWidth: true

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: openCreateCheckinPanel()
            }
            CustomButton {
                text: "🗳️ 发起投票"
                textColor: Theme.text
                implicitHeight: 36
                Layout.fillWidth: true

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: openCreateVotingPanel()
            }
        }

        // 3. 当前签到状态
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: Theme.surface
            radius: Theme.radiusS
            border.color: Theme.borderLine
            visible: {
                var m = getCurrentMeeting()
                return m.currentCheckIn && m.currentCheckIn.isActive === true
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "📋 当前签到"; color: Theme.accent1; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Text { text: "进行中"; color: Theme.success }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "已签到: " + getCheckedInCount() + "/" + getTotalParticipants(); color: Theme.text }
                    Item { Layout.fillWidth: true }
                    Text { text: "签到率: " + getCheckInPercentage() + "%"; color: Theme.accent1 }
                }
            }
        }

        // 4. 当前投票状态
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: Theme.surface
            radius: Theme.radiusS
            border.color: Theme.borderLine
            visible: {
                var m = getCurrentMeeting()
                return m.currentVoting && m.currentVoting.isActive === true
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "🗳️ 当前投票"; color: Theme.accent1; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Text { text: "进行中"; color: Theme.success }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "已投票: " + getVotedCount() + "/" + getTotalParticipants(); color: Theme.text }
                    Item { Layout.fillWidth: true }
                    Text { text: "投票率: " + getVotePercentage() + "%"; color: Theme.accent1 }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

}
