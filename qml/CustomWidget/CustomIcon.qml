// CustomIcon.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: functionButtons
    Layout.preferredWidth: 60
    Layout.fillHeight: true
    spacing: 12

    // 加载数据模型
    readonly property var menuData: CustomTreeMenuData {}

    // 功能按钮 F1-F5
    Repeater {
        model: menuData.functionButtons
        delegate: Rectangle {
            width: 60
            height: 60
            color: Theme.panel
            border.color: Theme.borderLine
            border.width: 1
            radius: Theme.radiusS

            Image {
                id: functionButtonIcon
                anchors.centerIn: parent
                source: iconArea.containsMouse ? modelData.hoverIcon : modelData.icon
                width: 42
                height: 42
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                id: iconArea
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    functionButtons.functionClicked(modelData.id)
                }

                onDoubleClicked: {
                    functionButtons.functionDoubleClicked(modelData.id)
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.preferredWidth: 60
    }

    // 底部问号按钮
    Rectangle {
        Layout.preferredHeight: 60
        Layout.preferredWidth: 60
        radius: 2
        color: Theme.panel
        border.color: Theme.borderLine
        border.width: 1

        Image {
            id: appHelpIcon
            anchors.centerIn: parent
            source: helpIconArea.containsMouse ? "qrc:/image/app-helphover.svg" : "qrc:/image/app-help.svg"
            width: 42
            height: 42
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        MouseArea {
            id: helpIconArea
            anchors.fill: parent
            hoverEnabled: true

            onClicked: {
                // 打开帮助页面 help.qml
            }
        }
    }

    Item {
        Layout.preferredHeight: 30
        Layout.preferredWidth: 60
    }

    signal functionClicked(string functionId)
    signal functionDoubleClicked(string functionId)
}