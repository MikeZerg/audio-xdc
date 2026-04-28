// qml/meeting_active.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"


ColumnLayout {
    id: meeting_active_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // ========== 接收外部传入的会议ID ==========
    property string selectedMeetingId: ""

    // 直接使用全局注入的 meetingManager
    property bool hasActiveMeeting: false
    property var currentMeetingData: ({})

    // 计时器刷新间隔
    property int elapsedSeconds: 0
    property string elapsedTimeString: "0分钟"

    Timer {
        id: meetingTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (hasActiveMeeting && currentMeetingData.startTime) {
                updateElapsedTime()
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
                checkActiveMeeting()
            }
        }
    }

    // 格式化日期时间
    function formatDateTime(dateTime) {
        if (!dateTime) return "N/A"
        var date = new Date(dateTime)
        return isNaN(date.getTime()) ? "N/A" : date.toLocaleString()
    }

    // 格式化时间显示
    function formatTimeDisplay(totalSeconds) {
        if (totalSeconds < 0) return "0分钟"
        var hours = Math.floor(totalSeconds / 3600)
        var minutes = Math.floor((totalSeconds % 3600) / 60)
        var seconds = totalSeconds % 60

        if (hours > 0) {
            return hours + "小时" + minutes + "分钟" + seconds + "秒"
        } else if (minutes > 0) {
            return minutes + "分钟" + seconds + "秒"
        }
        return seconds + "秒"
    }

    // 更新已进行时间
    function updateElapsedTime() {
        if (currentMeetingData.startTime) {
            var start = new Date(currentMeetingData.startTime)
            var now = new Date()
            if (!isNaN(start.getTime())) {
                elapsedSeconds = Math.floor((now - start) / 1000)
                elapsedTimeString = formatTimeDisplay(elapsedSeconds)
            }
        }
    }

    // 获取当前会议
    function getCurrentMeeting() {
        return currentMeetingData
    }

    // 检查是否有进行中的会议
    function checkActiveMeeting() {
        if (!meetingManager) return false

        // 先从当前活跃会议检查
        var activeMeeting = meetingManager.currentMeeting
        if (activeMeeting && (activeMeeting.status === 1 || activeMeeting.status === "InProgress")) {
            currentMeetingData = activeMeeting
            hasActiveMeeting = true
            if (!meetingTimer.running) {
                meetingTimer.start()
                updateElapsedTime()
            }
            return true
        }

        // 如果没有活跃会议，但传入了会议ID，则尝试启动会议
        if (selectedMeetingId !== "") {
            startSelectedMeeting()
            return false
        }

        hasActiveMeeting = false
        if (meetingTimer.running) meetingTimer.stop()
        return false
    }

    // 启动选中的会议
    function startSelectedMeeting() {
        if (!meetingManager || selectedMeetingId === "") return false

        console.log("[ActivePage] 启动会议 ID:", selectedMeetingId)
        var success = meetingManager.startScheduledMeeting(selectedMeetingId)

        if (success) {
            console.log("[ActivePage] 会议启动成功")
            // 清空选中的会议ID，避免重复启动
            selectedMeetingId = ""
            // 刷新当前会议数据
            currentMeetingData = meetingManager.currentMeeting
            hasActiveMeeting = true
            meetingTimer.start()
            updateElapsedTime()
            return true
        } else {
            console.error("[ActivePage] 会议启动失败")
            return false
        }
    }

    // 获取总参会人数
    function getTotalParticipants() {
        return currentMeetingData.expectedParticipants || 0
    }

    // 获取签到计数
    function getCheckedInCount() {
        var checkin = currentMeetingData.currentCheckIn || {}
        if (checkin.results) {
            return Object.keys(checkin.results).length
        }
        return checkin.checkedInCount || 0
    }

    // 获取签到百分比
    function getCheckInPercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getCheckedInCount() / total * 100).toFixed(1)
    }

    // 获取投票计数
    function getVotedCount() {
        var voting = currentMeetingData.currentVoting || {}
        if (voting.results) {
            return Object.keys(voting.results).length
        }
        return voting.totalVotes || 0
    }

    // 获取投票百分比
    function getVotePercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getVotedCount() / total * 100).toFixed(1)
    }

    // 获取会议关联主机
    function getFormattedHosts() {
        var hosts = currentMeetingData.hostAddresses || []
        if (hosts.length === 0) return "未关联"
        return hosts.map(addr => "0x" + addr.toString(16).toUpperCase().padStart(2, '0')).join(", ")
    }

    // 检查当前是否有活跃的签到
    function isCheckInActive() {
        var checkin = currentMeetingData.currentCheckIn || {}
        return checkin.isActive === true
    }

    // 检查当前是否有活跃的投票
    function isVotingActive() {
        var voting = currentMeetingData.currentVoting || {}
        return voting.isActive === true
    }

    // 打开创建签到事件面板
    function openCreateCheckinPanel() {
        var component = Qt.createComponent("CustomWidget/PanelCreateCheckin.qml")
        if (component.status === Component.Ready) {
            var panel = component.createObject(meeting_active_page, { parent: meeting_active_page })
            panel.checkinCreated.connect(function(duration) {
                if (meetingManager) {
                    meetingManager.createCheckInEvent(duration)
                    // 刷新数据
                    currentMeetingData = meetingManager.currentMeeting
                }
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
                    // 刷新数据
                    currentMeetingData = meetingManager.currentMeeting
                }
            })
            panel.open()
        }
    }

    // 结束当前会议
    function endCurrentMeeting() {
        if (meetingManager) {
            meetingManager.endCurrentMeeting()
            hasActiveMeeting = false
            currentMeetingData = {}
            if (meetingTimer.running) meetingTimer.stop()
            elapsedSeconds = 0
            elapsedTimeString = "0分钟"
        }
    }

    Connections {
        target: meetingManager
        enabled: meetingManager !== null

        function onSignal_meetingUpdated() {
            console.log("[ActivePage] 收到更新信号")
            checkActiveMeeting()
            // 更新当前会议数据
            if (hasActiveMeeting) {
                currentMeetingData = meetingManager.currentMeeting
                updateElapsedTime()
            }
        }
    }

    Component.onCompleted: {
        console.log("[ActivePage] 加载完成, Manager:", meetingManager !== null)
        checkActiveMeeting()
    }

    // ========== UI 布局 ==========

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
    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 10 }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.borderLine
    }
    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 40 }

    // --- 无会议状态 ---
    ColumnLayout {
        visible: !hasActiveMeeting
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 20

        Text {
            text: "当前没有进行中的会议\n请在【会议管理】页面点击开始会议"
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
        visible: hasActiveMeeting
        Layout.fillWidth: true
        spacing: 15

        // 1. 会议信息卡片
        Rectangle {
            Layout.fillWidth: true
            height: 280
            color: Theme.surface
            radius: Theme.radiusL
            border.color: Theme.borderLine
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Rectangle { width: 10; height: 10; radius: 5; color: Theme.success }
                    Text { text: "会议进行中"; color: Theme.success; font.bold: true; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    CustomButton {
                        text: "结束会议"
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: 100
                        backgroundColor: Theme.error
                        hoverColor: Qt.lighter(Theme.error, 1.2)
                        pressedColor: Qt.darker(Theme.error, 1.2)
                        textColor: "white"
                        font.pixelSize: 11
                        onClicked: endCurrentMeeting()
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 8

                    Text { text: "会议ID:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: currentMeetingData.mid || "N/A"; color: Theme.text; font.pixelSize: 11; Layout.fillWidth: true }

                    Text { text: "会议主题:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: currentMeetingData.subject || "未命名"; color: Theme.text; font.pixelSize: 11; Layout.fillWidth: true }

                    Text { text: "开始时间:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: formatDateTime(currentMeetingData.startTime); color: Theme.text; font.pixelSize: 11 }

                    Text { text: "已进行:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: elapsedTimeString; color: Theme.accent1; font.pixelSize: 11; font.bold: true }

                    Text { text: "预计时长:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: formatTimeDisplay(currentMeetingData.durationSecs || 0); color: Theme.text; font.pixelSize: 11 }

                    Text { text: "参会人数:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: getTotalParticipants() + "人"; color: Theme.text; font.pixelSize: 11 }

                    Text { text: "关联主机:"; color: Theme.textDim; font.pixelSize: 11 }
                    Text { text: getFormattedHosts(); color: Theme.text; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                }

                // 会议描述
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderLine
                    opacity: 0.3
                }

                Text {
                    text: "📝 " + (currentMeetingData.description || "无描述")
                    color: Theme.textDim
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    visible: currentMeetingData.description && currentMeetingData.description !== ""
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
                font.pixelSize: 12

                backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: openCreateCheckinPanel()
            }
            CustomButton {
                text: "🗳️ 发起投票"
                textColor: Theme.text
                implicitHeight: 36
                Layout.fillWidth: true
                font.pixelSize: 12

                backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: openCreateVotingPanel()
            }
        }

        // 3. 当前签到状态
        Rectangle {
            Layout.fillWidth: true
            height: 90
            color: Theme.surface
            radius: Theme.radiusS
            border.color: Theme.borderLine
            border.width: 1
            visible: isCheckInActive()

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "📋 当前签到"; color: Theme.accent1; font.bold: true; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    Text { text: "进行中"; color: Theme.success; font.pixelSize: 11 }
                    CustomButton {
                        text: "结束签到"
                        height: 24
                        width: 70
                        font.pixelSize: 10
                        backgroundColor: Theme.textDim
                        hoverColor: Qt.lighter(Theme.textDim, 1.2)
                        onClicked: {
                            if (meetingManager) meetingManager.resetCheckInEvent(0)
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "已签到: " + getCheckedInCount() + "/" + getTotalParticipants(); color: Theme.text; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 120
                        height: 6
                        radius: 3
                        color: Theme.borderLine
                        Rectangle {
                            width: parent.width * (getCheckInPercentage() / 100)
                            height: 6
                            radius: 3
                            color: Theme.success
                        }
                    }
                    Text { text: getCheckInPercentage() + "%"; color: Theme.accent1; font.pixelSize: 11; font.bold: true }
                }
            }
        }

        // 4. 当前投票状态
        Rectangle {
            Layout.fillWidth: true
            height: 90
            color: Theme.surface
            radius: Theme.radiusS
            border.color: Theme.borderLine
            border.width: 1
            visible: isVotingActive()

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "🗳️ 当前投票"; color: Theme.accent1; font.bold: true; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    Text { text: "进行中"; color: Theme.success; font.pixelSize: 11 }
                    CustomButton {
                        text: "结束投票"
                        height: 24
                        width: 70
                        font.pixelSize: 10
                        backgroundColor: Theme.textDim
                        hoverColor: Qt.lighter(Theme.textDim, 1.2)
                        onClicked: {
                            if (meetingManager) meetingManager.resetVotingEvent(0)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "已投票: " + getVotedCount() + "/" + getTotalParticipants(); color: Theme.text; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }

                    // 投票主题
                    Text {
                        text: "主题: " + (currentMeetingData.currentVoting?.subject || "投票进行中")
                        color: Theme.textDim
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        width: 100
                        height: 6
                        radius: 3
                        color: Theme.borderLine
                        Rectangle {
                            width: parent.width * (getVotePercentage() / 100)
                            height: 6
                            radius: 3
                            color: Theme.warning
                        }
                    }
                    Text { text: getVotePercentage() + "%"; color: Theme.accent1; font.pixelSize: 11; font.bold: true }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}