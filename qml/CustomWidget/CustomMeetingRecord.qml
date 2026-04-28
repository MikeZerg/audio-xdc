// qml/CustomWidget/CustomMeetingRecord.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."

Rectangle {
    id: root
    width: parent.width
    color: Theme.panel
    radius: 2
    border.width: 1
    border.color: Theme.borderLine
    clip: true

    // ========== 属性 ==========
    property var meetingData: ({})
    property bool showStartButton: true
    property bool isExpanded: false
    property int itemIndex: 0

    // 记录每个投票的展开状态（使用对象数组）
    property var votingExpandedStates: ({})

    signal selected(var data, int index)
    signal startMeeting(var data)

    // ========== 辅助函数 ==========
    function formatDateTime(str) {
        if (!str) return "N/A"
        var d = new Date(str)
        return isNaN(d.getTime()) ? "N/A" : (d.getMonth() + 1) + "/" + d.getDate() + " " +
               String(d.getHours()).padStart(2, '0') + ":" +
               String(d.getMinutes()).padStart(2, '0')
    }

    function formatDateTimeFull(str) {
        if (!str) return "N/A"
        var d = new Date(str)
        return isNaN(d.getTime()) ? "N/A" : d.getFullYear() + "-" +
               String(d.getMonth() + 1).padStart(2, '0') + "-" +
               String(d.getDate()).padStart(2, '0') + " " +
               String(d.getHours()).padStart(2, '0') + ":" +
               String(d.getMinutes()).padStart(2, '0') + ":" +
               String(d.getSeconds()).padStart(2, '0')
    }

    function formatDuration(secs) {
        if (!secs) return "0分钟"
        var hours = Math.floor(secs / 3600)
        var minutes = Math.floor((secs % 3600) / 60)
        if (hours > 0) {
            return hours + "小时" + (minutes > 0 ? minutes + "分钟" : "")
        }
        return minutes + "分钟"
    }

    // 判断会议状态
    function isEndedMeeting() {
        var status = meetingData.status
        if (typeof status === "number") {
            return status === 2
        }
        return status === "Ended"
    }

    function isInProgressMeeting() {
        var status = meetingData.status
        if (typeof status === "number") {
            return status === 1
        }
        return status === "InProgress"
    }

    // 判断是否为表决投票
    function isReferendumVoting(voting) {
        if (!voting || !voting.type) return false
        return voting.type === "referendum"
    }

    function getOptionLabel(option) {
        var val = option.value
        if (val === 1) return "赞成"
        if (val === 0) return "弃权"
        if (val === -1) return "反对"
        return option.label || "选项" + val
    }

    function getVoteColor(value) {
        if (value === 1) return Theme.success
        if (value === 0) return Theme.warning
        if (value === -1) return Theme.error
        return Theme.accent1
    }

    // 获取投票展开状态
    function getVotingExpanded(vid) {
        if (votingExpandedStates[vid] === undefined) {
            return false
        }
        return votingExpandedStates[vid]
    }

    // 设置投票展开状态
    function setVotingExpanded(vid, expanded) {
        votingExpandedStates[vid] = expanded
        // 触发高度重新计算
        var currentHeight = height
        height = isExpanded ? expandedHeight : collapsedHeight
    }

    // 组件高度 - 增加基础高度确保内容完全显示
    property int collapsedHeight: 48
    property int expandedHeight: {
        var height = 60  // 增加基础高度

        // 会议描述
        if (meetingData.description && meetingData.description !== "") {
            height += 28
        }

        // 基本信息行
        height += 28

        // 分割线
        height += 8

        // 签到区域
        var checkins = meetingData.checkInHistory || []
        if (checkins.length > 0) {
            height += 28 + (checkins.length * 28)
        } else if (meetingData.checkInHistory !== undefined) {
            height += 28
        }

        // 投票区域
        var votings = meetingData.votingHistory || []
        if (votings.length > 0) {
            height += 28  // 投票标题
            for (var i = 0; i < votings.length; i++) {
                var voting = votings[i]
                var isReferendum = isReferendumVoting(voting)
                var votingExpanded = getVotingExpanded(voting.vid)

                if (isReferendum) {
                    // 表决投票：一行显示三个选项
                    height += 32
                } else {
                    // 自定义投票
                    if (votingExpanded) {
                        // 展开状态：标题行 + 表头 + 每个选项一行
                        height += 32  // 投票标题行
                        height += 24  // 表头
                        height += (voting.options.length * 28)  // 每个选项一行
                    } else {
                        // 折叠状态：只显示标题行
                        height += 32
                    }
                }
            }
        } else if (meetingData.votingHistory !== undefined) {
            height += 28
        }

        // 底部留白
        height += 8

        return height
    }

    height: isExpanded ? expandedHeight : collapsedHeight

    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }

    // ========== 点击选中 ==========
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: selected(meetingData, itemIndex)
    }

    // ========== 底部边框分割线（仅折叠状态显示） ==========
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.borderLine
        opacity: 0.5
        visible: !isExpanded
    }

    // ========== 折叠/展开状态布局 ==========
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ========== 标题行（始终显示） ==========
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 12

            Text {
                text: meetingData.subject || "未命名会议"
                color: Theme.text
                font.pixelSize: 12
                font.bold: true
                font.family: Theme.fontFamily
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                text: formatDateTime(meetingData.startTime)
                color: Theme.textDim
                font.pixelSize: 11
                font.family: Theme.fontFamily
                Layout.preferredWidth: 130
            }

            // 开启会议按钮
            CustomButton {
                text: isInProgressMeeting() ? "会议中" : "开启会议"
                Layout.preferredWidth: 70
                Layout.preferredHeight: 24
                backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)
                font.pixelSize: 10
                font.family: Theme.fontFamily
                visible: showStartButton
                enabled: !isInProgressMeeting()
                onClicked: startMeeting(meetingData)
            }
        }

        // ========== 展开内容 ==========
        ColumnLayout {
            Layout.fillWidth: true
            visible: isExpanded
            spacing: 0

            // 会议描述
            Text {
                text: meetingData.description || ""
                color: Theme.textDim
                font.pixelSize: 11
                font.family: Theme.fontFamily
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 12
                Layout.bottomMargin: 8
                wrapMode: Text.Wrap
                lineHeight: 1.3
                visible: meetingData.description && meetingData.description !== ""
            }

            // 基本信息行
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 12
                Layout.bottomMargin: 8
                spacing: 16

                Text {
                    text: "⏱️ " + formatDuration(meetingData.durationSecs || 0)
                    color: Theme.textDim
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                }

                Text {
                    text: "👥 " + (meetingData.expectedParticipants || 0) + "人"
                    color: Theme.textDim
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                }

                Text {
                    text: {
                        if (meetingData.hostAddresses && meetingData.hostAddresses.length > 0) {
                            var addrs = meetingData.hostAddresses.map(function(addr) {
                                return "0x" + addr.toString(16).toUpperCase()
                            })
                            return "🔗 " + (addrs.length > 2 ? addrs.slice(0, 2).join(", ") + "等" + addrs.length + "个" : addrs.join(", "))
                        }
                        return "🔗 无"
                    }
                    color: Theme.textDim
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // 分割线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.leftMargin: 24
                Layout.rightMargin: 12
                Layout.bottomMargin: 8
                color: Theme.borderLine
                opacity: 0.5
            }

            // ========== 签到记录区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                visible: meetingData.checkInHistory !== undefined
                spacing: 0

                Text {
                    text: "📋 签到"
                    color: Theme.text
                    font.pixelSize: 11
                    font.bold: true
                    font.family: Theme.fontFamily
                    Layout.leftMargin: 24
                    Layout.bottomMargin: 6
                }

                // 有签到记录
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: (meetingData.checkInHistory?.length || 0) > 0

                    Repeater {
                        model: meetingData.checkInHistory || []
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 24
                            Layout.rightMargin: 12
                            spacing: 10

                            Text {
                                text: "#" + (index + 1)
                                color: Theme.textDim
                                font.pixelSize: 10
                                font.family: Theme.fontFamily
                                Layout.preferredWidth: 25
                            }
                            Text {
                                text: formatDateTimeFull(modelData.startTime)
                                color: Theme.textDim
                                font.pixelSize: 10
                                font.family: Theme.fontFamily
                                Layout.preferredWidth: 140
                            }
                            Text {
                                text: (modelData.checkedInCount || 0) + "/" + (meetingData.expectedParticipants || 0)
                                color: Theme.success
                                font.pixelSize: 10
                                font.family: Theme.fontFamily
                                Layout.preferredWidth: 50
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 4
                                radius: 2
                                color: Theme.borderLine

                                Rectangle {
                                    width: parent.width * ((modelData.checkedInCount || 0) / (meetingData.expectedParticipants || 1))
                                    height: 4
                                    radius: 2
                                    color: Theme.success
                                }
                            }
                            Text {
                                text: Math.round(((modelData.checkedInCount || 0) / (meetingData.expectedParticipants || 1)) * 100) + "%"
                                color: Theme.accent1
                                font.pixelSize: 10
                                font.family: Theme.fontFamily
                                Layout.preferredWidth: 35
                            }
                        }
                    }
                }

                // 无签到记录提示
                Text {
                    text: "暂无签到记录"
                    color: Theme.textDim
                    font.pixelSize: 10
                    font.family: Theme.fontFamily
                    Layout.leftMargin: 24
                    Layout.bottomMargin: 6
                    visible: (meetingData.checkInHistory?.length || 0) === 0
                }
            }

            // ========== 投票记录区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                visible: meetingData.votingHistory !== undefined
                spacing: 0

                Text {
                    text: "🗳️ 投票"
                    color: Theme.text
                    font.pixelSize: 11
                    font.bold: true
                    font.family: Theme.fontFamily
                    Layout.leftMargin: 24
                    Layout.topMargin: 4
                    Layout.bottomMargin: 6
                }

                // 有投票记录
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: (meetingData.votingHistory?.length || 0) > 0

                    Repeater {
                        model: meetingData.votingHistory || []
                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            property bool isReferendum: isReferendumVoting(modelData)
                            property bool optionsExpanded: getVotingExpanded(modelData.vid)

                            onOptionsExpandedChanged: {
                                setVotingExpanded(modelData.vid, optionsExpanded)
                            }

                            // 投票标题行
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 24
                                Layout.rightMargin: 12
                                spacing: 12

                                Text {
                                    text: "#" + (index + 1)
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    Layout.preferredWidth: 25
                                }
                                Text {
                                    text: modelData.subject || "投票"
                                    color: Theme.text
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    Layout.preferredWidth: 100
                                }
                                Text {
                                    text: formatDateTime(modelData.startTime)
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    Layout.preferredWidth: 120
                                }
                                Text {
                                    text: "时长:" + formatDuration(modelData.durationSecs || 0)
                                    color: Theme.textDim
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "总票:" + (modelData.totalVotes || 0)
                                    color: Theme.warning
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    Layout.preferredWidth: 55
                                }

                                CustomButton {
                                    text: optionsExpanded ? "收起" : "详情"
                                    Layout.preferredWidth: 50
                                    Layout.preferredHeight: 24
                                    backgroundColor: Qt.darker(Theme.textMenu, 1.1)
                                    hoverColor: Theme.textMenu
                                    pressedColor: Qt.darker(Theme.textMenu, 1.3)
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    visible: !isReferendum
                                    onClicked: {
                                        optionsExpanded = !optionsExpanded
                                    }
                                }
                            }

                            // 表决投票：简洁显示
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 24
                                Layout.rightMargin: 12
                                spacing: 20
                                visible: isReferendum

                                RowLayout {
                                    spacing: 4
                                    Rectangle { width: 8; height: 8; radius: 4; color: Theme.success }
                                    Text { text: "赞成"; color: Theme.textDim; font.pixelSize: 10; font.family: Theme.fontFamily }
                                    Text {
                                        text: {
                                            var options = modelData.options || []
                                            for (var i = 0; i < options.length; i++) {
                                                if (options[i].value === 1) return options[i].count + "票"
                                            }
                                            return "0票"
                                        }
                                        color: Theme.success
                                        font.pixelSize: 10
                                        font.family: Theme.fontFamily
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Rectangle { width: 8; height: 8; radius: 4; color: Theme.warning }
                                    Text { text: "弃权"; color: Theme.textDim; font.pixelSize: 10; font.family: Theme.fontFamily }
                                    Text {
                                        text: {
                                            var options = modelData.options || []
                                            for (var i = 0; i < options.length; i++) {
                                                if (options[i].value === 0) return options[i].count + "票"
                                            }
                                            return "0票"
                                        }
                                        color: Theme.warning
                                        font.pixelSize: 10
                                        font.family: Theme.fontFamily
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Rectangle { width: 8; height: 8; radius: 4; color: Theme.error }
                                    Text { text: "反对"; color: Theme.textDim; font.pixelSize: 10; font.family: Theme.fontFamily }
                                    Text {
                                        text: {
                                            var options = modelData.options || []
                                            for (var i = 0; i < options.length; i++) {
                                                if (options[i].value === -1) return options[i].count + "票"
                                            }
                                            return "0票"
                                        }
                                        color: Theme.error
                                        font.pixelSize: 10
                                        font.family: Theme.fontFamily
                                    }
                                }
                            }

                            // 自定义投票：展开显示选项详情
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 24
                                Layout.rightMargin: 12
                                spacing: 6
                                visible: !isReferendum && optionsExpanded

                                // 表头
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    spacing: 16

                                    Text { text: "投票选项"; color: Theme.textDim; font.pixelSize: 9; font.family: Theme.fontFamily; Layout.preferredWidth: 100 }
                                    Text { text: "票数"; color: Theme.textDim; font.pixelSize: 9; font.family: Theme.fontFamily; Layout.preferredWidth: 50 }
                                    Text { text: "百分比"; color: Theme.textDim; font.pixelSize: 9; font.family: Theme.fontFamily; Layout.fillWidth: true }
                                }

                                // 选项列表
                                Repeater {
                                    model: modelData.options || []
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 16

                                        RowLayout {
                                            spacing: 6
                                            Layout.preferredWidth: 100
                                            Rectangle {
                                                width: 8
                                                height: 8
                                                radius: 4
                                                color: getVoteColor(modelData.value)
                                            }
                                            Text {
                                                text: getOptionLabel(modelData)
                                                color: Theme.text
                                                font.pixelSize: 10
                                                font.family: Theme.fontFamily
                                            }
                                        }
                                        Text {
                                            text: (modelData.count || 0) + "票"
                                            color: Theme.text
                                            font.pixelSize: 10
                                            font.family: Theme.fontFamily
                                            Layout.preferredWidth: 50
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 6
                                            radius: 3
                                            color: Theme.borderLine

                                            Rectangle {
                                                width: parent.width * ((modelData.count || 0) / (modelData.totalVotes || 1))
                                                height: 6
                                                radius: 3
                                                color: getVoteColor(modelData.value)
                                            }
                                        }
                                        Text {
                                            text: ((modelData.count || 0) / (modelData.totalVotes || 1) * 100).toFixed(1) + "%"
                                            color: Theme.accent1
                                            font.pixelSize: 9
                                            font.family: Theme.fontFamily
                                            Layout.preferredWidth: 40
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    text: "暂无投票记录"
                    color: Theme.textDim
                    font.pixelSize: 10
                    font.family: Theme.fontFamily
                    Layout.leftMargin: 24
                    Layout.bottomMargin: 6
                    visible: (meetingData.votingHistory?.length || 0) === 0
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }
}