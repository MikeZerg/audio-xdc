import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Audio.Log 1.0
import "./CustomWidget"


ColumnLayout {
    id: app_logs_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // 顶部标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "系统日志"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredHeight: 30 }

    // 添加一层ColumnLayout， 可设置边距等布局
    ColumnLayout {
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true
        Layout.leftMargin: 25
        Layout.rightMargin: 20
        Layout.bottomMargin: 4

        RowLayout {
            id: setLogMode
            Layout.preferredHeight: 40
            Layout.fillWidth: true
            spacing: 20

            Text {
                text: "日志模式:"
                font.family: Theme.fontFamily
                font.pixelSize: 14
                color: Theme.text
                verticalAlignment: Text.AlignVCenter
            }

            CustomRadioButton {
                id: normalModeBtn
                text: "日常模式 (屏蔽数据流)"
                checked: logManager.currentLogMode() === LogManager.NormalMode
                font.family: Theme.fontFamily
                font.pixelSize: 12

                onCheckedChanged: {
                    if (checked) {
                        logManager.setLogMode(LogManager.NormalMode);
                    }
                }
            }

            CustomRadioButton {
                id: debugModeBtn
                text: "调试模式 (记录收发数据)"
                checked: logManager.currentLogMode() === LogManager.DebugMode
                font.family: Theme.fontFamily
                font.pixelSize: 12

                onCheckedChanged: {
                    if (checked) {
                        logManager.setLogMode(LogManager.DebugMode);
                    }
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: parent.height }

            // 【新增】关键字过滤输入框
            Rectangle {
                Layout.preferredWidth: 200
                Layout.preferredHeight: 26
                color: Theme.background
                border.color: Theme.borderLine
                radius: Theme.radiusS

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    Text { text: "🔍"; color: Theme.textDim }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextInput {
                            id: filterInput
                            anchors.fill: parent
                            text: logManager.filterKeywords.join(", ")
                            color: Theme.text
                            font.pixelSize: 11

                            onTextChanged: {
                                var keywords = text.split(",").map(s => s.trim()).filter(s => s.length > 0);
                                logManager.setFilterKeywords(keywords);
                            }
                        }

                        // 模拟 placeholderText：放在 TextInput 同级的 Item 中
                        Text {
                            text: "输入关键词，逗号分隔..."
                            color: Theme.textDim
                            font.pixelSize: 11
                            visible: filterInput.text.length === 0
                            anchors.left: parent.left
                            anchors.leftMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: 0.7
                        }
                    }

                    MouseArea {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        visible: filterInput.text.length > 0
                        cursorShape: Qt.PointingHandCursor

                        Text {
                            text: "✕"
                            color: Theme.textDim
                            anchors.centerIn: parent
                        }

                        onClicked: filterInput.text = ""
                    }
                }
            }

            CustomButton {
                id: testLogButton
                text: "添加测试日志"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSize12
                Layout.preferredWidth: 100
                Layout.preferredHeight: 28
                visible: false

                palette.buttonText: Theme.accent1

                onClicked: {
                    logManager.addLog("这是一条测试日志记录", LogLevel.INFO)
                    logManager.addLog("这是一条警告测试", LogLevel.WARNING)
                    logManager.addLog("这是一条错误测试", LogLevel.ERROR)
                    testLogTimer.restart()
                }

                Timer {
                    id: testLogTimer
                    interval: 220
                    onTriggered: testLogButton.enabled = true
                }
            }

            CustomButton {
                id: clearLogButton
                text: "清除日志记录"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSize12
                Layout.preferredWidth: 100
                Layout.preferredHeight: 26

                palette.buttonText: Theme.error

                onClicked: {
                    logManager.clearLogs()
                    clearLogTimer.restart()
                }

                Timer {
                    id: clearLogTimer
                    interval: 220
                    onTriggered: clearLogButton.enabled = true
                }
            }
        }

        ListView {
            id: logView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: logManager.logs
            spacing: 0 // 确保 ListView 内部项之间没有额外间距

            delegate: TextArea {
                text: modelData
                font.family: Theme.fontFamily
                font.pixelSize: 11

                // 【关键修改】根据过滤关键词决定是否显示
                visible: {
                    if (!logManager.filterKeywords || logManager.filterKeywords.length === 0) return true;
                    // 只要包含任意一个关键词就显示
                    for (var i = 0; i < logManager.filterKeywords.length; i++) {
                        if (text.toLowerCase().includes(logManager.filterKeywords[i].toLowerCase())) {
                            return true;
                        }
                    }
                    return false;
                }

                color: {
                    if (text.includes("[ERR]")) return Theme.error
                    if (text.includes("[WARN]")) return Theme.warning
                    if (text.includes("[INFO]")) return Theme.success
                    if (text.includes("[RX]")) return "#00BFFF"
                    if (text.includes("[TX]")) return "#32CD32"
                    return Theme.text
                }

                // 【关键修改1】支持选中和拷贝
                readOnly: true
                selectByMouse: true
                wrapMode: Text.Wrap

                // 【关键修改2】缩小行间距
                height: visible ? 20 : 0    // 设置每行高度，根据字体大小调整，不显示时高度为0
                verticalAlignment: Text.AlignVCenter
                padding: 2
                topPadding: 4

                // 隐藏 TextArea 默认的背景和光标，使其看起来像普通文本
                background: Rectangle { color: "transparent" }
                cursorVisible: false    // 隐藏光标
                selectionColor: "#267CD8" // 使用主题色作为选中背景
                selectedTextColor: "white"    // 选中时的文字颜色

                width: ListView.view.width
            }

            onCountChanged: positionViewAtEnd()

            // 滚动条设置
            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                policy: ScrollBar.AsNeeded
                interactive: true
                z: 2
            }

            clip: true

            // ListView背景设置
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: Theme.borderLine
                border.width: 1
                radius: 2
                opacity: 0.5
                z: -1
            }
        }
    }

    Item { Layout.preferredHeight: 20; Layout.fillWidth: true }
}
