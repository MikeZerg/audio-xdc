// qml/CustomWidget/CustomSpin.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: customSpin

    // === 可调属性 ===
    property int spinWidth: 150          // 总宽度，默认150
    property int spinHeight: 28          // 总高度，默认28
    property int value: 50
    property int from: 1
    property int to: 100
    property int step: 10
    property bool editable: true

    // 颜色属性
    property color backgroundColor: Theme.surface
    property color numberBackgroundColor: Theme.surface
    property color fontColor: Theme.text
    property color borderColor: Theme.borderLine

    // hover 高亮颜色
    property color hoverColor: Theme.surfaceSelected
    property color numberHoverColor: Theme.surfaceSelected

    // 字体属性
    property int fontSize: 12
    property bool fontBold: false

    // 边框宽度属性
    property double leftBorderWidth: 0.5
    property double centerBorderWidth: 0.5
    property double rightBorderWidth: 0.5

    // === 内部计算 ===
    // 比例：左按钮(0.24) : 中间(0.52) : 右按钮(0.24)
    property real leftRatio: 0.24
    property real centerRatio: 0.52
    property real rightRatio: 0.24

    property int leftWidth: Math.floor(spinWidth * leftRatio)
    property int centerWidth: Math.floor(spinWidth * centerRatio)
    property int rightWidth: spinWidth - leftWidth - centerWidth

    // === 尺寸设置 ===
    width: spinWidth
    height: spinHeight
    implicitWidth: spinWidth
    implicitHeight: spinHeight

    // === 信号定义 ===
    signal valueModified(var newValue)

    // === 内部值变化处理 ===
    onValueChanged: {
        valueModified(value)
    }

    // 当总宽度变化时，重新计算各部分宽度
    onSpinWidthChanged: {
        leftWidth = Math.floor(spinWidth * leftRatio)
        centerWidth = Math.floor(spinWidth * centerRatio)
        rightWidth = spinWidth - leftWidth - centerWidth
    }

    // === 控件布局 ===
    Row {
        anchors.fill: parent
        spacing: 0

        // 减号按钮
        Rectangle {
            id: minusRect
            width: customSpin.leftWidth
            height: parent.height
            color: minusArea.containsMouse ? customSpin.hoverColor : customSpin.backgroundColor
            border.color: customSpin.borderColor
            border.width: customSpin.leftBorderWidth
            radius: 1
            opacity: customSpin.editable ? 1.0 : 0.6

            Text {
                text: "−"
                color: customSpin.fontColor
                anchors.centerIn: parent
                font.pixelSize: customSpin.fontSize + 2
                font.bold: true
            }

            MouseArea {
                id: minusArea
                anchors.fill: parent
                hoverEnabled: true
                enabled: customSpin.editable
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (customSpin.value > customSpin.from) {
                        customSpin.value = customSpin.value - customSpin.step
                    }
                }
            }
        }

        // 数值显示区域
        Rectangle {
            id: centerRect
            width: customSpin.centerWidth
            height: parent.height
            color: centerArea.containsMouse ? customSpin.numberHoverColor : customSpin.numberBackgroundColor
            border.color: customSpin.borderColor
            border.width: customSpin.centerBorderWidth
            radius: 1

            TextInput {
                id: textInput
                visible: customSpin.editable
                text: customSpin.value
                color: customSpin.fontColor
                anchors.fill: parent
                anchors.margins: 4
                font.pixelSize: customSpin.fontSize
                font.bold: customSpin.fontBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                selectByMouse: true

                onEditingFinished: {
                    var newValue = parseInt(text)
                    if (!isNaN(newValue)) {
                        if (newValue < customSpin.from) {
                            customSpin.value = customSpin.from
                        } else if (newValue > customSpin.to) {
                            customSpin.value = customSpin.to
                        } else {
                            customSpin.value = newValue
                        }
                    }
                    text = customSpin.value
                }
            }

            Text {
                visible: !customSpin.editable
                text: customSpin.value
                color: customSpin.fontColor
                anchors.centerIn: parent
                font.pixelSize: customSpin.fontSize
                font.bold: customSpin.fontBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: centerArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.IBeamCursor
                // 点击时让 TextInput 获得焦点
                onClicked: {
                    if (customSpin.editable) {
                        textInput.forceActiveFocus()
                    }
                }
            }
        }

        // 加号按钮
        Rectangle {
            id: plusRect
            width: customSpin.rightWidth
            height: parent.height
            color: plusArea.containsMouse ? customSpin.hoverColor : customSpin.backgroundColor
            border.color: customSpin.borderColor
            border.width: customSpin.rightBorderWidth
            radius: 1
            opacity: customSpin.editable ? 1.0 : 0.6

            Text {
                text: "+"
                color: customSpin.fontColor
                anchors.centerIn: parent
                font.pixelSize: customSpin.fontSize + 2
                font.bold: true
            }

            MouseArea {
                id: plusArea
                anchors.fill: parent
                hoverEnabled: true
                enabled: customSpin.editable
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (customSpin.value < customSpin.to) {
                        customSpin.value = customSpin.value + customSpin.step
                    }
                }
            }
        }
    }
}