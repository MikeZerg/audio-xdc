import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Audio.Log 1.0
import "./CustomWidget"


ColumnLayout {
    id: meeting_console_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    property var meetingData: ({})
    property var scheduledMeetings: []
    property var historicalMeetings: []

    function loadMeetingData() {
        scheduledMeetings = [
            {
                "mid": "SCHEDULE_001",
                "subject": "2024年度产品规划会议",
                "description": "讨论年度产品路线图和关键里程碑，以及新的设计需求",
                "startTime": new Date().toISOString(),
                "durationSecs": 5400,
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

    // 创建会议函数
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

    function formatHostAddresses(addresses) {
        if (!addresses || addresses.length === 0) return "无"
        return addresses.map(function(addr) {
            return "0x" + addr.toString(16).toUpperCase().padStart(2, '0')
        }).join(", ")
    }

    Component.onCompleted: {
        loadMeetingData()
    }

    // 页面标题
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

    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 10 }    // 占位行

    RowLayout {
        Layout.fillWidth: true
        Item { Layout.fillWidth: true; Layout.preferredHeight: 40 }
        CustomButton {
            id: createMeetingBtn
            text: "+  创建会议"
            implicitWidth: 150
            implicitHeight: 28
            Layout.rightMargin: 20

            backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
            hoverColor: Theme.textMenu
            pressedColor: Qt.darker(Theme.textMenu, 1.3)

            onClicked: {
                var component = Qt.createComponent("CustomWidget/PanelCreateMeeting.qml")
                if (component.status === Component.Ready) {
                    var panel = component.createObject(meeting_console_page)
                    panel.meetingCreated.connect(createMeeting)
                    panel.open()
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.borderLine
    }

    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 20 }    // 占位行

    ColumnLayout {
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true
        Layout.leftMargin: 25
        Layout.rightMargin: 20
        Layout.bottomMargin: 4

        // 预定会议区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "📅 预定会议 (" + scheduledMeetings.length + ")"
                color: Theme.textMenu
                font.pixelSize: 12
                font.bold: true
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
                    height: topHeight + middleHeight + bottomHeight

                    property int topHeight: 48
                    property int bottomHeight: 40
                    property int middleHeight: expanded ? 75 : 0

                    property bool expanded: false

                    Behavior on middleHeight {
                        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: Theme.surface
                        radius: Theme.radiusS
                        border.color: Theme.borderLine
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 0

                            // ========= 上部：主题 + 时间 + 描述 =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text {
                                        text: modelData.subject
                                        color: Theme.text
                                        font.pixelSize: 12
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: formatDateTime(modelData.startTime)
                                        color: Theme.text
                                        font.pixelSize: 12
                                    }
                                }

                                Text {
                                    text: modelData.description || "无描述"
                                    color: Theme.text
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                            }

                            // ========= 中部：详情信息（高度可动画） =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: expanded

                                Item { height: 0 }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "时长: " + formatDuration(modelData.durationSecs)
                                        color: Theme.textDim
                                        font.pixelSize: 12
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "参会人数: " + (modelData.expectedParticipants || 0) + "人"
                                        color: Theme.textDim
                                        font.pixelSize: 12
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "关联主机: " + formatHostAddresses(modelData.hostAddresses)
                                        color: Theme.textDim
                                        font.pixelSize: 12
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
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
                                    height: 26
                                    Layout.preferredWidth: 70
                                    font.pixelSize: 11
                                    onClicked: expanded = !expanded
                                }

                                CustomButton {
                                    text: "开始会议"
                                    height: 26
                                    font.pixelSize: 11
                                    Layout.preferredWidth: 70

                                    backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                                    hoverColor: Theme.textMenu
                                    pressedColor: Qt.darker(Theme.textMenu, 1.3)

                                    enabled: modelData.status === "NotStarted"
                                    onClicked: {
                                        console.log("开始会议:", modelData.mid)
                                        if (meetingManager) {
                                            meetingManager.createScheduledMeeting(
                                                modelData.subject,
                                                modelData.description,
                                                new Date(modelData.startTime),
                                                modelData.durationSecs / 60
                                            )
                                            meetingManager.setMeetingHosts(modelData.hostAddresses)
                                            meetingManager.startCurrentMeeting()
                                            var manager = meeting_console_page.parent
                                            while (manager && !manager.jumpToPage) {
                                                manager = manager.parent
                                            }
                                            if (manager && manager.jumpToPage) {
                                                manager.jumpToPage("F3-11")
                                            }
                                        }
                                    }
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
            Layout.topMargin: 20
            spacing: 10

            Text {
                text: "📋 历史会议 (" + historicalMeetings.length + ")"
                color: Theme.textMenu
                font.pixelSize: 12
                font.bold: true
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
                    height: topHeight + middleHeight + bottomHeight

                    property int topHeight: 48
                    property int bottomHeight: 40
                    property int middleHeight: expanded ? 60 : 0

                    property bool expanded: false

                    Behavior on middleHeight {
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

                            // ========= 上部 =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text {
                                        text: modelData.subject
                                        color: Theme.text
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: formatDateTime(modelData.startTime)
                                        color: Theme.text
                                        font.pixelSize: 11
                                    }
                                }

                                Text {
                                    text: modelData.description || "无描述"
                                    color: Theme.text
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                            }

                            // ========= 中部 =========
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: expanded

                                Item { height: 4 }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "结束时间: " + formatDateTime(modelData.endTime)
                                        color: Theme.textDim
                                        font.pixelSize: 11
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: "参会人数: " + (modelData.expectedParticipants || 0) + "人"
                                        color: Theme.textDim
                                        font.pixelSize: 11
                                    }
                                    Text {
                                        text: "签到: " + (modelData.checkInHistory ? modelData.checkInHistory.length : 0) + "次"
                                        color: Theme.textDim
                                        font.pixelSize: 11
                                    }
                                    Text {
                                        text: "投票: " + (modelData.votingHistory ? modelData.votingHistory.length : 0) + "次"
                                        color: Theme.textDim
                                        font.pixelSize: 11
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                            }

                            // ========= 弹性空间 =========
                            Item { Layout.fillHeight: true }

                            // ========= 下部 =========
                            RowLayout {
                                Layout.fillWidth: true
                                height: bottomHeight
                                spacing: 8

                                Item { Layout.fillWidth: true }

                                CustomButton {
                                    text: expanded ? "收起" : "详情"
                                    height: 26
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

    Item { Layout.preferredHeight: 15; Layout.fillWidth: true } // 占位行
}
