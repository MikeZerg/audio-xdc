import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

// ========== 主页面 ==========
ColumnLayout {
    id: meeting_console_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // ========== 区域展开状态（互斥） ==========
    property bool scheduledExpanded: true
    property bool historyExpanded: false

    // 当前展开的预定会议索引（同一时间只展开一个）
    property int expandedScheduledIndex: -1

    // 当前选中的历史会议索引
    property int selectedHistoryIndex: -1

    // 记录折叠状态下的固定高度
    property int headerHeight: 45

    // ========== 辅助函数 ==========
    function formatDateTime(str) {
        if (!str) return "N/A"
        var d = new Date(str)
        return isNaN(d.getTime()) ? "N/A" : d.toLocaleString()
    }

    function formatDuration(secs) {
        if (!secs) return "0分钟"
        var m = Math.floor(secs / 60)
        return m < 60 ? m + "分钟" : Math.floor(m / 60) + "小时" + (m % 60) + "分钟"
    }

    // 判断会议是否已结束（历史会议）
    function isEndedMeeting(meeting) {
        var status = meeting.status
        if (typeof status === "number") {
            return status === 2
        }
        return status === "Ended"
    }

    // 判断会议是否未开始或进行中（预定会议/进行中会议）
    function isActiveMeeting(meeting) {
        var status = meeting.status
        if (typeof status === "number") {
            return status === 0 || status === 1
        }
        return status === "NotStarted" || status === "InProgress"
    }

    // 判断会议是否进行中
    function isInProgressMeeting(meeting) {
        var status = meeting.status
        if (typeof status === "number") {
            return status === 1
        }
        return status === "InProgress"
    }

    // 获取未完结会议列表（NotStarted 和 InProgress）
    function getActiveMeetings() {
        if (!meetingManager) return []
        var allMeetings = meetingManager.scheduledMeetings
        var result = []
        for (var i = 0; i < allMeetings.length; i++) {
            if (isActiveMeeting(allMeetings[i])) {
                result.push(allMeetings[i])
            }
        }
        return result
    }

    // 获取历史会议列表（Ended）
    function getHistoricalMeetings() {
        if (!meetingManager) return []
        var allMeetings = meetingManager.historicalMeetings
        var result = []
        for (var i = 0; i < allMeetings.length; i++) {
            if (isEndedMeeting(allMeetings[i])) {
                result.push(allMeetings[i])
            }
        }
        return result
    }

    // 展开预定会议区域，折叠历史会议区域
    function expandScheduled() {
        if (!scheduledExpanded) {
            scheduledExpanded = true
            historyExpanded = false
            selectedHistoryIndex = -1
        }
    }

    // 展开历史会议区域，折叠预定会议区域
    function expandHistory() {
        if (!historyExpanded) {
            historyExpanded = true
            scheduledExpanded = false
            expandedScheduledIndex = -1
        }
    }

    // 选中预定会议（同一时间只展开一个）
    function selectScheduledMeeting(index) {
        if (expandedScheduledIndex === index) {
            expandedScheduledIndex = -1
        } else {
            expandedScheduledIndex = index
        }
    }

    // 选中历史会议
    function selectHistoryMeeting(meetingData, index) {
        if (selectedHistoryIndex === index) {
            selectedHistoryIndex = -1
        } else {
            selectedHistoryIndex = index
        }
    }

    // 开始会议（仅对未开始的会议有效）
    function onStartMeeting(meetingData) {
        if (meetingManager && !isInProgressMeeting(meetingData) && !isEndedMeeting(meetingData)) {
            console.log("[MeetingConsole] 启动会议 ID:", meetingData.mid)
            meetingManager.startScheduledMeeting(meetingData.mid)
        }
    }

    // ========== 顶部标题 ==========
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "会议记录"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredHeight: 40 }

    // ========== 主体内容区域 ==========
    Item {
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true

        Item {
            anchors.fill: parent
            anchors.leftMargin: 25
            anchors.rightMargin: 20

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                // ============================================================
                // 未完结会议区域（NotStarted 和 InProgress）
                // ============================================================
                ColumnLayout {
                    id: scheduledArea
                    Layout.fillWidth: true
                    Layout.preferredHeight: scheduledExpanded ? scheduledExpandedHeight : 60
                    spacing: 0
                    clip: true

                    Behavior on Layout.preferredHeight {
                        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4

                            Text {
                                text: "进行中的会议"
                                color: Theme.textMenu
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Text {
                                text: "(" + getActiveMeetings().length + ")"
                                color: Theme.textDim
                                font.pixelSize: 12
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: scheduledExpanded ? "点击收起 ▲" : "点击展开 ▼"
                                color: Theme.textDim
                                font.pixelSize: 11
                                opacity: 0.7
                            }
                        }

                        Button {
                            anchors.fill: parent
                            opacity: 0
                            onClicked: {
                                if (scheduledExpanded) {
                                    scheduledExpanded = false
                                    historyExpanded = false
                                } else {
                                    expandScheduled()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.borderLine
                        opacity: 0.5
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: scheduledExpanded ? true : false
                        visible: scheduledExpanded ? true : false

                        ScrollView {
                            anchors.fill: parent
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ListView {
                                id: scheduledListView
                                width: parent.width
                                implicitHeight: contentHeight
                                interactive: true
                                model: getActiveMeetings()
                                spacing: 4

                                delegate: CustomMeetingRecord {
                                    width: ListView.view.width
                                    meetingData: modelData
                                    showStartButton: true
                                    isExpanded: (expandedScheduledIndex === index)
                                    itemIndex: index

                                    onSelected: function(data, idx) {
                                        selectScheduledMeeting(idx)
                                    }

                                    onStartMeeting: function(data) {
                                        onStartMeeting(data)
                                    }
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 历史会议区域（Ended）
                // ============================================================
                ColumnLayout {
                    id: historyArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    clip: true

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4

                            Text {
                                text: "历史记录"
                                color: Theme.textMenu
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Text {
                                text: "(" + getHistoricalMeetings().length + ")"
                                color: Theme.textDim
                                font.pixelSize: 12
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: historyExpanded ? "点击收起 ▲" : "点击展开 ▼"
                                color: Theme.textDim
                                font.pixelSize: 11
                                opacity: 0.7
                            }
                        }

                        Button {
                            anchors.fill: parent
                            opacity: 0
                            onClicked: {
                                if (historyExpanded) {
                                    historyExpanded = false
                                    scheduledExpanded = false
                                } else {
                                    expandHistory()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.borderLine
                        opacity: 0.5
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: historyExpanded ? true : false
                        visible: historyExpanded ? true : false

                        ScrollView {
                            anchors.fill: parent
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ListView {
                                id: historicalListView
                                width: parent.width
                                implicitHeight: contentHeight
                                interactive: true
                                model: getHistoricalMeetings()
                                spacing: 4

                                delegate: CustomMeetingRecord {
                                    width: ListView.view.width
                                    meetingData: modelData
                                    showStartButton: false
                                    isExpanded: (selectedHistoryIndex === index)
                                    itemIndex: index

                                    onSelected: function(data, idx) {
                                        selectHistoryMeeting(data, idx)
                                    }

                                    onStartMeeting: function(data) {}
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        color: "transparent"
                        visible: !historyExpanded

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                var endedCount = getHistoricalMeetings().length
                                var activeCount = getActiveMeetings().length
                                if (activeCount > 0 || endedCount > 0) {
                                    return "共 " + activeCount + " 条未完结会议记录\n共 " + endedCount + " 条历史会议记录，点击展开查看"
                                }
                                return "暂无历史记录"
                            }
                            color: Theme.textDim
                            font.pixelSize: 10
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Item { Layout.preferredHeight: 20 }

    property int scheduledExpandedHeight: {
        var count = getActiveMeetings().length
        if (count === 0) return 100
        return headerHeight + count * 48 + 20
    }

    Connections {
        target: meetingManager
        enabled: meetingManager !== null

        function onSignal_meetingUpdated() {
            console.log("[MeetingConsole] 会议数据更新，刷新列表")
            if (scheduledListView) {
                scheduledListView.visible = false
                scheduledListView.visible = true
            }
            if (historicalListView) {
                historicalListView.visible = false
                historicalListView.visible = true
            }
        }
    }

    Component.onCompleted: {
        console.log("[MeetingConsole] 会议记录页面初始化完成")
        if (meetingManager) {
            meetingManager.loadMeetingsFromJson()
        } else {
            console.error("[MeetingConsole] 错误：全局 meetingManager 未找到！")
        }
    }
}
