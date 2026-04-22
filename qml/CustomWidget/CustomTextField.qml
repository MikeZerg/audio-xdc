// qml/CustomWidget/CustomTextField.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    id: customTextField

    // === 自定义样式属性 ===
    property color backgroundColor: Theme.surface    // 背景颜色
    property color borderColor: Theme.borderLine    // 边框颜色
    property color textColor: Theme.text            // 文本颜色
    property color placeholderColor: Theme.textDim  // 占位符颜色
    property int borderRadius: Theme.radiusS        // 圆角半径

    // === 样式定义 ===
    color: customTextField.textColor
    placeholderTextColor: customTextField.placeholderColor
    horizontalAlignment: Text.AlignLeft

    // 自定义背景
    background: Rectangle {
        color: customTextField.backgroundColor
        border.color: customTextField.borderColor
        border.width: 1
        radius: customTextField.borderRadius
    }
}
