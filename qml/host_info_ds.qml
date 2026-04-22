// host_info_ds.qml - DS240 跳频主机属性显示组件
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

Item {
    id: root
    anchors.fill: parent

    // 对外暴露的属性
    property int hostAddress: 0
    property var controller: hostAddress > 0 ? hostManager.getController(hostAddress) : null

    property bool forceUpdate: false

    // DS240 主机的属性（根据实际协议定义）
    property var hostProps: controller ? controller.hostProperties : null

    onHostAddressChanged: {
        controller = hostAddress > 0 ? hostManager.getController(hostAddress) : null
        forceUpdate = !forceUpdate
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.borderLine
        border.width: 0
        radius: 2

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            // 标题
            GridLayout {
                columns: 4
                columnSpacing: 15
                rowSpacing: 15
                Layout.fillWidth: true

                Label {
                    text: "当前选中的主机:"
                    color: Theme.text
                    font.pixelSize: 12
                    font.bold: true
                }
                Label {
                    text: "0x" + (hostAddress ? hostAddress.toString(16).toUpperCase().padStart(2, '0') : "00")
                    color: Theme.warning
                    font.pixelSize: 12
                    font.bold: true
                }
                Label {
                    text: "设备类型: DS240 跳频主机"
                    color: Theme.text
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
            }

            // 分隔线
            Rectangle {
                Layout.preferredHeight: 0.5
                Layout.preferredWidth: parent.width
                color: Theme.borderFilled
                opacity: 0.2
            }

            // 提示信息
            Label {
                text: "DS240 跳频主机功能开发中..."
                color: Theme.text
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 50
            }

            Label {
                text: "请等待后续版本更新"
                color: Theme.text
                opacity: 0.5
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}