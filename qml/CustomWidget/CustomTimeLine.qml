// qml/CustomWidget/CustomTimeLine.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property date currentDate: new Date()
    property string displayMode: "day"

    signal meetingCreated(var meetingData)
    signal requestCreateMeeting(date selectedDate)
    signal meetingClicked(string meetingId)

    // 从全局 MeetingManager 获取真实会议数据
    function getMeetingsFromManager() {
        var meetings = []
        if (typeof meetingManager === 'undefined' || !meetingManager) return meetings

        var scheduledMeetings = meetingManager.scheduledMeetings
        if (!scheduledMeetings) return meetings

        for (var i = 0; i < scheduledMeetings.length; i++) {
            var meeting = scheduledMeetings[i]
            // 只显示未开始的会议 (NotStarted 或 status 为 0)
            if (meeting.status === "NotStarted" || meeting.status === 0) {
                var startDateTime = new Date(meeting.startTime)
                var startHour = startDateTime.getHours() + startDateTime.getMinutes() / 60
                var durationHours = meeting.durationSecs / 3600

                // 获取会议日期（只比较日期部分）
                var meetingDateOnly = new Date(meeting.startTime)
                meetingDateOnly.setHours(0, 0, 0, 0)

                // [关键修复] 日视图：只显示当前选中日期的会议
                if (displayMode === "day") {
                    var currentViewDate = new Date(currentDate)
                    currentViewDate.setHours(0, 0, 0, 0)

                    if (meetingDateOnly.toDateString() !== currentViewDate.toDateString()) {
                        // 不是当天的会议，跳过
                        continue
                    }
                }

                // 计算会议所在的天偏移（仅周视图/工作周视图需要）
                var dayOffset = 0
                if (displayMode !== "day") {
                    var viewDate = new Date(currentDate)
                    viewDate.setHours(0, 0, 0, 0)
                    dayOffset = Math.round((meetingDateOnly - viewDate) / (1000 * 3600 * 24))
                }

                meetings.push({
                    title: meeting.subject,
                    startHour: startHour,
                    duration: durationHours,
                    color: getMeetingColor(meeting),
                    dayOffset: dayOffset,
                    mid: meeting.mid,
                    description: meeting.description,
                    hostAddresses: meeting.hostAddresses,
                    status: meeting.status
                })
            }
        }
        return meetings
    }

    // 根据会议主题生成颜色
    function getMeetingColor(meeting) {
        if (!meeting.subject) return Theme.meetingItemColor

        var hash = 0
        for (var i = 0; i < meeting.subject.length; i++) {
            hash = ((hash << 5) - hash) + meeting.subject.charCodeAt(i)
            hash = hash & hash
        }
        var colors = Theme.meetingColors
        return colors[Math.abs(hash) % colors.length]
    }

    // 动态会议数据
    property var dynamicMeetings: getMeetingsFromManager()

    // 刷新会议列表
    function refreshMeetings() {
        dynamicMeetings = getMeetingsFromManager()
        console.log("[CustomTimeLine] 刷新会议列表，视图模式:", displayMode, "，会议数量:", dynamicMeetings.length)
    }

    // 格式化时间
    function formatTime(hour) {
        var h = Math.floor(hour)
        var m = Math.round((hour - h) * 60)
        return h.toString().padStart(2, '0') + ":" + m.toString().padStart(2, '0')
    }

    // 四周间距容器
    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 日期标尺
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                spacing: 0

                // 左侧空白占位
                Item {
                    Layout.preferredWidth: 50
                    Layout.fillHeight: true
                }

                // 日期列标题
                Repeater {
                    model: displayMode === "day" ? 1 : (displayMode === "workweek" ? 5 : 7)
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Label {
                            anchors.centerIn: parent
                            text: {
                                var d = new Date(currentDate);
                                var offset = index;
                                if (displayMode === "day") {
                                    offset = 0;
                                } else if (displayMode === "week") {
                                    offset = index - d.getDay();
                                } else if (displayMode === "workweek") {
                                    offset = index - (d.getDay() === 0 ? 6 : d.getDay() - 1);
                                }
                                d.setDate(d.getDate() + offset);
                                return Qt.formatDate(d, "M/d");
                            }
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: Theme.textMenu
                            font.pixelSize: 12
                            font.bold: true

                            // [新增] 日视图时显示完整日期
                            Component.onCompleted: {
                                if (displayMode === "day") {
                                    var d = new Date(currentDate);
                                    text = Qt.formatDate(d, "yyyy/MM/dd");
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onDoubleClicked: function(mouse) {
                                var d = new Date(currentDate);
                                var offset = index;
                                if (displayMode === "day") {
                                    offset = 0;
                                } else if (displayMode === "week") {
                                    offset = index - d.getDay();
                                } else if (displayMode === "workweek") {
                                    offset = index - (d.getDay() === 0 ? 6 : d.getDay() - 1);
                                }
                                d.setDate(d.getDate() + offset);
                                root.requestCreateMeeting(d);
                            }
                        }
                    }
                }
            }

            // 主体区域：左侧时间 + 右侧表格
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // 左侧时间刻度
                Column {
                    Layout.preferredWidth: 50
                    Layout.fillHeight: true
                    spacing: 0

                    Repeater {
                        model: 24
                        Item {
                            height: parent.height / 24
                            width: parent.width

                            Text {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 8
                                text: index + ":00"
                                font.pixelSize: 9
                                color: Theme.textDim
                                opacity: 0.7
                            }
                        }
                    }
                }

                // 右侧表格区域
                Rectangle {
                    id: timelineGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"
                    border.color: Theme.borderLine
                    border.width: 0.5

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: function(mouse) {
                            var colWidth = parent.width / (displayMode === "day" ? 1 : (displayMode === "workweek" ? 5 : 7));
                            var colIndex = Math.floor(mouse.x / colWidth);

                            var d = new Date(currentDate);
                            var offset = colIndex;
                            if (displayMode === "day") {
                                offset = 0;
                            } else if (displayMode === "week") {
                                offset = colIndex - d.getDay();
                            } else if (displayMode === "workweek") {
                                offset = colIndex - (d.getDay() === 0 ? 6 : d.getDay() - 1);
                            }
                            d.setDate(d.getDate() + offset);

                            root.requestCreateMeeting(d);
                        }
                    }

                    // 表格横线 - 左端出头
                    Repeater {
                        model: 24
                        Rectangle {
                            y: index * (parent.height / 24)
                            x: -8
                            width: parent.width + 8
                            height: 1
                            color: Theme.borderLine
                            opacity: 0.3
                        }
                    }

                    // 表格竖线
                    Repeater {
                        model: displayMode === "day" ? 0 : (displayMode === "workweek" ? 4 : 6)
                        Rectangle {
                            x: (index + 1) * (parent.width / (displayMode === "workweek" ? 5 : 7))
                            width: 1
                            height: parent.height
                            color: Theme.borderLine
                            opacity: 0.3
                        }
                    }

                    // 会议事件块
                    Repeater {
                        model: dynamicMeetings
                        delegate: Rectangle {
                            readonly property int totalCols: displayMode === "day" ? 1 : (displayMode === "workweek" ? 5 : 7)
                            readonly property int colIndex: {
                                if (displayMode === "day") {
                                    // [修复] 日视图下，所有会议都显示在第0列
                                    return 0;
                                }
                                var idx = modelData.dayOffset || 0;
                                if (displayMode === "workweek" && (idx < 0 || idx > 4)) return -1;
                                if (displayMode === "week" && (idx < 0 || idx > 6)) return -1;
                                return idx;
                            }

                            visible: colIndex >= 0 && modelData.startHour >= 0 && modelData.startHour < 24

                            x: colIndex * (parent.width / totalCols) + 2
                            y: modelData.startHour * (parent.height / 24)
                            width: (parent.width / totalCols) - 4
                            height: Math.max(20, modelData.duration * (parent.height / 24))

                            color: modelData.color
                            radius: 2
                            opacity: Theme.meetingItemOpacity
                            border.color: Qt.lighter(modelData.color, 1.2)
                            border.width: 1

                            Column {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 2

                                Text {
                                    text: modelData.title
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    font.bold: true
                                    wrapMode: Text.Wrap
                                    width: parent.width
                                    elide: Text.ElideRight
                                    maximumLineCount: 2
                                }

                                Text {
                                    text: formatTime(modelData.startHour) + " - " + formatTime(modelData.startHour + modelData.duration)
                                    color: Theme.textDim
                                    font.pixelSize: 9
                                    opacity: 0.8
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: function() { parent.opacity = 1.0 }
                                onExited: function() { parent.opacity = Theme.meetingItemOpacity }
                                onClicked: function() {
                                    console.log("会议点击:", modelData.title, "ID:", modelData.mid)
                                    root.meetingClicked(modelData.mid)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 监听会议管理器变化
    Connections {
        target: meetingManager
        enabled: typeof meetingManager !== 'undefined' && meetingManager !== null

        function onSignal_meetingUpdated() {
            refreshMeetings()
        }
    }

    // 监听视图模式和日期变化
    onDisplayModeChanged: {
        refreshMeetings()
    }

    onCurrentDateChanged: {
        refreshMeetings()
    }

    Component.onCompleted: {
        refreshMeetings()
        console.log("[CustomTimeLine] 初始化完成")
    }
}
