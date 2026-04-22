// qml/CustomWidget/CustomButton.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: customButton

    // === 自定义样式属性 ===
    property color backgroundColor: Theme.surface
    property color hoverColor: Theme.panel
    property color pressedColor: Theme.panel
    property color textColor: Theme.text
    property color borderColor: Theme.borderLine
    property double borderWidth: 1
    property int borderRadius: 2

    // 使用Qt Quick Controls的内置enabled属性
    enabled: true

    // === 样式定义 ===
    background: Rectangle {
        id: buttonBackground
        color: customButton.backgroundColor  // 始终使用正常背景色
        border.color: customButton.borderColor  // 始终使用正常边框色
        border.width: borderWidth
        radius: customButton.borderRadius
        opacity: customButton.enabled ? 1.0 : 0.5  // 只控制透明度
    }

    // 自定义文本颜色
    contentItem: Text {
        text: customButton.text
        color: customButton.textColor  // 始终使用正常文字色
        font: customButton.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: customButton.enabled ? 1.0 : 0.35  // 只控制透明度
    }

    // 状态管理 - 禁用时不改变任何颜色，只控制交互和透明度
    states: [
        State {
            name: "normal"
            when: customButton.enabled && !customButton.pressed && !customButton.hovered
            // 正常状态，使用默认样式
        },
        State {
            name: "hovered"
            when: customButton.enabled && customButton.hovered && !customButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: customButton.hoverColor
            }
        },
        State {
            name: "pressed"
            when: customButton.enabled && customButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: customButton.pressedColor
            }
        },
        State {
            name: "disabled"
            when: !customButton.enabled
            // 禁用状态：不改变颜色，只通过opacity控制透明度
            // hover和pressed状态也不会触发
        }
    ]
}
