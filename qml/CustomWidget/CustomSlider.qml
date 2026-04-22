// qml/CustomWidget/CustomSlider.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    id: customSlider

    // === 自定义样式属性 ===
    property color backgroundColor: Theme.surface        // 背景色
    property color progressColor: Theme.textMenu          // 进度条颜色
    property int progressHeight: 4                       // 进度条高度
    property int progressWidth: 0                        // 进度条长度（0表示自动填满可用宽度）

    property int handleSize: 14                          // 滑块大小
    property int handleRadius: 0                         // 滑块圆角半径（0表示使用默认值 handleSize/2）
    property color handleColor: Theme.textMenu           // 滑块颜色
    property color handleBorderColor: Theme.borderLineL  // 滑块边框颜色
    property double handleBorderWidth: 1                 // 滑块边框宽度

    // 计算滑块圆角半径
    function getHandleRadius() {
        var defaultRadius = customSlider.handleSize / 2
        if (customSlider.handleRadius > 0) {
            return Math.min(customSlider.handleRadius, defaultRadius)
        }
        return defaultRadius
    }

    // 使用 Theme 样式
    background: Rectangle {
        x: customSlider.leftPadding + (customSlider.progressWidth > 0 ? (customSlider.availableWidth - customSlider.progressWidth) / 2 : 0)
        y: customSlider.topPadding + (customSlider.availableHeight - height) / 2
        width: customSlider.progressWidth > 0 ? customSlider.progressWidth : customSlider.availableWidth
        height: customSlider.progressHeight
        radius: customSlider.progressHeight / 2
        color: customSlider.backgroundColor

        // 进度条
        Rectangle {
            width: (customSlider.value - customSlider.from) / (customSlider.to - customSlider.from) * parent.width
            height: parent.height
            radius: parent.radius
            color: customSlider.progressColor
        }
    }

    // 滑块 - 可拖拽
    handle: Rectangle {
        id: handleRect
        width: customSlider.handleSize
        height: customSlider.handleSize
        radius: customSlider.getHandleRadius()
        color: customSlider.handleColor
        border.color: customSlider.handleBorderColor
        border.width: customSlider.handleBorderWidth

        // 计算滑块位置
        x: {
            var trackWidth = customSlider.progressWidth > 0 ? customSlider.progressWidth : customSlider.availableWidth
            var trackStart = customSlider.leftPadding + (customSlider.progressWidth > 0 ? (customSlider.availableWidth - customSlider.progressWidth) / 2 : 0)
            var ratio = (customSlider.value - customSlider.from) / (customSlider.to - customSlider.from)
            return trackStart + ratio * trackWidth - width / 2
        }
        y: customSlider.topPadding + (customSlider.availableHeight - height) / 2

        // 悬停效果
        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: {
                var trackStart = customSlider.leftPadding + (customSlider.progressWidth > 0 ? (customSlider.availableWidth - customSlider.progressWidth) / 2 : 0)
                return trackStart - parent.width / 2
            }
            drag.maximumX: {
                var trackWidth = customSlider.progressWidth > 0 ? customSlider.progressWidth : customSlider.availableWidth
                var trackStart = customSlider.leftPadding + (customSlider.progressWidth > 0 ? (customSlider.availableWidth - customSlider.progressWidth) / 2 : 0)
                return trackStart + trackWidth - parent.width / 2
            }

            onPositionChanged: {
                if (drag.active) {
                    var trackWidth = customSlider.progressWidth > 0 ? customSlider.progressWidth : customSlider.availableWidth
                    var trackStart = customSlider.leftPadding + (customSlider.progressWidth > 0 ? (customSlider.availableWidth - customSlider.progressWidth) / 2 : 0)
                    var ratio = (parent.x + parent.width / 2 - trackStart) / trackWidth
                    var newValue = customSlider.from + ratio * (customSlider.to - customSlider.from)
                    newValue = Math.max(customSlider.from, Math.min(customSlider.to, newValue))
                    if (customSlider.stepSize > 0) {
                        newValue = Math.round(newValue / customSlider.stepSize) * customSlider.stepSize
                    }
                    customSlider.value = newValue
                }
            }

            onEntered: parent.scale = 1.1
            onExited: parent.scale = 1.0
            Behavior on scale {
                NumberAnimation { duration: 100 }
            }
        }
    }
}