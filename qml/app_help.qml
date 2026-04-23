import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

ColumnLayout {
    id: app_help_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    property int selectedIndex: 0

    // 帮助数据模型
    property var helpData: [
        {
            title: "快速入门",
            icon: "📖",
            content: "欢迎使用会议管理系统！本系统支持多主机协同会议管理。\n\n" +
                     "【开始使用】\n" +
                     "1. 在左侧导航栏选择功能模块\n" +
                     "2. 点击「创建会议」按钮创建新会议\n" +
                     "3. 在日程视图中双击日期快速创建会议\n" +
                     "4. 会议进行中可实时监控主机状态\n\n" +
                     "【主要功能】\n" +
                     "• 主机监控 - 实时查看主机连接状态\n" +
                     "• 会议管理 - 创建、编辑、删除会议\n" +
                     "• 日程查看 - 日/周/工作周视图切换\n" +
                     "• 投票签到 - 支持多种会议互动功能"
        },
        {
            title: "主机管理",
            icon: "🖥️",
            content: "【主机连接】\n" +
                     "• 系统自动检测并显示已连接的主机\n" +
                     "• 主机卡片显示连接状态、信号强度等信息\n" +
                     "• 支持查看主机详细信息和配置\n\n" +
                     "【状态指示】\n" +
                     "• 绿色 - 主机在线且正常\n" +
                     "• 黄色 - 主机连接不稳定\n" +
                     "• 红色 - 主机离线或故障\n\n" +
                     "【会议关联】\n" +
                     "• 创建会议时可选择关联的主机\n" +
                     "• 会议进行中可实时监控主机状态\n" +
                     "• 支持批量发送指令到多个主机"
        },
        {
            title: "会议管理",
            icon: "📅",
            content: "【创建会议】\n" +
                     "• 点击「+ 创建会议」按钮或双击日程中的日期\n" +
                     "• 填写会议主题、描述、开始时间和时长\n" +
                     "• 选择需要关联的主机设备\n" +
                     "• 点击「创建会议」完成创建\n\n" +
                     "【编辑会议】\n" +
                     "• 在会议列表中点击会议卡片进入编辑模式\n" +
                     "• 可修改会议时间、描述等信息\n" +
                     "• 支持快速开始和结束会议\n\n" +
                     "【会议状态】\n" +
                     "• NotStarted - 未开始\n" +
                     "• InProgress - 进行中\n" +
                     "• Completed - 已完成"
        },
        {
            title: "日程视图",
            icon: "🗓️",
            content: "【视图切换】\n" +
                     "• 日视图 - 显示单天的详细时间安排\n" +
                     "• 工作周视图 - 显示周一至周五的日程\n" +
                     "• 周视图 - 显示完整一周的日程安排\n\n" +
                     "【快捷操作】\n" +
                     "• 双击任意时间格可快速创建会议\n" +
                     "• 点击「今天」按钮快速返回当前日期\n" +
                     "• 点击左侧月历中的日期切换显示\n\n" +
                     "【时间轴说明】\n" +
                     "• 纵轴显示 24 小时时间段\n" +
                     "• 横轴显示日期列\n" +
                     "• 彩色块表示会议事件，显示标题和时间"
        },
        {
            title: "投票与签到",
            icon: "✅",
            content: "【签到功能】\n" +
                     "• 会议中可发起签到\n" +
                     "• 支持快速签到和定时签到\n" +
                     "• 实时统计签到人数和名单\n\n" +
                     "【投票功能】\n" +
                     "• 支持表决投票（赞成/弃权/反对）\n" +
                     "• 支持自定义选项投票（最多10个选项）\n" +
                     "• 实时显示投票结果和统计图表\n\n" +
                     "【操作说明】\n" +
                     "• 在会议进行中点击「发起签到/投票」\n" +
                     "• 设置投票主题和选项\n" +
                     "• 选择投票模式（公开/匿名）\n" +
                     "• 点击「开始投票」并等待结果"
        },
        {
            title: "常见问题",
            icon: "❓",
            content: "【Q: 如何连接新主机？】\n" +
                     "A: 系统会自动检测网络中的主机，无需手动添加。确保主机与电脑在同一网络。\n\n" +
                     "【Q: 会议创建失败怎么办？】\n" +
                     "A: 检查：1) 会议主题不能为空 2) 开始时间不能早于当前时间 3) 至少选择一个主机\n\n" +
                     "【Q: 如何修改已创建的会议？】\n" +
                     "A: 在会议列表或日程视图中点击会议卡片，进入编辑模式后修改并保存。\n\n" +
                     "【Q: 主机显示离线怎么处理？】\n" +
                     "A: 1) 检查主机电源和网络连接 2) 重启主机设备 3) 在主机设置页面刷新连接\n\n" +
                     "【Q: 投票结果如何查看？】\n" +
                     "A: 投票结束后会自动显示结果图表，也可在会议记录中查看历史投票数据。"
        },
        {
            title: "技术支持",
            icon: "🔧",
            content: "【系统信息】\n" +
                     "• 版本：1.0.1\n" +
                     "• 构建时间：2026-04-19\n" +
                     "• 运行环境：Win 11\n\n" +
                     "【联系方式】\n" +
                     "• 产品网站：http://www.iweex.cn/\n\n" +
                     "• 技术支持邮箱：support@example.com\n" +
                     "• 客服电话：400-XXX-XXXX\n" +
                     "【日志查看】\n" +
                     "• 可在「系统日志」页面查看运行日志\n" +
                     "• 遇到问题时请提供日志信息以便排查\n\n" +
                     "【更新说明】\n" +
                     "• 定期关注系统更新公告\n" +
                     "• 建议保持最新版本以获得最佳体验"
        }
    ]

    Item { Layout.preferredHeight: 20 }

    // 主内容区域
    RowLayout {
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true
        Layout.leftMargin: 25
        Layout.rightMargin: 20
        Layout.bottomMargin: 4
        spacing: 20

        // 左侧导航列表
        Rectangle {
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: Theme.panel
            radius: Theme.radiusL
            border.color: Theme.borderLine
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label {
                    text: "帮助导航"
                    font.pixelSize: 14
                    font.bold: true
                    color: Theme.text
                    Layout.bottomMargin: 8
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 0.5
                    color: Theme.borderLine
                }

                ListView {
                    id: helpListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: app_help_page.helpData
                    spacing: 4

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 28
                        radius: Theme.radiusS
                        color: {
                            if (index === app_help_page.selectedIndex) return Qt.darker(Theme.textMenu, 1.3);
                            if (helpMouseArea.containsMouse) return Theme.surfaceSelected;
                            return "transparent";
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: modelData.icon
                                font.pixelSize: 18
                                Layout.preferredWidth: 24
                            }

                            Text {
                                text: modelData.title
                                color: index === app_help_page.selectedIndex ? Theme.textLit : Theme.text
                                font.pixelSize: 12
                                font.bold: index === app_help_page.selectedIndex
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            id: helpMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: app_help_page.selectedIndex = index
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }
        }

        // 右侧内容显示区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.panel
            radius: Theme.radiusL
            border.color: Theme.borderLine
            border.width: 1

            Flickable {
                id: contentFlickable
                anchors.fill: parent
                anchors.margins: 20
                contentWidth: width
                contentHeight: contentColumn.height
                clip: true

                ColumnLayout {
                    id: contentColumn
                    width: contentFlickable.width
                    spacing: 16

                    // 标题区域
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: helpData[selectedIndex].icon
                            font.pixelSize: 18
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: helpData[selectedIndex].title
                                font.pixelSize: 14
                                font.bold: true
                                color: Theme.text
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: Theme.borderLine
                            }
                        }
                    }

                    // 内容区域 - 使用 TextArea 确保文字不会重叠
                    TextArea {
                        Layout.fillWidth: true
                        text: helpData[selectedIndex].content
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.Wrap

                        // 关键设置：防止文字重叠
                        readOnly: true
                        selectByMouse: true
                        background: Rectangle { color: "transparent" }
                        cursorVisible: false
                        padding: 0
                        leftPadding: 4
                    }

                    // 底部提示
                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.borderLine
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "提示：点击左侧列表切换帮助内容"
                            color: Theme.textMenu
                            font.pixelSize: 10
                            font.italic: true
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }
    }

    Item { Layout.preferredHeight: 20; Layout.fillWidth: true }
}