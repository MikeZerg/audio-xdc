import QtQuick 2.15
import QtQuick.Controls 2.15

CheckBox {
    id: root
    text: "复选框"

    // 自定义指示器（只改变外观，逻辑复用原生）
    indicator: Rectangle {
        implicitWidth: 14
        implicitHeight: 14
        x: root.leftPadding
        y: root.topPadding + (root.availableHeight - height) / 2
        radius: 2
        border.color: root.checked ? Theme.textMenu : "#757575"
        border.width: 2
        color: root.pressed ? "#888484" : "transparent"

        // 勾选标记
        Text {
            anchors.centerIn: parent
            text: "✓"
            font.pixelSize: 10
            font.bold: true
            color: Theme.textMenu
            visible: root.checked
        }
    }

    // 自定义文本样式
    contentItem: Text {
        text: root.text
        font.pixelSize: 11
        font.family: "Microsoft YaHei"
        color: {
            if (!root.enabled) return "#888888"
            return root.checked ? Theme.textMenu : Theme.text
        }
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + root.spacing
    }
}