import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

ColumnLayout {
    id: root
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // 接收外部传入的会议ID（用于从日程跳转）
    property string selectedMeetingId: ""

    // 当前选中的会议数据
    property var selectedMeeting: ({})

    // 顶部区域展开状态
    property bool topExpanded: true

    // 辅助函数：格式化时间
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

    // 根据ID加载会议信息
    function loadMeetingById(meetingId) {
        if (!meetingManager) return false

        // 先在预定会议中查找
        var scheduledMeetings = meetingManager.scheduledMeetings
        for (var i = 0; i < scheduledMeetings.length; i++) {
            if (scheduledMeetings[i].mid === meetingId) {
                selectedMeeting = scheduledMeetings[i]
                selectedMeeting.isCompleted = false
                console.log("[Console] 加载预定会议成功:", selectedMeeting.subject)
                // [修复] 强制刷新顶部显示
                topArea.visible = true
                return true
            }
        }

        // 在历史会议中查找
        var historicalMeetings = meetingManager.historicalMeetings
        for (var it = 0; it < historicalMeetings.length; it++) {
            if (historicalMeetings[it].mid === meetingId) {
                selectedMeeting = historicalMeetings[it]
                selectedMeeting.isCompleted = true
                console.log("[Console] 加载历史会议成功:", selectedMeeting.subject)
                topArea.visible = true
                return true
            }
        }

        console.warn("[Console] 未找到会议，ID:", meetingId)
        return false
    }

    // 开始会议
    function startMeeting(meetingId) {
        if (meetingManager) {
            meetingManager.startScheduledMeeting(meetingId)
            loadMeetingById(meetingId)
        }
    }

    Component.onCompleted: {
        console.log("[Console] 会议模块入口加载，正在同步数据...");
        if (meetingManager) {
            meetingManager.loadMeetingsFromJson();

            if (selectedMeetingId !== "") {
                Qt.callLater(function() {
                    loadMeetingById(selectedMeetingId)
                })
            }
        } else {
            console.error("[Console] 致命错误：全局 meetingManager 未找到！");
        }
    }

    // 顶部标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "检测在线主机及主机属性"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredHeight: 40 }


    // ==================== 第一部分：当前选中会议详情 ====================
    Rectangle {
        id: topArea
        Layout.fillWidth: true
        Layout.preferredHeight: 200 //selectedMeetingId !== "" ? (topExpanded ? 280 : 80) : 0
        color: Theme.surface
        radius: Theme.radiusL
        border.color: Theme.borderLine
        border.width: 1
        //visible: selectedMeetingId !== ""

        Behavior on height { NumberAnimation { duration: 200 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: selectedMeeting.isCompleted === true ? Theme.textDim : Theme.success
                }
                Text {
                    text: selectedMeeting.isCompleted === true ? "已完成会议" : "预定会议"
                    color: selectedMeeting.isCompleted === true ? Theme.textDim : Theme.success
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                CustomButton {
                    text: topExpanded ? "收起 ▲" : "展开 ▼"
                    height: 24
                    width: 60
                    font.pixelSize: 10
                    backgroundColor: "transparent"
                    hoverColor: Theme.surfaceSelected
                    onClicked: topExpanded = !topExpanded
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: selectedMeeting.subject || "未命名"
                    font.bold: true
                    color: Theme.text
                    Layout.fillWidth: true
                }
                Text {
                    text: formatDateTime(selectedMeeting.startTime)
                    color: Theme.textDim
                    font.pixelSize: 11
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: topExpanded
                spacing: 5

                Text {
                    text: "描述: " + (selectedMeeting.description || "无描述");
                    color: Theme.textDim;
                    wrapMode: Text.Wrap;
                    Layout.fillWidth: true
                }
                Text {
                    text: "关联主机: " + JSON.stringify(selectedMeeting.hostAddresses || []);
                    color: Theme.textDim;
                    font.pixelSize: 11
                }
                Text {
                    text: "参会人数: " + (selectedMeeting.expectedParticipants || 0);
                    color: Theme.textDim;
                    font.pixelSize: 11
                }
                Text {
                    text: "会议时长: " + formatDuration(selectedMeeting.durationSecs || 0);
                    color: Theme.textDim;
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: !selectedMeeting.isCompleted && selectedMeeting.status !== 1

                    CustomButton {
                        text: "▶ 开始会议"
                        height: 28
                        Layout.preferredWidth: 100
                        backgroundColor: Theme.success
                        textColor: "white"
                        onClicked: {
                            if (selectedMeeting.mid) {
                                startMeeting(selectedMeeting.mid)
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // 已完成会议的签到/投票记录
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: selectedMeeting.isCompleted === true
                    spacing: 8

                    Text {
                        text: "📌 会议事件记录:";
                        color: Theme.accent1;
                        font.pixelSize: 11;
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: checkinEventList.contentHeight + 10
                        color: Qt.rgba(0,0,0,0.05)
                        radius: 4
                        visible: (selectedMeeting.checkInHistory || []).length > 0

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "📋 签到记录"; color: Theme.text; font.pixelSize: 10; font.bold: true }

                            ListView {
                                id: checkinEventList
                                Layout.fillWidth: true
                                height: contentHeight
                                interactive: false
                                spacing: 4
                                model: selectedMeeting.checkInHistory || []

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 30
                                    color: "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        Text { text: "签到时间: " + formatDateTime(modelData.startTime); color: Theme.textDim; font.pixelSize: 10 }
                                        Item { Layout.fillWidth: true }
                                        Text { text: "实到: " + (modelData.checkedInCount || 0) + "人"; color: Theme.success; font.pixelSize: 10 }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: votingEventList.contentHeight + 10
                        color: Qt.rgba(0,0,0,0.05)
                        radius: 4
                        visible: (selectedMeeting.votingHistory || []).length > 0

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "🗳️ 投票记录"; color: Theme.text; font.pixelSize: 10; font.bold: true }

                            ListView {
                                id: votingEventList
                                Layout.fillWidth: true
                                height: contentHeight
                                interactive: false
                                spacing: 4
                                model: selectedMeeting.votingHistory || []

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 30
                                    color: "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        Text { text: "投票主题: " + (modelData.subject || "投票"); color: Theme.textDim; font.pixelSize: 10; Layout.fillWidth: true }
                                        Text { text: "总票数: " + (modelData.totalVotes || 0); color: Theme.warning; font.pixelSize: 10 }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ==================== 第二部分：预定会议列表 ====================
    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: scheduledListView.contentHeight + 40
        spacing: 10

        Text {
            text: "📅 预定会议"
            color: Theme.textMenu
            font.bold: true
        }

        ListView {
            id: scheduledListView
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            interactive: false
            model: meetingManager ? meetingManager.scheduledMeetings : []
            spacing: 8

            delegate: Rectangle {
                width: ListView.view.width
                height: listExpanded ? (80 + detailCol.height + 20) : 60
                color: Theme.surface
                radius: Theme.radiusS
                border.color: Theme.borderLine
                border.width: 0.5
                clip: true

                property bool listExpanded: false
                Behavior on height { NumberAnimation { duration: 200 } }

                ColumnLayout {
                    id: detailCol
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.subject;
                            font.bold: true;
                            color: Theme.text;
                            elide: Text.ElideRight;
                            Layout.fillWidth: true
                        }
                        Text {
                            text: formatDateTime(modelData.startTime);
                            color: Theme.textDim;
                            font.pixelSize: 11
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: parent.parent.listExpanded
                        spacing: 5

                        Text {
                            text: modelData.description || "无描述";
                            color: Theme.textDim;
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "关联主机: " + JSON.stringify(modelData.hostAddresses || []);
                            color: Theme.textDim;
                            font.pixelSize: 11
                        }
                        Text {
                            text: "参会人数: " + (modelData.expectedParticipants || 0);
                            color: Theme.textDim;
                            font.pixelSize: 11
                        }

                        RowLayout {
                            CustomButton {
                                text: modelData.status === 1 ? "会议进行中" : "▶ 开始会议"
                                height: 28
                                enabled: modelData.status !== 1
                                backgroundColor: modelData.status === 1 ? Theme.textDim : Theme.success
                                textColor: "white"
                                onClicked: {
                                    if (meetingManager && modelData.status !== 1) {
                                        console.log("[Console] 启动会议 ID:", modelData.mid);
                                        meetingManager.startScheduledMeeting(modelData.mid);
                                    }
                                }
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }

                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        CustomButton {
                            text: parent.parent.listExpanded ? "收起" : "详情"
                            height: 22
                            font.pixelSize: 10
                            onClicked: parent.parent.listExpanded = !parent.parent.listExpanded
                        }
                    }
                }
            }
        }
    }

    // ==================== 第三部分：历史会议列表 ====================
    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: historicalListView.contentHeight + 40
        spacing: 10

        Text {
            text: "📋 历史会议归档"
            color: Theme.textMenu
            font.bold: true
        }

        ListView {
            id: historicalListView
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            interactive: false
            model: meetingManager ? meetingManager.historicalMeetings : []
            spacing: 8

            delegate: Rectangle {
                width: ListView.view.width
                height: historyExpanded ? (70 + eventList.contentHeight + 20) : 60
                color: Theme.panel
                radius: Theme.radiusS
                border.color: Theme.borderLine
                border.width: 0.5
                clip: true

                property bool historyExpanded: false
                Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData.subject;
                            font.bold: true;
                            color: Theme.text;
                            Layout.fillWidth: true
                        }
                        Text {
                            text: formatDateTime(modelData.startTime);
                            color: Theme.textDim;
                            font.pixelSize: 11
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: parent.parent.historyExpanded
                        spacing: 10

                        Text {
                            text: "描述: " + (modelData.description || "");
                            color: Theme.textDim;
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "参会人数: " + (modelData.expectedParticipants || 0);
                            color: Theme.textDim;
                            font.pixelSize: 11
                        }
                        Text {
                            text: "📌 会议事件记录:";
                            color: Theme.accent1;
                            font.pixelSize: 11;
                            font.bold: true
                        }

                        ListView {
                            id: eventList
                            Layout.fillWidth: true
                            Layout.preferredHeight: contentHeight
                            interactive: false
                            spacing: 5
                            model: [
                                ...(modelData.checkInHistory || []),
                                ...(modelData.votingHistory || [])
                            ]

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 40
                                color: Qt.rgba(0,0,0,0.1)
                                radius: 2

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 5

                                    Text {
                                        text: modelData.options ? "🗳️ 投票" : "📋 签到"
                                        font.bold: true
                                        color: modelData.options ? Theme.warning : Theme.success
                                    }

                                    Text {
                                        text: modelData.subject || (modelData.options ? "自定义投票" : "常规签到")
                                        color: Theme.text
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: modelData.options ?
                                              "总票数: " + modelData.totalVotes :
                                              "实到: " + modelData.checkedInCount
                                        color: Theme.textDim
                                        font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        CustomButton {
                            text: parent.parent.historyExpanded ? "收起" : "详情"
                            height: 22
                            font.pixelSize: 10
                            onClicked: parent.parent.historyExpanded = !parent.parent.historyExpanded
                        }
                    }
                }
            }
        }
    }

    // 底部占位空间
    Item { Layout.preferredHeight: 20 }



    // 监听会议管理器变化
    Connections {
        target: meetingManager
        enabled: meetingManager !== null

        function onSignal_meetingUpdated() {
            console.log("[Console] 会议数据更新，刷新列表")
            if (selectedMeetingId !== "") {
                loadMeetingById(selectedMeetingId)
            }
            // 刷新 ListView
            scheduledListView.visible = false
            scheduledListView.visible = true
            historicalListView.visible = false
            historicalListView.visible = true
        }
    }
}
