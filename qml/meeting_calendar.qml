// qml/meeting_calendar.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

Item {
    id: page

    property date selectedDate: new Date()
    property string viewMode: "week"

    readonly property int firstDayOfWeek: new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1).getDay()

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // 顶部标题 （标题及视图选项）
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: Theme.surface
            border.color: Qt.darker(Theme.borderLine, 1.2)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Label {
                    text: "会议日程"
                    font.pixelSize: 18
                    font.bold: true
                    color: Theme.text
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Repeater {
                    model: ["day", "workweek", "week"]
                    CustomButton {
                        text: modelData === "day" ? "日视图" : (modelData === "workweek" ? "工作周" : "周视图")
                        textColor: checked ? Theme.textLit : Theme.text
                        checkable: true
                        checked: viewMode === modelData
                        onClicked: viewMode = modelData

                        backgroundColor: "transparent"
                        hoverColor: Theme.textMenu
                        pressedColor: Qt.darker(Theme.textMenu, 1.3)

                        Layout.preferredWidth: 60
                        Layout.alignment: Qt.AlignVCenter
                        borderRadius: 2
                        borderWidth: 0.5
                    }
                }
            }
        }

        // 主体
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            // 左侧月历
            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                color: Theme.panel
                border.color: Qt.darker(Theme.borderLine, 1.2)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Item { Layout.fillWidth: true; Layout.preferredHeight: 25 }

                    // 月份导航
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: Qt.formatDate(selectedDate, "yyyy年 MM月")
                            font.bold: true
                            color: Theme.text
                        }
                        Item { Layout.fillWidth: true }

                        CustomButton {
                            text: "<"
                            onClicked: selectedDate = new Date(selectedDate.getFullYear(), selectedDate.getMonth() - 1, 1)
                            implicitWidth: 30
                            implicitHeight: 22
                            borderWidth: 0.5
                        }
                        CustomButton {
                            text: ">"
                            onClicked: selectedDate = new Date(selectedDate.getFullYear(), selectedDate.getMonth() + 1, 1)
                            implicitWidth: 30
                            implicitHeight: 22
                            borderWidth: 0.5
                        }
                    }
                    Item { Layout.fillWidth: true; Layout.preferredHeight: 20 }

                    // 星期头
                    RowLayout {
                        Layout.fillWidth: true
                        Repeater {
                            model: ["日", "一", "二", "三", "四", "五", "六"]
                            Label {
                                Layout.fillWidth: true
                                text: modelData
                                horizontalAlignment: Text.AlignHCenter
                                color: Theme.textDim
                                font.pixelSize: 12
                            }
                        }
                    }

                    // 日期网格
                    Grid {
                        id: calendarGrid
                        Layout.fillWidth: true
                        columns: 7
                        spacing: 2

                        Repeater {
                            model: 42
                            delegate: Item {
                                width: (calendarGrid.width - (6 * calendarGrid.spacing)) / 7
                                height: 30

                                property var cellDate: {
                                    var d = new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1);
                                    d.setDate(d.getDate() + (index - page.firstDayOfWeek));
                                    return d;
                                }

                                property bool isToday: {
                                    var today = new Date()
                                    return cellDate.toDateString() === today.toDateString()
                                }

                                property bool isSelected: cellDate.toDateString() === selectedDate.toDateString()

                                property bool hasMeeting: false

                                Component.onCompleted: {
                                    updateHasMeeting()
                                }

                                onCellDateChanged: {
                                    updateHasMeeting()
                                }

                                function updateHasMeeting() {
                                    if (!meetingManager) {
                                        hasMeeting = false
                                        return
                                    }
                                    Qt.callLater(function() {
                                        var meetings = meetingManager.scheduledMeetings
                                        if (!meetings) {
                                            hasMeeting = false
                                            return
                                        }
                                        for (var i = 0; i < meetings.length; i++) {
                                            var meeting = meetings[i]
                                            var meetingDate = new Date(meeting.startTime)
                                            if (meetingDate.toDateString() === cellDate.toDateString()) {
                                                hasMeeting = true
                                                return
                                            }
                                        }
                                        hasMeeting = false
                                    })
                                }

                                Rectangle {
                                    anchors.centerIn: parent
                                    anchors.margins: 1
                                    radius: 13
                                    width: 26
                                    height: 26
                                    opacity: 0.6

                                    color: {
                                        if (cellDate.getMonth() !== selectedDate.getMonth()) return "transparent";
                                        if (isSelected) return Theme.textMenu;
                                        if (isToday) return Theme.surfaceSelected;
                                        return "transparent";
                                    }

                                    border.width: {
                                        if (hasMeeting && !isToday && !isSelected) return 2;
                                        return 0;
                                    }
                                    border.color: Theme.textMenu

                                    Text {
                                        anchors.centerIn: parent
                                        text: cellDate.getDate()
                                        color: {
                                            if (cellDate.getMonth() !== selectedDate.getMonth()) return "#565457";
                                            if (isSelected) return "white";
                                            if (hasMeeting && !isToday && !isSelected) return Theme.textMenu;
                                            if (isToday) return Theme.text;
                                            return Theme.text;
                                        }
                                        font.bold: isSelected
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: page.selectedDate = cellDate
                                    }
                                }
                            }
                        }
                    }

                    CustomButton {
                        text: "今天"
                        onClicked: selectedDate = new Date()
                        textColor: Theme.text
                        font.pixelSize: 11
                        borderRadius: 4
                        borderWidth: 0.5
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 30
                        implicitWidth: 48
                        implicitHeight: 24
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // 右侧时间轴
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.panel
                border.color: Qt.darker(Theme.borderLine, 1.2)

                CustomTimeLine {
                    id: timeline
                    anchors.fill: parent
                    currentDate: page.selectedDate
                    displayMode: page.viewMode

                    // [关键] 处理双击创建会议
                    onRequestCreateMeeting: function(selectedDate) {
                        console.log("[meeting_calendar] 双击创建会议，日期:", selectedDate)

                        var component = Qt.createComponent("./CustomWidget/PanelCreateMeeting.qml");
                        if (component.status === Component.Ready) {
                            var panel = component.createObject(page);

                            var now = new Date();
                            var defaultStartTime = new Date(selectedDate);
                            defaultStartTime.setHours(now.getHours(), now.getMinutes(), 0, 0);
                            panel.startTime = Qt.formatDateTime(defaultStartTime, "yyyy-MM-dd HH:mm");

                            panel.meetingCreated.connect(function(meetingData) {
                                if (meetingManager) {
                                    meetingManager.createScheduledMeeting(
                                        meetingData.subject,
                                        meetingData.description,
                                        new Date(meetingData.startTime),
                                        meetingData.durationMinutes,
                                        meetingData.expectedParticipants || 0
                                    );
                                }
                                panel.destroy();
                            });

                            panel.open();
                        } else {
                            console.error("创建 PanelCreateMeeting 失败:", component.errorString());
                        }
                    }

                    // [关键] 单击查看会议详情
                    onMeetingClicked: function(meetingId) {
                            console.log("[meeting_calendar] 打开会议详情面板，ID:", meetingId)

                            // 从 MeetingManager 获取会议详细信息
                            if (!meetingManager) {
                                console.error("[meeting_calendar] meetingManager 未找到")
                                return
                            }

                            // 查找会议信息
                            var targetMeeting = null

                            // 先在预定会议中查找
                            var scheduledMeetings = meetingManager.scheduledMeetings
                            for (var i = 0; i < scheduledMeetings.length; i++) {
                                if (scheduledMeetings[i].mid === meetingId) {
                                    targetMeeting = scheduledMeetings[i]
                                    targetMeeting.isCompleted = false
                                    break
                                }
                            }

                            // 如果没找到，在历史会议中查找
                            if (!targetMeeting) {
                                var historicalMeetings = meetingManager.historicalMeetings
                                for (var ik = 0; ik < historicalMeetings.length; ik++) {
                                    if (historicalMeetings[ik].mid === meetingId) {
                                        targetMeeting = historicalMeetings[ik]
                                        targetMeeting.isCompleted = true
                                        break
                                    }
                                }
                            }

                            if (!targetMeeting) {
                                console.error("[meeting_calendar] 未找到会议，ID:", meetingId)
                                return
                            }

                            // 创建 PanelViewMeeting 弹窗
                            var component = Qt.createComponent("./CustomWidget/PanelViewMeeting.qml")
                            if (component.status === Component.Ready) {
                                var panel = component.createObject(page)
                                panel.meetingData = targetMeeting
                                panel.open()
                            } else {
                                console.error("创建 PanelViewMeeting 失败:", component.errorString())
                            }
                        }
                }
            }
        }
    }

    // 监听会议管理器变化
    Connections {
        target: meetingManager
        enabled: meetingManager !== null

        function onSignal_meetingUpdated() {
            calendarGrid.visible = false
            calendarGrid.visible = true
        }
    }

    Component.onCompleted: {
        console.log("[meeting_calendar] 初始化完成")
    }
}
