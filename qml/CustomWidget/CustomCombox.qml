// qml/CustomWidget/CustomComboBox.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

ComboBox {
    id: customCombox

    // === 自定义样式属性 ===
    property color backgroundColor: Theme.surface      // 背景颜色
    property color borderColor: Theme.borderLine      // 边框颜色
    property color textColor: Theme.text              // 文本颜色
    property color selectionColor: Theme.surfaceSelected    // 选中项背景色
    property int borderRadius: 2          // 圆角半径


    // === 样式定义 ===
    background: Rectangle {
        color: customCombox.backgroundColor
        border.color: customCombox.borderColor
        border.width: 1
        radius: customCombox.borderRadius
    }

    contentItem: Text {
        text: customCombox.currentText
        color: customCombox.textColor
        font: customCombox.font
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        leftPadding: 8
        rightPadding: 24
    }

    delegate: ItemDelegate {
        width: customCombox.width
        height: 28

        contentItem: Text {
            text: modelData !== undefined ? modelData :
                  (model.display !== undefined ? model.display : "")
            color: customCombox.textColor
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            leftPadding: 8
        }

        background: Rectangle {
            color: index === customCombox.currentIndex ?
                   customCombox.selectionColor : customCombox.backgroundColor
        }
    }
}
