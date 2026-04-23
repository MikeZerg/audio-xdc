import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

Item {
    id: page

    property date selectedDate: new Date()
    property string viewMode: "week"
    property var meetingManager: null

    // 预计算当月第一天是周几，避免在 Repeater 中频繁计算
    readonly property int firstDayOfWeek: new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1).getDay()

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // 头部（标题，选择主视图的模式日视图，工作周视图，周视图）
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

                        backgroundColor:  "transparent"
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

        // 主体（左侧月历，右侧 “日-时间”视图
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            // 左侧：自定义简易月历
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

                    // 月份日期网格
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

                                Rectangle {
                                    anchors.centerIn: parent
                                    anchors.margins: 1
                                    radius: 13
                                    width: 26
                                    height: 26
                                    opacity: 0.6

                                    color: {
                                        if (cellDate.getMonth() !== selectedDate.getMonth()) return "transparent";
                                        if (cellDate.toDateString() === selectedDate.toDateString()) return Theme.textMenu;
                                        if (cellDate.toDateString() === new Date().toDateString()) return Theme.surfaceSelected;
                                        return "transparent";
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: cellDate.getDate()
                                        color: {
                                            if (cellDate.getMonth() !== selectedDate.getMonth()) return "#565457";
                                            if (cellDate.toDateString() === selectedDate.toDateString()) return "white";
                                            return Theme.text;
                                        }
                                        font.bold: cellDate.toDateString() === selectedDate.toDateString()
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
                        textColor: Theme.text;
                        font.pixelSize: 11
                        borderRadius: 6
                        borderWidth: 0.5
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 30
                        implicitWidth: 48
                        implicitHeight: 24
                    }

                    Item { Layout.fillHeight: true }

                }
            }

            // 右侧：时间轴 (集成 CustomTimeLine)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.panel
                border.color: Qt.darker(Theme.borderLine, 1.2)

                CustomTimeLine {
                    id: timeline
                    anchors.fill: parent
                    // 传递当前选中的日期
                    currentDate: page.selectedDate
                    // 传递当前的视图模式 (day/workweek/week)
                    displayMode: page.viewMode
                    meetingManager: page.meetingManager

                    onRequestCreateMeeting: (selectedDate) => {
                        var component = Qt.createComponent("./CustomWidget/PanelCreateMeeting.qml");
                        if (component.status === Component.Ready) {
                            var panel = component.createObject(page);
                            panel.meetingManager = page.meetingManager;
                            
                            // 使用双击选中的日期，但时间为当前系统时间
                            var now = new Date();
                            var defaultStartTime = new Date(selectedDate);
                            defaultStartTime.setHours(now.getHours(), now.getMinutes(), 0, 0);
                            panel.startTime = Qt.formatDateTime(defaultStartTime, "yyyy-MM-dd HH:mm");
                            
                            panel.meetingCreated.connect(function(meetingData) {
                                if (page.meetingManager) {
                                    page.meetingManager.createScheduledMeeting(
                                        meetingData.subject,
                                        meetingData.description,
                                        new Date(meetingData.startTime),
                                        meetingData.durationMinutes
                                    );
                                }
                                panel.destroy();
                            });
                            
                            panel.open();
                        } else if (component.status === Component.Error) {
                            console.error("创建 PanelCreateMeeting 失败:", component.errorString());
                        }
                    }

                    onMeetingCreated: (meetingData) => {
                        // 处理会议创建逻辑，例如调用 meetingManager 创建会议
                        if (page.meetingManager) {
                            page.meetingManager.createScheduledMeeting(
                                meetingData.subject,
                                meetingData.description,
                                new Date(meetingData.startTime),
                                meetingData.durationMinutes
                            );
                        }
                    }
                }
            }
        }
    }
}