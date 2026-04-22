import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RadioButton {
    text: "自定义单选按钮"

    // 自定义指示器（圆圈部分）
    indicator: Rectangle {
        id: indicatorRect
        implicitWidth: 14
        implicitHeight: 14
        x: parent.leftPadding
        y: parent.topPadding + (parent.availableHeight - height) / 2
        radius: width / 2
        border.color: parent.checked ? Theme.textMenu : "#757575"
        border.width: 2
        color: parent.pressed ? "#888484" : "transparent"

        // 中心圆点 - 内圈直径 = 外圈直径 - 2 * 边框宽度
        Rectangle {
            width: indicatorRect.implicitWidth - (indicatorRect.border.width * 4)  // 14 - 8 = 6
            height: indicatorRect.implicitHeight - (indicatorRect.border.width * 4)  // 14 - 8 = 6
            radius: width / 2
            anchors.centerIn: parent
            color: parent.parent.checked ? Theme.textMenu : "transparent"
            visible: parent.parent.checked
        }
    }

    // 自定义文本样式
    contentItem: Text {
        text: parent.text
        font.pixelSize: 11
        font.family: "Microsoft YaHei"
        color: {
            if (!parent.enabled) {
                return "#888888"
            }
            return parent.checked ? Theme.textMenu : Theme.text
        }
        verticalAlignment: Text.AlignVCenter
        leftPadding: parent.indicator.width + parent.spacing
    }
}