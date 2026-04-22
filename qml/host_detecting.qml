import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

ColumnLayout {
    id: host_detecting_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // 添加选中主机的属性
    property int selectedHostAddress: 0

    // 顶部标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "检测在线主机及主机属性"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredHeight: 40 }

    // ========== 主机模型显示区域 ==========
    Item {
        id: hostModelView
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 180
        Layout.leftMargin: 25
        Layout.rightMargin: 20
        Layout.bottomMargin: 4

        ColumnLayout {
            Layout.preferredWidth: parent.width
            spacing: 4

            // 主机列表容器 - 使用 anchors 填充父项
            Rectangle {
                id: hostViewSection

               // 固定宽度 - 根据父容器宽度计算能容纳的整数个主机，最少3个
               width: {
                   var containerWidth = parent.width - 60  // 减去左右边距 (30 + 30)
                   var hostWidth = 210
                   var spacing = 10
                   var leftMargin = 10   // 内容左边距
                   var rightMargin = 10  // 内容右边距

                   // 可用内容宽度 = 容器宽度 - 内容左右边距
                   var availableWidth = containerWidth - leftMargin - rightMargin

                   // 计算能容纳的完整主机数量 (向下取整)
                   // 每个主机占用的总宽度 = 主机宽度 + 间距 (最后一个主机后面没有间距)
                   var hostTotalWidth = hostWidth + spacing
                   var maxHostCount = Math.floor((availableWidth + spacing) / hostTotalWidth)

                   // 最少显示3个主机
                   maxHostCount = Math.max(4, maxHostCount)

                   // 实际内容宽度 = 左边距 + (主机宽度 * 数量) + (间距 * (数量 - 1)) + 右边距
                   var contentWidth = leftMargin +
                                     (hostWidth * maxHostCount) +
                                     (spacing * (maxHostCount - 1)) +
                                     rightMargin

                   return contentWidth
               }

               height: 140
               //anchors.horizontalCenter: parent.horizontalCenter  // 水平居中
               color: "transparent"
               border.color: Theme.borderLine
               border.width: hostListView.count === 0 ? 0.5 : 0
               radius: 2
               clip: true

                ListView {
                    id: hostListView
                    anchors.fill: parent
                    orientation: ListView.Horizontal
                    spacing: 15
                    clip: true

                    // 🔥 关键配置：启用鼠标拖动
                    interactive: true
                    boundsBehavior: Flickable.DragAndOvershootBounds
                    flickDeceleration: 2000
                    highlightMoveDuration: 0

                    // 直接使用 connectedHostList 作为模型
                    model: hostManager.connectedHostList

                    delegate: Item {
                        width: 210
                        height: ListView.view.height  // 使用 ListView 的高度

                        // 🔥 使用最可靠的方式获取地址
                        property int addressValue: {
                            // 方法 2: 尝试 model
                            if (typeof model !== 'undefined' && model !== null) {
                                if (typeof model === 'number') {
                                    return model
                                }
                                // 如果 model 是对象
                                if (typeof model === 'object') {
                                    if (model.modelData !== undefined) {
                                        return model.modelData
                                    }
                                    if (model.address !== undefined) {
                                        return model.address
                                    }
                                }
                            }

                            console.warn("无法获取地址值，使用默认值 0")
                            return 0
                        }

                        // 创建 CustomHostUnit 并传递地址
                        CustomHostUnit {
                            hostAddress: addressValue

                            // 传递选中状态
                            isSelected: host_detecting_page.selectedHostAddress === addressValue


                            // 处理选中信号
                            onSelected: function(address) {
                                // 如果点击的是当前选中的主机，则取消选中；否则选中新的主机
                                if (host_detecting_page.selectedHostAddress === address) {
                                    console.log("取消选中")
                                    host_detecting_page.selectedHostAddress = 0  // 取消选中
                                } else {
                                    console.log("选中新主机，地址: 0x" + address.toString(16).toUpperCase())
                                    host_detecting_page.selectedHostAddress = address  // 选中新主机
                                }
                            }
                        }
                    }

                    // 空状态提示
                    Text {
                        text: "暂无主机，请点击下方按钮检测"
                        font.pixelSize: 12
                        color: Theme.text
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 15
                        visible: hostListView.count === 0
                        opacity: 0.3
                    }

                    // 🔥 滚动条 - 必须直接附加到 ListView
                    ScrollBar.horizontal: ScrollBar {
                        id: scrollBar
                        active: true
                        policy: ScrollBar.AsNeeded
                        size: hostListView.width / hostListView.contentWidth
                        opacity: 0.5

                        // 可选：自定义滚动条样式
                        contentItem: Rectangle {
                            color: Theme.mProgressBar
                            radius: 2
                            opacity: 0.8
                        }
                    }
                }
            }

            RowLayout {
                spacing: 20

                // 检测按钮
                CustomButton {
                    id: sendDetectingCommand
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 28

                    backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                    hoverColor: Theme.textMenu
                    pressedColor: Qt.darker(Theme.textMenu, 1.3)

                    text: {
                        if (!connectionFactory.isConnected) return "端口未连接"
                        if (isLoading) return "检测中..."
                        if (hasAnyHostInstance) return "主机工作中"
                        return "检测主机"
                    }

                    property bool isLoading: false
                    property bool hasAnyHostInstance: {
                        var list = hostManager.connectedHostList
                        for (var i = 0; i < list.length; i++) {
                            var info = hostManager.getAllHostMap(list[i])
                            if (info.isReady || info.isActive) return true
                        }
                        return false
                    }

                    enabled: connectionFactory.isConnected && !isLoading && !hasAnyHostInstance

                    Timer {
                        id: resetLoadingTimer
                        interval: 3000
                        onTriggered: sendDetectingCommand.isLoading = false
                    }

                    onClicked: {
                        if (!connectionFactory.isConnected) {
                            console.log("请先连接适配器")
                            return
                        }
                        if (hasAnyHostInstance) {
                            console.log("有主机正在工作，无法探测")
                            return
                        }
                        isLoading = true
                        var detected = hostManager.detectHosts(4)
                        //console.log("探测完成，发现 " + detected.length + " 个主机")
                        resetLoadingTimer.start()
                    }
                }

                CustomButton {
                    id: queryBasicProperties
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 28

                    text: "查询主机信息"

                    // 检查是否有选中的主机，并且选中主机存在实例，即Ready状态
                    enabled: {
                        if (selectedHostAddress <= 0) return false

                        // 获取主机信息，检查是否为 Ready 状态
                        var hostInfo = hostManager.getAllHostMap(selectedHostAddress)
                        return hostInfo && (hostInfo.isReady || hostInfo.isActive)
                    }

                    onClicked: {
                        if (selectedHostAddress <= 0) {
                            console.log("请先在上方列表中选择一个主机")
                            return
                        }

                        console.log("=== 开始查询主机基本属性 ===")
                        console.log("目标主机地址：0x" + selectedHostAddress.toString(16).toUpperCase().padStart(2, '0'))

                        // 🔥 获取选中主机的控制器并调用初始化函数
                        var controller = hostManager.getController(selectedHostAddress)

                        if (controller) {
                            console.log("找到主机控制器，开始发送请求...")

                            // 🔥 调用 getBasicProperties() 发送 6 个请求
                            // 这会触发以下查询：
                            // - CMD 0x0202, subCmd 0x00 → 内核版本
                            // - CMD 0x0202, subCmd 0x01 → Link 版本
                            // - CMD 0x0202, subCmd 0x02 → Anta 版本
                            // - CMD 0x0203, subCmd 0x00 → 内核日期
                            // - CMD 0x0203, subCmd 0x01 → Link 日期
                            // - CMD 0x0203, subCmd 0x02 → Anta 日期
                            controller.getHardwareInfo()

                            console.log("✅ 已发送 6 个查询请求，等待硬件响应...")
                        } else {
                            console.log("❌ 错误：未找到主机控制器（地址：0x" + selectedHostAddress.toString(16).toUpperCase() + "）")
                            console.log("提示：请先创建主机实例（点击'检测主机'后自动创建）")
                        }
                    }
                }

                CustomButton {
                    id: resetAllModels
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 28

                    text: confirmMode ? "确认移除？" : "重置所有主机"

                    textColor: confirmMode ? Theme.warning : Theme.mText

                    // 检查是否有主机实例存在
                    property bool hasAnyHostInstance: {
                        var list = hostManager.connectedHostList
                        for (var i = 0; i < list.length; i++) {
                            var info = hostManager.getAllHostMap(list[i])
                            if (info.isReady || info.isActive) return true
                        }
                        return false
                    }

                    // 确认模式标志
                    property bool confirmMode: false

                    // 只在有主机实例时启用
                    enabled: hasAnyHostInstance

                    // 重置确认模式的定时器
                    Timer {
                        id: resetConfirmTimer
                        interval: 3000  // 3 秒后自动取消确认模式
                        onTriggered: {
                            resetAllModels.confirmMode = false
                            console.log("确认超时，已取消重置操作")
                        }
                    }

                    onClicked: {
                        if (!confirmMode) {
                            // 第一次点击：进入确认模式
                            confirmMode = true
                            resetConfirmTimer.start()
                            console.log("请再次点击确认移除所有主机")
                        } else {
                            // 第二次点击：执行移除命令
                            console.log("执行移除所有主机操作")

                            // 清除选中的主机
                            host_detecting_page.selectedHostAddress = 0

                            // 调用移除所有主机的函数
                            hostManager.removeAllHosts()

                            // 重置按钮状态
                            confirmMode = false
                            resetConfirmTimer.stop()

                            console.log("所有主机已移除")
                        }
                    }

                    // 监听主机列表变化，当所有主机被移除后自动禁用按钮
                    Connections {
                        target: hostManager
                        function onHostInfoChanged(address) {
                            // 如果按钮处于确认模式且有主机被移除，退出确认模式
                            if (resetAllModels.confirmMode && !resetAllModels.hasAnyHostInstance) {
                                resetAllModels.confirmMode = false
                                resetConfirmTimer.stop()
                            }
                        }
                    }
                }
            }
        }
    }
    // 分隔线
    Rectangle {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 0.5
        color: Theme.borderFilled
        opacity: 0.7
    }

    // 主机属性区域（显示活动主机的详细信息）
    Item {
        id: hostInformation
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true

        Loader {
            id: propertyLoader
            anchors.fill: parent
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            anchors.topMargin: 10
            anchors.bottomMargin: 40

            source: {
                if (selectedHostAddress <= 0) return ""

                var hostInfo = hostManager.getAllHostMap(selectedHostAddress)
                var deviceType = hostInfo ? hostInfo.deviceType : 0

                // 直接返回 QML 文件路径
                if (deviceType === 1) return "host_info_xdc.qml"
                if (deviceType === 2) return "host_info_ds.qml"
                return ""
            }

            onStatusChanged: {
                if (status === Loader.Ready && item) {
                    // 加载完成后设置主机地址
                    item.hostAddress = selectedHostAddress
                }
            }
        }

        // 空状态
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: propertyLoader.source === "" || propertyLoader.status !== Loader.Ready

            Text {
                anchors.centerIn: parent
                text: "请点击主机选择查看详细信息"
                color: Theme.text
                opacity: 0.5
                font.pixelSize: 14
            }
        }
    }


    // ========== 信号连接 ==========
    Connections {
        target: hostManager
        function onHostInfoChanged(address) {
            // console.log("主机信息更新，地址: 0x" + address.toString(16).toUpperCase())
            // 如果更新的是当前选中的主机，强制刷新Loader
            if (address === selectedHostAddress || address === 0) {
                // 重新加载属性组件以刷新显示
                var temp = selectedHostAddress
                selectedHostAddress = 0
                selectedHostAddress = temp
            }
        }
    }

    // 监听主机列表变化，如果选中的主机被移除，清除选中状态
    Connections {
        target: hostManager
        function onConnectedHostListChanged() {
            if (selectedHostAddress > 0) {
                var list = hostManager.connectedHostList
                var found = false
                for (var i = 0; i < list.length; i++) {
                    if (list[i] === selectedHostAddress) {
                        found = true
                        break
                    }
                }
                if (!found) {
                    selectedHostAddress = 0
                }
            }
        }
    }

    Component.onCompleted: {
        // console.log("页面初始化完成")
        // console.log("当前主机列表:", hostManager.connectedHostList)
    }
}
