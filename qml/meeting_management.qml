import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Audio.Hardware 1.0
import "./CustomWidget"

Rectangle {
    id: root
    color: "transparent"

    property var meetingData: ({})
    property var scheduledMeetings: []
    property var historicalMeetings: []

    function loadMeetingData() {
        // 模拟数据，实际应从 MeetingManager 获取或从数据库加载
        scheduledMeetings = [
            {
                "mid": "SCHEDULE_001",
                "subject": "2024年度产品规划会议",
                "description": "讨论年度产品路线图和关键里程碑",
                "startTime": new Date().toISOString(), // 使用当前时间方便测试
                "durationSecs": 7200,
                "expectedParticipants": 25,
                "status": "NotStarted",
                "hostAddresses": [1, 2, 3]
            },
            {
                "mid": "SCHEDULE_002",
                "subject": "技术评审会",
                "description": "Q4技术方案评审",
                "startTime": "2024-12-22T14:00:00",
                "durationSecs": 5400,
                "expectedParticipants": 12,
                "status": "NotStarted",
                "hostAddresses": [1, 2]
            }
        ]

        historicalMeetings = [
            {
                "mid": "HISTORY_001",
                "subject": "2024年度启动大会",
                "description": "年度战略宣导与团队建设",
                "startTime": "2024-01-15T09:00:00",
                "endTime": "2024-01-15T17:00:00",
                "durationSecs": 28800,
                "expectedParticipants": 50,
                "status": "Ended",
                "hostAddresses": [1, 2, 3],
                "checkInHistory": [],
                "votingHistory": []
            }
        ]

        scheduledListView.model = scheduledMeetings
        historicalListView.model = historicalMeetings
    }

    function createMeeting(meetingInfo) {
        console.log("创建会议:", JSON.stringify(meetingInfo))
        if (meetingManager) {
            meetingManager.createScheduledMeeting(
                meetingInfo.subject,
                meetingInfo.description,
                new Date(meetingInfo.startTime),
                meetingInfo.durationMinutes
            )
        }
        var newMeeting = {
            mid: "SCHEDULE_" + Date.now(),
            subject: meetingInfo.subject,
            description: meetingInfo.description,
            startTime: meetingInfo.startTime,
            durationSecs: meetingInfo.durationMinutes * 60,
            expectedParticipants: meetingInfo.expectedParticipants,
            status: "NotStarted",
            hostAddresses: meetingInfo.hostAddresses
        }
        scheduledMeetings.unshift(newMeeting)
        scheduledListView.model = scheduledMeetings
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

    Component.onCompleted: {
        loadMeetingData()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "会议管理"
                color: Theme.text
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                id: createMeetingBtn
                text: "+ 创建会议"
                implicitHeight: 32
                onClicked: {
                    var component = Qt.createComponent("CustomWidget/PanelCreateMeeting.qml")
                    if (component.status === Component.Ready) {
                        var panel = component.createObject(root, {
                            parent: root
                            // 注意：Panel 内部也应直接使用全局 meetingManager
                        })
                        panel.meetingCreated.connect(createMeeting)
                        panel.open()
                    } else {
                        console.error("组件加载失败:", component.errorString())
                    }
                }
                background: Rectangle {
                    color: createMeetingBtn.hovered ? Theme.accent : Qt.darker(Theme.accent, 1.2)
                    radius: Theme.radiusS
                }
                contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderLine
        }

        // 预定会议区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "📅 预定会议 (" + scheduledMeetings.length + ")"
                color: Theme.accent1
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.borderLine
                opacity: 0.5
            }

            ListView {
                id: scheduledListView
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(300, contentHeight)
                spacing: 8
                clip: true
                model: []

                delegate: Item {
                    width: parent.width
                    height: expanded ? 130 : 75

                    property bool expanded: false

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.surface
                        radius: Theme.radiusS
                        border.color: Theme.borderLine
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            // 可点击展开的内容区域
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: childrenRect.height

                                ColumnLayout {
                                    width: parent.width
                                    spacing: 5

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.subject
                                            color: Theme.text
                                            font.bold: true
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: formatDateTime(modelData.startTime)
                                            color: Theme.textDim
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: !expanded
                                        Text {
                                            text: modelData.description || "无描述"
                                            color: Theme.textDim
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: "时长: " + formatDuration(modelData.durationSecs)
                                            color: Theme.accent1
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: expanded
                                        Text {
                                            text: "描述: " + (modelData.description || "无")
                                            color: Theme.textDim
                                            Layout.fillWidth: true
                                            wrapMode: Text.Wrap
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: expanded
                                        Text {
                                            text: "参会人数: " + (modelData.expectedParticipants || 0) + "人"
                                            color: Theme.textDim
                                        }
                                        Text {
                                            text: "关联主机: " + (modelData.hostAddresses ? modelData.hostAddresses.join(", ") : "无")
                                            color: Theme.textDim
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: expanded = !expanded
                                }
                            }

                            // 按钮区域
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Item { Layout.fillWidth: true }

                                Button {
                                    text: expanded ? "收起" : "详情"
                                    implicitHeight: 26
                                    onClicked: expanded = !expanded
                                }

                                Button {
                                    text: "开始会议"
                                    implicitHeight: 26
                                    enabled: modelData.status === "NotStarted"
                                    onClicked: {
                                        console.log("开始会议:", modelData.mid)
                                        if (meetingManager) {
                                            // 1. 创建/更新会议信息
                                            meetingManager.createScheduledMeeting(
                                                modelData.subject,
                                                modelData.description,
                                                new Date(modelData.startTime),
                                                modelData.durationSecs / 60
                                            )
                                            // 2. 关联主机
                                            meetingManager.setMeetingHosts(modelData.hostAddresses)
                                            // 3. 启动会议 (状态变为 InProgress)
                                            meetingManager.startCurrentMeeting()

                                            // 4. 【关键】跳转到实时监控页面
                                            // 假设 meeting_active.qml 在菜单数据中的 ID 是 "F2-2" (根据你的 CustomTreeMenuData 调整)
                                            // 这里我们通过查找 parent 来调用 Main.qml 的 jumpToPage
                                            var manager = root.parent;
                                            while (manager && !manager.jumpToPage) {
                                                manager = manager.parent;
                                            }
                                            if (manager && manager.jumpToPage) {
                                                manager.jumpToPage("F3-12"); // 请替换为你实际定义的实时会议页面ID
                                            }

                                            console.log("会议已在全局 MeetingManager 中启动")
                                        }
                                    }
                                    background: Rectangle {
                                        color: parent.enabled ? Theme.success : Theme.disabled
                                        radius: Theme.radiusS
                                    }
                                    contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 历史会议区域
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Text {
                text: "📋 历史会议 (" + historicalMeetings.length + ")"
                color: Theme.textDim
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.borderLine
                opacity: 0.5
            }

            ListView {
                id: historicalListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                clip: true
                model: []

                delegate: Item {
                    width: parent.width
                    height: expanded ? 110 : 65
                    property bool expanded: false

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.surface
                        radius: Theme.radiusS
                        border.color: Theme.borderLine
                        border.width: 1
                        opacity: 0.8

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: childrenRect.height
                                ColumnLayout {
                                    width: parent.width
                                    spacing: 5
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: modelData.subject; color: Theme.text; font.bold: true; Layout.fillWidth: true }
                                        Text { text: formatDateTime(modelData.startTime); color: Theme.textDim }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: !expanded
                                        Text { text: modelData.description || "无描述"; color: Theme.textDim; Layout.fillWidth: true; elide: Text.ElideRight }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: expanded
                                        Text { text: "结束时间: " + formatDateTime(modelData.endTime); color: Theme.textDim; Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        visible: expanded
                                        Text { text: "参会人数: " + (modelData.expectedParticipants || 0) + "人"; color: Theme.textDim }
                                        Text { text: "签到次数: " + (modelData.checkInHistory ? modelData.checkInHistory.length : 0); color: Theme.textDim }
                                        Text { text: "投票次数: " + (modelData.votingHistory ? modelData.votingHistory.length : 0); color: Theme.textDim }
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: expanded = !expanded
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Item { Layout.fillWidth: true }
                                Button {
                                    text: expanded ? "收起" : "查看详情"
                                    implicitHeight: 26
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
