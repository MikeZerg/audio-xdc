// // qml/CustomWidget/CustomTreeMenu.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: treeMenu

    // 定义初始
    property string currentCategory: "F1"
    property var menuItems: []
    property int treeMenuWidth: 180
    property string currentSelectedId: ""  // 记录当前选中的菜单项ID

    // 加载数据模型
    readonly property var menuData: CustomTreeMenuData {}

    // 更新菜单项
    function updateMenuItems() {
        menuItems = menuData.menuItems[currentCategory] || [];
        // 切换分类时清除选中状态
        currentSelectedId = "";
    }

    // 初始化菜单数据
    Component.onCompleted: {
        updateMenuItems();
    }

    Layout.preferredWidth: treeMenuWidth
    Layout.fillHeight: true
    spacing: 10     // 树状菜单上标题与下列项之间的间距

    // 树状菜单标题
    Rectangle {
        Layout.preferredWidth: treeMenuWidth
        Layout.preferredHeight: 32
        color: Theme.background
        border.width: 0

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 2
            text: {
                var item = menuData.functionButtons.find(function(btn) {
                    return btn.id === currentCategory;
                });
                return item ? item.name : currentCategory;
            }
            color: Theme.text
            font.bold: true
            font.pixelSize: 14
        }
    }

    // 上部水平分隔线
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Theme.borderFilled          // 分隔线颜色
        opacity: 0.8
    }

    // 树状菜单内容区域
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1   // 高度为1，分隔线
        Layout.minimumHeight: 1
        Layout.maximumHeight: parent.height
        color: "transparent"
        border.width: 0

        // 内部菜单项列表
        Column {
            spacing: 2  // 菜单项目之间间隔
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            width: parent.width

            Repeater {
                model: menuItems
                delegate: Item {
                    width: parent.width
                    height: 30

                    property bool isSelected: modelData.id === treeMenu.currentSelectedId
                    property bool isHovered: false

                    Rectangle {
                        anchors.fill: parent
                        color: modelData.level === 1 ? Theme.panel : Theme.background;
                        // {
                        //     if (isSelected) {
                        //         return Theme.warning;
                        //     }
                        //     return modelData.level === 1 ? Theme.panel : Theme.background;
                        // }
                        clip: true

                        // border.width: isSelected ? 1 : 0
                        // border.color: isSelected ? Theme.highlightBorder : "transparent"

                        Text {
                            id: menuItemText
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: (modelData.level === 2 ? 18 : 0) + 10    // 菜单缩进10，二级菜单额外缩进18, 与第一级文字对齐
                            text: (modelData.level === 2 ? modelData.name : "●   " + modelData.name) // 一级菜单加前缀
                            color: {
                                if (isSelected) {
                                    return Theme.textMenu;
                                }
                                if (isHovered) {
                                    return Theme.textMenu;
                                }
                                return Theme.text;
                            }
                            font.pixelSize: 12
                            font.bold: isSelected
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true  // 启用悬停检测

                            onEntered: {
                                parent.parent.isHovered = true;
                            }

                            onExited: {
                                parent.parent.isHovered = false;
                            }

                            onClicked: function() {
                                if (treeMenu.currentSelectedId !== modelData.id) {
                                    treeMenu.currentSelectedId = modelData.id;
                                    treeMenu.menuItemSelected(modelData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    signal menuItemSelected(var item)
}