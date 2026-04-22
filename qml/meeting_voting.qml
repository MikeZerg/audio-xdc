import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"


Rectangle {
    id: root
    color: "transparent"

    property var meetingManager: null
    property var votingHistory: []

    function loadVotingHistory() {
        votingHistory = [
            {
                meetingSubject: "2024年度启动大会",
                subject: "年度工作目标投票",
                startTime: "2024-01-15T14:00:00",
                endTime: "2024-01-15T14:30:00",
                durationSecs: 1800,
                options: [
                    {label: "赞成", value: 1, count: 35, percentage: 72.9},
                    {label: "弃权", value: 0, count: 8, percentage: 16.7},
                    {label: "反对", value: -1, count: 5, percentage: 10.4}
                ],
                totalVotes: 48
            },
            {
                meetingSubject: "产品需求评审会",
                subject: "功能优先级投票",
                startTime: "2024-03-10T15:00:00",
                endTime: "2024-03-10T15:20:00",
                durationSecs: 1200,
                options: [
                    {label: "功能A", value: 2, count: 5, percentage: 35.7},
                    {label: "功能B", value: 3, count: 6, percentage: 42.9},
                    {label: "功能C", value: 4, count: 3, percentage: 21.4}
                ],
                totalVotes: 14
            }
        ]
        historyListView.model = votingHistory
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

    function getCurrentVoting() {
        var meeting = getCurrentMeeting()
        return meeting.currentVoting || {}
    }

    function isVotingActive() {
        var voting = getCurrentVoting()
        return voting.isActive === true
    }

    function getVotedCount() {
        var voting = getCurrentVoting()
        if (voting && voting.results) {
            return Object.keys(voting.results).length
        }
        return 0
    }

    function getTotalParticipants() {
        if (meetingManager && meetingManager.getTotalParticipantCount) {
            return meetingManager.getTotalParticipantCount() || 0
        }
        return 0
    }

    function getVotePercentage() {
        var total = getTotalParticipants()
        if (total === 0) return 0
        return (getVotedCount() / total * 100).toFixed(1)
    }

    // 使用 Theme 中的统一颜色获取函数
    function getOptionColor(value) {
        // 表决投票：value 为 -1, 0, 1
        if (value === 1) return Theme.votingColorPalette[0]   // 赞成
        if (value === 0) return Theme.votingColorPalette[1]   // 弃权
        if (value === -1) return Theme.votingColorPalette[2]  // 反对

        // 自定义投票：value >= 2 或 value <= -2，从索引3开始
        var index = (Math.abs(value) - 2) + 3
        // 超出长度时循环使用
        index = index % Theme.votingColorPalette.length
        return Theme.votingColorPalette[index]
    }

    // 获取得票最高的选项
    function getTopOption(options) {
        if (!options || options.length === 0) return null
        var top = options[0]
        for (var i = 1; i < options.length; i++) {
            if (options[i].count > top.count) {
                top = options[i]
            }
        }
        return top
    }

    Component.onCompleted: {
        loadVotingHistory()
    }

    Connections {
        target: meetingManager
        enabled: meetingManager !== null
        function onSignal_votingEventsChanged() {}
        function onSignal_votingResultsChanged() {}
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Text {
            text: "投票记录"
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
            height: 150
            color: Theme.surface
            radius: Theme.radiusL
            border.color: isVotingActive() ? Theme.accent1 : Theme.borderLine
            border.width: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "🗳️ 当前投票"
                        color: Theme.accent1
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: isVotingActive() ? "进行中" : "未开始"
                        color: isVotingActive() ? Theme.success : Theme.textDim
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: isVotingActive()

                    Text {
                        text: "投票主题: " + (getCurrentVoting().subject || "会议投票")
                        color: Theme.text
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: "时效: " + (getCurrentVoting().durationSecs / 60) + "分钟"
                        color: Theme.textDim
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: isVotingActive()

                    Text {
                        text: "总人数: " + getTotalParticipants()
                        color: Theme.textDim
                    }

                    Text {
                        text: "已投票: " + getVotedCount()
                        color: Theme.text
                    }

                    Text {
                        text: "投票率: " + getVotePercentage() + "%"
                        color: Theme.accent1
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 15
                    visible: isVotingActive()

                    Repeater {
                        model: getCurrentVoting().options || []

                        delegate: Row {
                            spacing: 5

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: getOptionColor(modelData.value)
                            }

                            Text {
                                text: modelData.label + ": " +
                                      (function() {
                                          var results = getCurrentVoting().results || {}
                                          var count = 0
                                          for (var key in results) {
                                              if (results[key] && results[key].option === modelData.value) count++
                                          }
                                          return count
                                      })() + "票"
                                color: Theme.textDim
                            }
                        }
                    }
                }

                Text {
                    text: "暂无进行中的投票"
                    visible: !isVotingActive()
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
                text: "📋 历史投票 (" + votingHistory.length + ")"
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

                    property int topHeight: 48
                    property int bottomHeight: 40
                    property int middleHeight: 100

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

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: !expanded
                                    Text {
                                        text: "主题: " + (modelData.subject || "投票")
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Loader {
                                        active: getTopOption(modelData.options) !== null
                                        sourceComponent: topOptionComp
                                        property var topOption: getTopOption(modelData.options)
                                    }
                                }
                            }

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
                                        text: "投票时长: " + formatDuration(modelData.durationSecs)
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                    Text {
                                        text: "总票数: " + (modelData.totalVotes || 0) + "票"
                                        color: Theme.text
                                        font.pixelSize: 10
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                Text {
                                    text: "投票选项:"
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 12
                                    Repeater {
                                        model: modelData.options || []
                                        delegate: Row {
                                            spacing: 4
                                            Rectangle {
                                                width: 10
                                                height: 10
                                                radius: 5
                                                color: getOptionColor(modelData.value)
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            Text {
                                                text: modelData.label + " (" + (modelData.count || 0) + "票/" + (modelData.percentage || 0) + "%)"
                                                color: Theme.text
                                                font.pixelSize: 10
                                            }
                                        }
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }

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

    Component {
        id: topOptionComp
        Row {
            spacing: 4
            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: getOptionColor(topOption.value)
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: topOption.label + " (" + topOption.count + "票/" + topOption.percentage + "%)"
                color: Theme.accent1
                font.pixelSize: 10
            }
        }
    }
}
