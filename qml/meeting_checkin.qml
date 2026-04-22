import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

Rectangle {
    id: root
    color: "transparent"

    property var meetingManager: null
    property var checkinHistory: []

    function loadCheckinHistory() {
        checkinHistory = [
            {
                meetingSubject: "2024年度启动大会",
                startTime: "2024-01-15T09:00:00",
                endTime: "2024-01-15T10:00:00",
                durationSecs: 3600,
                checkedInCount: 48,
                totalExpected: 50
            },
            {
                meetingSubject: "产品需求评审会",
                startTime: "2024-03-10T13:30:00",
                endTime: "2024-03-10T14:00:00",
                durationSecs: 1800,
                checkedInCount: 14,
                totalExpected: 15
            },
            {
                meetingSubject: "技术培训",
                startTime: "2024-05-20T14:00:00",
                endTime: "2024-05-20T14:30:00",
                durationSecs: 1800,
                checkedInCount: 18,
                totalExpected: 20
            }
        ]
        historyListView.model = checkinHistory
    }

    function formatDateTime(dateTimeStr) {
        if (!dateTimeStr) return "N/A"
        var date = new Date(dateTimeStr)
        if (isNaN(date.getTime())) return "N/A"
        return date.toLocaleString()
    }

    function formatDuration(secs) {
        if (!secs) return "0分钟"
        var minutes = Math.floor(secs / 60)
        if (minutes < 60) return minutes + "分钟"
        var hours = Math.floor(minutes / 60)
        var mins = minutes % 60
        return hours + "小时" + (mins > 0 ? mins + "分钟" : "")
    }

    function getCurrentMeeting() {
        if (meetingManager && meetingManager.currentMeetingData) {
            return meetingManager.currentMeetingData
        }
        return {}
    }

    function getCurrentCheckIn() {
        var meeting = getCurrentMeeting()
        return meeting.currentCheckIn || {}
    }

    function isCheckInActive() {
        var checkin = getCurrentCheckIn()
        return checkin.isActive === true
    }

    function getCheckedInCount() {
        var checkin = getCurrentCheckIn()
        if (checkin && checkin.results) {
            return Object.keys(checkin.results).length
        }
        return 0
    }

    function getTotalParticipants() {
        if (meetingManager && meetingManager.getTotalParticipantCount) {
            return meetingManager.getTotalParticipantCount() || 0
        }
        return 0
    }

    function getCheckInPercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getCheckedInCount() / total * 100).toFixed(1)
    }

    Component.onCompleted: {
        loadCheckinHistory()
    }

    Connections {
        target: meetingManager
        enabled: meetingManager !== null
        function onSignal_checkInEventsChanged() {}
        function onSignal_checkInResultsChanged() {}
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Text {
            text: "签到记录"
            color: Theme.text
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderLine
        }

        Rectangle {
            Layout.fillWidth: true
            height: 130
            color: Theme.surface
            radius: Theme.radiusL
            border.color: isCheckInActive() ? Theme.accent1 : Theme.borderLine
            border.width: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "📋 当前签到"
                        color: Theme.accent1
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: isCheckInActive() ? "进行中" : "未开始"
                        color: isCheckInActive() ? Theme.success : Theme.textDim
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: isCheckInActive()

                    Text {
                        text: "签到时效: " + (getCurrentCheckIn().durationSecs / 60) + "分钟"
                        color: Theme.textDim
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "开始: " + formatDateTime(getCurrentCheckIn().startTime)
                        color: Theme.textDim
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: isCheckInActive()

                    Text {
                        text: "应到人数: " + getTotalParticipants()
                        color: Theme.textDim
                    }

                    Text {
                        text: "实到人数: " + getCheckedInCount()
                        color: Theme.text
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "签到率: " + getCheckInPercentage() + "%"
                        color: Theme.accent1
                    }
                }

                Text {
                    text: "暂无进行中的签到"
                    visible: !isCheckInActive()
                    color: Theme.textDim
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Text {
                text: "📋 历史签到 (" + checkinHistory.length + ")"
                color: Theme.textDim
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.borderLine
                opacity: 0.5
            }

            ListView {
                id: historyListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                clip: true
                model: []

                delegate: Item {
                    width: parent.width

                    // 高度定义：上部分+下部分，展开时加中间部分
                    property int topHeight: 48      // 会议主题+时间行
                    property int bottomHeight: 40    // 按钮区域
                    property int middleHeight: 60    // 详情区域（展开时显示）

                    property int foldedHeight: topHeight + bottomHeight
                    property int expandedHeight: topHeight + middleHeight + bottomHeight
                    property bool expanded: false

                    height: expanded ? expandedHeight : foldedHeight

                    Behavior on height {
                        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.surface
                        radius: Theme.radiusS
                        border.color: Theme.borderLine
                        border.width: 1
                        opacity: 0.8
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 0

                            // ========= 上部：会议主题 + 时间 =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text {
                                        text: modelData.meetingSubject || "会议"
                                        color: Theme.text
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: formatDateTime(modelData.startTime)
                                        color: Theme.text
                                        font.pixelSize: 10
                                    }
                                }

                                // 折叠状态下显示的简要信息行
                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: !expanded
                                    Text {
                                        text: "签到: " + (modelData.checkedInCount || 0) + "/" + (modelData.totalExpected || 0)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: "时长: " + formatDuration(modelData.durationSecs)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            // ========= 中部：详情信息（展开时显示） =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: expanded
                                opacity: expanded ? 1 : 0

                                Behavior on opacity {
                                    NumberAnimation { duration: 200 }
                                }

                                Item { height: 4 }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "开始时间: " + formatDateTime(modelData.startTime)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "结束时间: " + formatDateTime(modelData.endTime)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "应到人数: " + (modelData.totalExpected || 0) + "人"
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: "实到人数: " + (modelData.checkedInCount || 0) + "人"
                                        color: Theme.text
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: "签到率: " + ((modelData.checkedInCount || 0) / (modelData.totalExpected || 1) * 100).toFixed(1) + "%"
                                        color: Theme.accent1
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "会议时长: " + formatDuration(modelData.durationSecs)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                            }

                            // ========= 弹性空间 =========
                            Item { Layout.fillHeight: true }

                            // ========= 下部：按钮区域 =========
                            RowLayout {
                                Layout.fillWidth: true
                                height: bottomHeight
                                spacing: 8

                                Item { Layout.fillWidth: true }

                                CustomButton {
                                    text: expanded ? "收起" : "详情"
                                    height: 24
                                    Layout.preferredWidth: 70
                                    font.pixelSize: 10
                                    onClicked: expanded = !expanded
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}