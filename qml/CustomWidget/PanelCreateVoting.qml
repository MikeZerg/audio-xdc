// qml/CustomWidget/PanelCreateVoting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Popup {
    id: root

    property var meetingManager: null

    signal votingCreated(string subject, int durationMinutes, string voteType, var customOptions, bool anonymous, bool repeat)

    property string voteSubject: "会议投票"
    property int durationMinutes: 10
    property string voteType: "referendum"
    property var customOptions: []
    property bool anonymousVote: true
    property bool repeatVote: false

    width: parent.width * 0.4
    height: parent.height
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: parent.width
    y: (parent.height - height) / 2
    padding: 0
    modal: true

    enter: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 15
            to: parent.width + 15 - root.width
            easing.type: Easing.OutCubic
            duration: 350
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 15 - root.width
            to: parent.width + 15
            easing.type: Easing.InCubic
            duration: 200
        }
    }

    background: Rectangle {
        color: Theme.background
    }

    function resetForm() {
        voteSubject = "会议投票"
        durationMinutes = 5
        voteType = "referendum"
        customOptions = []
        anonymousVote = true
        repeatVote = false
    }

    function getVotingColor(value) {
        if (value === 1) return Theme.votingColorPalette[0]   // 赞成
        if (value === 0) return Theme.votingColorPalette[1]   // 弃权
        if (value === -1) return Theme.votingColorPalette[2]  // 反对

        var index = (Math.abs(value) - 2) + 3
        index = index % Theme.votingColorPalette.length
        return Theme.votingColorPalette[index]
    }

    function addCustomOption(optionText) {
        var newValue = customOptions.length + 2
        customOptions.push({
            option: optionText,
            value: newValue,
            color: getVotingColor(newValue)
        })
        customOptionsRepeater.model = customOptions
    }

    function removeCustomOption(index) {
        customOptions.splice(index, 1)
        for (var i = 0; i < customOptions.length; i++) {
            customOptions[i].value = i + 2
            customOptions[i].color = getVotingColor(i + 2)
        }
        customOptionsRepeater.model = customOptions
    }

    function clearCustomOptions() {
        customOptions = []
        customOptionsRepeater.model = customOptions
    }

    function addDefaultOptions() {
        clearCustomOptions()
        addCustomOption("选项A")
        addCustomOption("选项B")
        addCustomOption("选项C")
    }

    Component.onCompleted: {
        addDefaultOptions()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "创建投票"
                font.bold: true
                font.pixelSize: 16
                font.family: Theme.fontFamily
                color: Theme.text
            }
            Item { Layout.fillWidth: true }
            CustomButton {
                text: "✕"
                Layout.preferredHeight: 24
                Layout.preferredWidth: 24
                Layout.rightMargin: 4
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: "transparent"
                onClicked: root.close()
            }
        }

        Rectangle {
            height: 1
            Layout.fillWidth: true
            color: Theme.borderLine
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: scrollView.availableWidth
                spacing: 15

                // 投票主题
                ColumnLayout {
                    spacing: 5
                    Layout.fillWidth: true
                    Text { text: "投票主题"; color: Theme.text }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: Theme.background
                        border.color: Theme.borderLine
                        border.width: 1
                        radius: Theme.radiusS
                        TextInput {
                            id: subjectInput
                            anchors.fill: parent
                            anchors.margins: 8
                            text: voteSubject
                            onTextChanged: voteSubject = text
                            color: Theme.text
                            selectByMouse: true
                        }
                    }
                }

                // 投票时效
                ColumnLayout {
                    spacing: 5
                    Layout.fillWidth: true
                    Text { text: "投票时效 (分钟)"; color: Theme.text }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        CustomSlider {
                            id: durationSlider
                            from: 1
                            to: 30
                            stepSize: 1
                            value: durationMinutes
                            onValueChanged: durationMinutes = value
                            Layout.fillWidth: true
                            progressHeight: 6
                            handleSize: 14
                            handleRadius: 2
                        }
                        Text {
                            text: durationMinutes + "分钟"
                            color: Theme.textMenu
                            Layout.preferredWidth: 40
                        }
                    }
                }

                // 投票类型
                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    Text { text: "投票类型"; color: Theme.text }

                    RowLayout {
                        spacing: 20
                        CustomRadioButton {
                            text: "表决投票"
                            checked: voteType === "referendum"
                            onCheckedChanged: if (checked) voteType = "referendum"
                        }
                        CustomRadioButton {
                            text: "自定义选项"
                            checked: voteType === "custom"
                            onCheckedChanged: if (checked) voteType = "custom"
                        }
                    }
                }

                // 表决投票选项展示
                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    visible: voteType === "referendum"

                    Text { text: "投票选项"; color: Theme.text }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            height: 26
                            Layout.fillWidth: true
                            color: getVotingColor(1)
                            opacity: 0.8
                            radius: 4
                            Text {
                                text: "赞成 (1)"
                                anchors.centerIn: parent
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                        Rectangle {
                            height: 26
                            Layout.fillWidth: true
                            color: getVotingColor(0)
                            opacity: 0.8
                            radius: 4
                            Text {
                                text: "弃权 (0)"
                                anchors.centerIn: parent
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                        Rectangle {
                            height: 26
                            Layout.fillWidth: true
                            color: getVotingColor(-1)
                            opacity: 0.8
                            radius: 4
                            Text {
                                text: "反对 (-1)"
                                anchors.centerIn: parent
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                // 自定义投票选项
                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    visible: voteType === "custom"

                    Text { text: "自定义选项"; color: Theme.text }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            id: customOptionsRepeater
                            model: customOptions

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 36
                                color: Theme.background
                                border.color: Theme.borderLine
                                border.width: 1
                                radius: Theme.radiusS

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 10

                                    Rectangle {
                                        width: 20
                                        height: 20
                                        radius: 3
                                        color: modelData.color
                                        border.color: Theme.borderLine
                                        border.width: 0.5
                                    }

                                    Text {
                                        text: modelData.option + " (值:" + modelData.value + ")"
                                        color: Theme.text
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        font.pixelSize: 11
                                    }

                                    CustomButton {
                                        text: "移除"
                                        implicitWidth: 50
                                        implicitHeight: 24
                                        backgroundColor: "transparent"
                                        hoverColor: Theme.surfaceSelected
                                        pressedColor: Theme.surfaceSelected
                                        borderColor: Theme.borderLine
                                        borderWidth: 0.5
                                        font.pixelSize: 10
                                        onClicked: removeCustomOption(index)
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: Theme.background
                            border.color: Theme.borderLine
                            border.width: 1
                            radius: Theme.radiusS

                            TextInput {
                                id: newOptionInput
                                anchors.fill: parent
                                anchors.margins: 6
                                color: Theme.text
                                selectByMouse: true
                            }

                            Text {
                                text: "输入选项名称"
                                color: Theme.textDim
                                font.pixelSize: 12
                                anchors.fill: parent
                                anchors.margins: 8
                                visible: newOptionInput.text === ""
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        CustomButton {
                            text: "添加"
                            implicitWidth: 60
                            implicitHeight: 32
                            backgroundColor: "transparent"
                            hoverColor: Theme.surfaceSelected
                            pressedColor: Theme.surfaceSelected
                            borderColor: Theme.borderLine
                            borderWidth: 0.5
                            onClicked: {
                                if (newOptionInput.text.trim() !== "") {
                                    addCustomOption(newOptionInput.text.trim())
                                    newOptionInput.clear()
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        CustomButton {
                            text: "清空选项"
                            implicitHeight: 28
                            Layout.fillWidth: true
                            backgroundColor: "transparent"
                            hoverColor: Theme.surfaceSelected
                            pressedColor: Theme.surfaceSelected
                            borderColor: Theme.borderLine
                            borderWidth: 0.5
                            font.pixelSize: 11
                            onClicked: clearCustomOptions()
                        }

                        CustomButton {
                            text: "添加默认"
                            implicitHeight: 28
                            Layout.fillWidth: true
                            backgroundColor: "transparent"
                            hoverColor: Theme.surfaceSelected
                            pressedColor: Theme.surfaceSelected
                            borderColor: Theme.borderLine
                            borderWidth: 0.5
                            font.pixelSize: 11
                            onClicked: addDefaultOptions()
                        }
                    }
                }

                // 投票控制选项
                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    Text { text: "投票控制"; color: Theme.text }

                    CustomCheckBox {
                        text: "匿名投票"
                        checked: anonymousVote
                        onCheckedChanged: anonymousVote = checked
                    }

                    CustomCheckBox {
                        text: "自动重发投票"
                        checked: repeatVote
                        onCheckedChanged: repeatVote = checked
                    }

                    CustomCheckBox {
                        text: "仅有线单元投票"
                        checked: false
                        enabled: false
                    }
                }

                Item { height: 10 }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderLine
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CustomButton {
                text: "取消"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                onClicked: root.close()
            }

            CustomButton {
                id: btnCreateVoting
                text: "开始投票"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 26

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: {
                    votingCreated(voteSubject, durationMinutes, voteType, customOptions, anonymousVote, repeatVote)
                    root.close()
                }
            }
        }
    }
}
