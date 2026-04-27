// qml/CustomWidget/CustomMultipleCheckBox.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    width: parent.width
    height: 70
    color: Qt.darker(Theme.surface, 1.3)
    border.color: Theme.borderLine
    border.width: 1
    radius: Theme.radiusS
    clip: true

    property var availableHosts: []      // 可用主机列表 [{address: 1, name: "主机1"}]
    property var selectedHosts: []       // 选中的主机地址列表
    property string title: "选择主机"

    signal selectionChanged(var selectedAddresses)

    // 选择地址为会议主机
    function updateSelectedHosts() {
        console.log("[CustomMultipleCheckBox] 更新选中主机:", selectedHosts)
        selectionChanged(selectedHosts)
    }

    // 格式化地址为两位十六进制
    function formatAddress(addr) {
        return "0x" + addr.toString(16).toUpperCase().padStart(2, '0')
    }

    // 检查指定地址是否被选中
    function isAddressSelected(address) {
        var result = root.selectedHosts.indexOf(address) !== -1
        return result
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // 标题行
        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: 11
            font.family: Theme.fontFamily
            Layout.fillWidth: true
        }

        // 主机选项行（水平布局）
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            Repeater {
                id: hostRepeater
                model: root.availableHosts

                CheckBox {
                    id: hostCheckBox
                    text: formatAddress(modelData.address)
                    checked: root.isAddressSelected(modelData.address)
                    font.pixelSize: 11
                    font.family: Theme.fontFamily

                    // 外部方框边框始终保持 Theme.textMenu 颜色，不随选中状态变化
                    indicator: Rectangle {
                        implicitWidth: 14
                        implicitHeight: 14
                        x: hostCheckBox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 2
                        border.color: Theme.textMenu   // 固定颜色，不随状态变化
                        border.width: 2
                        color: "transparent"

                        // [关键] 内部对勾 - 只有它表示选中状态
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 1
                            anchors.centerIn: parent
                            visible: hostCheckBox.checked   // 只有选中时才显示
                            color: Theme.textMenu
                        }
                    }

                    // 文字颜色根据选中状态变化
                    contentItem: Text {
                        text: hostCheckBox.text
                        font: hostCheckBox.font
                        color: hostCheckBox.checked ? Theme.textMenu : Theme.text
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: hostCheckBox.indicator.width + hostCheckBox.spacing
                    }

                    onClicked: {
                        var address = modelData.address
                        var index = root.selectedHosts.indexOf(address)

                        console.log("[CustomMultipleCheckBox] 点击复选框, 地址:", address, "当前选中状态:", hostCheckBox.checked)

                        if (hostCheckBox.checked) {
                            if (index === -1) {
                                root.selectedHosts.push(address)
                                console.log("[CustomMultipleCheckBox] 添加主机:", address)
                            }
                        } else {
                            if (index !== -1) {
                                root.selectedHosts.splice(index, 1)
                                console.log("[CustomMultipleCheckBox] 移除主机:", address)
                            }
                        }
                        root.updateSelectedHosts()
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        // 按钮行
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CustomButton {
                text: "全选"
                implicitWidth: 50
                implicitHeight: 24
                font.pixelSize: 11
                font.family: Theme.fontFamily
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: Theme.borderLine
                borderWidth: 0.5
                textColor: Theme.text
                onClicked: {
                    root.selectedHosts = []
                    for (var i = 0; i < root.availableHosts.length; i++) {
                        root.selectedHosts.push(root.availableHosts[i].address)
                    }
                    console.log("[CustomMultipleCheckBox] 全选:", root.selectedHosts)
                    root.updateSelectedHosts()
                }
            }

            CustomButton {
                text: "清空"
                implicitWidth: 50
                implicitHeight: 24
                font.pixelSize: 11
                font.family: Theme.fontFamily
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: Theme.borderLine
                borderWidth: 0.5
                textColor: Theme.text
                onClicked: {
                    root.selectedHosts = []
                    console.log("[CustomMultipleCheckBox] 清空")
                    root.updateSelectedHosts()
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "已选: " + root.selectedHosts.length + " 台主机"
                color: Theme.textMenu
                font.pixelSize: 11
                font.family: Theme.fontFamily
            }
        }
    }
}
