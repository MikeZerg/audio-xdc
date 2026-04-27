import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Audio.Log 1.0
import "./CustomWidget"


ColumnLayout {
    id: host_detecting_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

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

    Item { Layout.preferredHeight: 30 }

    ColumnLayout {

        Label {
            text: "设置串口通信缓存大小"
            color: Theme.text
        }
        Label {
            text: "设置主机地址范围"
            color: Theme.text
        }
        Label {
            text: "设置XDC236主机单元地址范围"
            color: Theme.text
        }
        Label {
            text: "设置XDC236主机会议记录保存路径"
            color: Theme.text
        }

    }

    Item { Layout.preferredHeight: 20; Layout.fillWidth: true }
}
