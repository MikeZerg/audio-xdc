import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    width: parent.width
    height: 70
    color: Theme.surface
    border.color: Theme.borderLine
    border.width: 1
    radius: Theme.radiusS
    clip: true

    property var availableHosts: []      // 可用主机列表 [{address: 1, name: "主机1"}]
    property var selectedHosts: []       // 选中的主机地址列表
    property string title: "选择主机"

    signal selectionChanged(var selectedAddresses)

    function updateSelectedHosts() {
        selectionChanged(selectedHosts)
    }

    // 格式化地址为两位十六进制
    function formatAddress(addr) {
        return "0x" + addr.toString(16).toUpperCase().padStart(2, '0')
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
            Layout.fillWidth: true
        }

        // 主机选项行（水平布局）
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            Repeater {
                model: root.availableHosts

                CheckBox {
                    id: hostCheckBox
                    text: formatAddress(modelData.address)
                    checked: root.selectedHosts.indexOf(modelData.address) !== -1
                    font.pixelSize: 11

                    onCheckedChanged: {
                        if (checked) {
                            if (root.selectedHosts.indexOf(modelData.address) === -1) {
                                root.selectedHosts.push(modelData.address)
                            }
                        } else {
                            var index = root.selectedHosts.indexOf(modelData.address)
                            if (index !== -1) {
                                root.selectedHosts.splice(index, 1)
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
                    root.updateSelectedHosts()
                }
            }

            CustomButton {
                text: "清空"
                implicitWidth: 50
                implicitHeight: 24
                font.pixelSize: 11
                backgroundColor: "transparent"
                hoverColor: Theme.surfaceSelected
                pressedColor: Theme.surfaceSelected
                borderColor: Theme.borderLine
                borderWidth: 0.5
                textColor: Theme.text
                onClicked: {
                    root.selectedHosts = []
                    root.updateSelectedHosts()
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "已选: " + root.selectedHosts.length + " 台主机"
                color: Theme.textMenu
                font.pixelSize: 11
            }
        }
    }
}