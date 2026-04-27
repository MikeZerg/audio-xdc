// Main.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "." // 导入当前目录

ApplicationWindow {
    id: mainApp
    visible: true
    width: 1280
    height: 800
    title: "WIRED & WIRELESS INTEGRATED DIGITAL CONFERENCE SYSTEM"
    color: Theme.background

    property bool startupStatus: false
    property bool treeMenuFold: false
    readonly property var menuData: CustomTreeMenuData {}   // 创建菜单数据实例

    // 背景装饰矩形
    Rectangle {
        anchors.fill: parent
        // radius: Theme.radiusL
        color: Theme.background
        anchors.margins: -10
        z: -1
    }

    // 主布局：水平排列的三栏结构
    RowLayout {
        anchors.fill: parent
        layoutDirection: Qt.LeftToRight
        spacing: 18
        anchors.margins: 18

        // 左侧功能按钮区域
        CustomIcon {
            id: functionButtons
            Layout.alignment: Qt.AlignTop
            // 单击事件：跳转到对应页面
            onFunctionClicked: function(functionId) {
                treeMenu.currentCategory = functionId;
                treeMenu.updateMenuItems();

                // 根据点击的功能按钮，跳转到对应的默认页面
                var targetPageId = ""
                switch(functionId) {
                    case "F1":
                        targetPageId = "F1-10"
                        break
                    case "F2":
                        targetPageId = "F2-10"
                        break
                    case "F3":
                        targetPageId = "F3-10"
                        break
                    case "F4":
                        targetPageId = "F4-10"
                        break
                    case "F5":
                        targetPageId = "F5-10"
                        break
                    default:
                        targetPageId = "F1-10"
                }

                // 跳转到对应页面
                pageManager.jumpToPage(targetPageId)
            }

            // 双击事件：展开/折叠树状菜单
            onFunctionDoubleClicked: function(functionId) {
                if (treeMenuFold) {
                    treeMenuFold = false  // 展开 -> 折叠
                    iconArrow.text = String.fromCharCode(238)
                } else {
                    treeMenuFold = true   // 折叠 -> 展开
                    iconArrow.text = String.fromCharCode(237)
                }
            }
        }

        // 功能按钮右侧竖直分隔线及“展开/折叠”箭头
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Theme.borderFilled
            opacity: 0.8

            Text {
                id: iconArrow
                text: String.fromCharCode(238)
                color: Theme.text
                font.family: "Wingdings 3"
                font.pixelSize: 28
                font.bold: true
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (treeMenuFold) {
                            treeMenuFold = false // 展开状态 -> 折叠
                            iconArrow.text = String.fromCharCode(238);
                        } else  {
                            treeMenuFold = true  // 折叠状态 -> 展开
                            iconArrow.text = String.fromCharCode(237);
                        }
                    }
                }
            }
        }

        // 中间树状菜单区域
        CustomTreeMenu {
            id: treeMenu
            visible: treeMenuFold
            Layout.alignment: Qt.AlignTop
            Layout.minimumWidth: 160
            Layout.maximumWidth: 200
            Layout.fillWidth: false

            onMenuItemSelected: function(item) {
                // 直接使用菜单项
                pageManager.showPage(item);
            }
        }

        // 右侧显示区域
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // 显示区域标题
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: Theme.background
                border.width: 0

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    text: "iWeeX 数字音频设备管理系统  "
                    color: Theme.text
                    font.bold: true
                    font.pixelSize: 18
                }
            }

            // 内容显示区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.panel
                radius: Theme.radiusS   // 2号圆角

                Item {
                    id: pageManager
                    anchors.fill:parent
                    anchors.margins: 15

                    // 存储所有已加载的页面的实例
                    property var pageInstances: ({})
                    property string currentPageId: ""

                    // 跳转显示页面，全局函数，供其它页面不用点击菜单跳转指定ID页面
                    function jumpToPage(pageId) {
                        if (menuData && menuData.menuItems) {
                            // 遍历菜单数据找到对应的 menuItem
                            for (var cat in menuData.menuItems) {
                                var items = menuData.menuItems[cat];
                                for (var i = 0; i < items.length; i++) {
                                    if (items[i].id === pageId) {
                                        showPage(items[i]);
                                        return;
                                    }
                                }
                            }
                        }
                    }
                    // 加载显示页面
                    function showPage(menuItem) {
                        if (!menuItem.page) {
                            console.warn("菜单项 " + menuItem.name + " 未设置页面");
                            return;
                        }

                        var pageId = menuItem.id;

                        // 如果是同一页面，不做操作
                        if (currentPageId === pageId) return;

                        // 隐藏当前页面
                        if (currentPageId && pageInstances[currentPageId]) {
                            pageInstances[currentPageId].visible = false;
                        }

                        // 显示或创建新页面
                        if (!pageInstances[pageId]) {
                            var component = Qt.createComponent(menuItem.page);
                            if (component.status === Component.Ready) {
                                var page = component.createObject(pageManager);
                                page.anchors.fill = pageManager;
                                page.visible = false;  // 先隐藏

                                // 不要添加 pageId 属性，直接用对象引用
                                pageInstances[pageId] = page;

                                console.log("创建新页面:", menuItem.name, "ID:", pageId);
                            } else if (component.status === Component.Error) {
                                console.error("加载页面失败:", menuItem.page, component.errorString());
                                return;
                            } else {
                                // 异步加载的情况
                                component.statusChanged.connect(function() {
                                    if (component.status === Component.Ready) {
                                        var page = component.createObject(pageManager);
                                        page.anchors.fill = pageManager;
                                        page.visible = false;
                                        pageInstances[pageId] = page;

                                        // 如果这个页面是当前要显示的，显示它
                                        if (currentPageId === pageId) {
                                            page.visible = true;
                                        }
                                    }
                                });
                                return;
                            }
                        }

                        // 显示页面
                        if (pageInstances[pageId]) {
                            pageInstances[pageId].visible = true;
                            currentPageId = pageId;
                            console.log("切换到页面:", menuItem.name, "ID:", pageId);
                        }
                    }

                    // 默认加载第一个页面 Id: F1
                    Component.onCompleted: {
                        // 确保 menuData 已定义
                        if (menuData && menuData.menuItems && menuData.menuItems["F1"]) {
                            var defaultItem = menuData.menuItems["F1"].find(function(item) {
                                return item.page === "qml/Startup_Product.qml";
                            });
                            if (defaultItem) {
                                showPage(defaultItem);
                            } else if (menuData.menuItems["F1"].length > 0) {
                                // 如果找不到指定页面，加载第一个
                                showPage(menuData.menuItems["F1"][0]);
                            }
                        } else {
                            console.error("menuData 未正确初始化");
                        }
                    }
                }
            }
        }
    }


    // 在 Main.qml 的 Component.onCompleted 中加载JSON会议数据文件
    Component.onCompleted: {
        if (meetingManager) {
            meetingManager.loadMeetingsFromJson();
        }
    }
}
