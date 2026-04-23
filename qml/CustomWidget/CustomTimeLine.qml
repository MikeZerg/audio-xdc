import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    
    property date currentDate: new Date()
    property string displayMode: "day"
    property var meetingManager: null

    signal meetingCreated(var meetingData)
    signal requestCreateMeeting(date selectedDate)

    // 动态会议数据生成
    function getDynamicMeetings() {
        var list = [];
        var day = currentDate.getDate();
        
        if (day % 2 !== 0) { 
            list.push({ 
                title: "产品规划会", 
                startHour: 9.5, 
                duration: 1.5, 
                color: "#4CAF50", 
                dayOffset: 0 
            });
            
            if (displayMode !== "day") {
                list.push({ 
                    title: "跨部门协作", 
                    startHour: 14, 
                    duration: 2, 
                    color: "#2196F3", 
                    dayOffset: 2 
                });
            }
        }
        return list;
    }

    property var dynamicMeetings: getDynamicMeetings()

    // 四周间距容器
    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        color: "transparent"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 日期标尺 - 无背景无边框
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
                                if (displayMode === "week") offset = index - d.getDay();
                                else if (displayMode === "workweek") offset = index - (d.getDay() === 0 ? 6 : d.getDay() - 1);
                                d.setDate(d.getDate() + offset);
                                return Qt.formatDate(d, "M/d");
                            }
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: Theme.textMenu
                            font.pixelSize: 12
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onDoubleClicked: (mouse) => {
                                var d = new Date(currentDate);
                                var offset = index;
                                if (displayMode === "week") offset = index - d.getDay();
                                else if (displayMode === "workweek") offset = index - (d.getDay() === 0 ? 6 : d.getDay() - 1);
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
                                text: (index + 1) + ":00"
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
                        onDoubleClicked: (mouse) => {
                            var colWidth = parent.width / (displayMode === "day" ? 1 : (displayMode === "workweek" ? 5 : 7));
                            var colIndex = Math.floor(mouse.x / colWidth);
                            
                            var d = new Date(currentDate);
                            var offset = colIndex;
                            if (displayMode === "week") offset = colIndex - d.getDay();
                            else if (displayMode === "workweek") offset = colIndex - (d.getDay() === 0 ? 6 : d.getDay() - 1);
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
                                var idx = modelData.dayOffset || 0;
                                if (displayMode === "workweek" && (idx < 0 || idx > 4)) return -1; 
                                if (displayMode === "week" && (idx < 0 || idx > 6)) return -1;
                                if (displayMode === "day" && idx !== 0) return -1;
                                return idx;
                            }

                            visible: colIndex >= 0

                            x: colIndex * (parent.width / totalCols) + 2
                            y: modelData.startHour * (parent.height / 24)
                            width: (parent.width / totalCols) - 4
                            height: Math.max(20, modelData.duration * (parent.height / 24))
                            
                            color: modelData.color
                            radius: 0
                            opacity: 0.5
                            border.color: Qt.lighter(modelData.color, 1.2)

                            Column {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 2
                                
                                Text {
                                    text: modelData.title
                                    color: Theme.textDim
                                    font.pixelSize: 11
                                    wrapMode: Text.Wrap
                                    width: parent.width
                                }
                                
                                Text {
                                    text: Qt.formatTime(new Date(2000, 0, 1, Math.floor(modelData.startHour), (modelData.startHour % 1) * 60)) + 
                                          " - " + 
                                          Qt.formatTime(new Date(2000, 0, 1, Math.floor(modelData.startHour + modelData.duration), ((modelData.startHeight + modelData.duration) % 1) * 60))
                                    color: Theme.textLit
                                    font.pixelSize: 9
                                    opacity: 0.9
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: () => parent.opacity = 1.0
                                onExited: () => parent.opacity = 0.85
                                onClicked: () => console.log("Meeting clicked:", modelData.title)
                            }
                        }
                    }
                }
            }
        }
    }
}