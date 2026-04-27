import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

ColumnLayout {
    id: host_detecting_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

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

            // 主机列表容器
            Rectangle {
                id: hostViewSection
                width: {
                    var containerWidth = parent.width - 60
                    var hostWidth = 210
                    var spacing = 10
                    var leftMargin = 10
                    var rightMargin = 10

                    var availableWidth = containerWidth - leftMargin - rightMargin
                    var hostTotalWidth = hostWidth + spacing
                    var maxHostCount = Math.floor((availableWidth + spacing) / hostTotalWidth)
                    maxHostCount = Math.max(4, maxHostCount)

                    var contentWidth = leftMargin +
                                     (hostWidth * maxHostCount) +
                                     (spacing * (maxHostCount - 1)) +
                                     rightMargin
                    return contentWidth
                }
                height: 140
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

                    interactive: true
                    boundsBehavior: Flickable.DragAndOvershootBounds
                    flickDeceleration: 2000
                    highlightMoveDuration: 0

                    model: hostManager.connectedHostList

                    delegate: Item {
                        width: 210
                        height: ListView.view.height

                        property int addressValue: {
                            if (typeof model !== 'undefined' && model !== null) {
                                if (typeof model === 'number') {
                                    return model
                                }
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

                            // 使用 selectedHostAddressInt 进行数字比较
                            isSelected: hostManager.selectedHostAddressInt === addressValue

                            onSelected: function(address) {
                                // console.log("主机卡片选中信号: 地址 0x" + address.toString(16))
                                if (hostManager.selectedHostAddressInt === address) {
                                    // console.log("取消选中主机")
                                    hostManager.selectHost(0)
                                } else {
                                    // console.log("选中新主机，地址: 0x" + address.toString(16).toUpperCase())
                                    var success = hostManager.selectHost(address)
                                    if (success) {
                                        console.log("选中成功，当前选中地址: 0x" +
                                                   hostManager.selectedHostAddressInt.toString(16))
                                    } else {
                                        console.log("选中失败")
                                    }
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

                    // 滚动条
                    ScrollBar.horizontal: ScrollBar {
                        id: scrollBar
                        active: true
                        policy: ScrollBar.AsNeeded
                        size: hostListView.width / hostListView.contentWidth
                        opacity: 0.5

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

                    backgroundColor: Qt.darker(Theme.textMenu, 1.1)
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
                            if (info.isReady) return true
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
                        console.log("开始探测主机，地址范围 1-4")
                        hostManager.detectHosts(4)
                        resetLoadingTimer.start()
                    }
                }

                // 查询主机信息按钮
                CustomButton {
                    id: queryBasicProperties
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 28

                    text: "查询主机信息"

                    // [修复] 使用 selectedHostAddressInt
                    enabled: {
                        var selectedAddr = hostManager.selectedHostAddressInt
                        if (selectedAddr <= 0) return false

                        var hostInfo = hostManager.getAllHostMap(selectedAddr)
                        return hostInfo && hostInfo.isReady
                    }

                    onClicked: {
                        var selectedAddr = hostManager.selectedHostAddressInt
                        if (selectedAddr <= 0) {
                            console.log("请先在上方列表中选择一个主机")
                            return
                        }

                        console.log("=== 开始查询主机基本属性 ===")
                        console.log("目标主机地址：0x" + selectedAddr.toString(16).toUpperCase())

                        var controller = hostManager.getController(selectedAddr)

                        if (controller) {
                            console.log("找到主机控制器，开始发送请求...")
                            controller.getHardwareInfo()
                            console.log("✅ 已发送查询请求，等待硬件响应...")
                        } else {
                            console.log("❌ 错误：未找到主机控制器（地址：0x" +
                                       selectedAddr.toString(16).toUpperCase() + "）")
                            console.log("提示：请先右键点击主机卡片，选择'设为就绪'创建实例")
                        }
                    }
                }

                // 重置所有主机按钮
                CustomButton {
                    id: resetAllModels
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 28

                    text: confirmMode ? "确认移除？" : "重置所有主机"
                    textColor: confirmMode ? Theme.warning : Theme.mText

                    property bool hasAnyHostInstance: {
                        var list = hostManager.connectedHostList
                        for (var i = 0; i < list.length; i++) {
                            var info = hostManager.getAllHostMap(list[i])
                            if (info.isReady) return true
                        }
                        return false
                    }

                    property bool confirmMode: false
                    enabled: hasAnyHostInstance

                    Timer {
                        id: resetConfirmTimer
                        interval: 3000
                        onTriggered: {
                            resetAllModels.confirmMode = false
                            console.log("确认超时，已取消重置操作")
                        }
                    }

                    onClicked: {
                        if (!confirmMode) {
                            confirmMode = true
                            resetConfirmTimer.start()
                            console.log("请再次点击确认移除所有主机")
                        } else {
                            console.log("执行移除所有主机操作")
                            hostManager.removeAllHosts()
                            confirmMode = false
                            resetConfirmTimer.stop()
                            console.log("所有主机已移除")
                        }
                    }

                    Connections {
                        target: hostManager
                        function onHostInfoChanged(address) {
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
                var selectedAddr = hostManager.selectedHostAddressInt
                if (selectedAddr <= 0) return ""

                var hostInfo = hostManager.getAllHostMap(selectedAddr)
                var deviceType = hostInfo ? hostInfo.deviceType : 0

                if (deviceType === 1) return "host_info_xdc.qml"
                if (deviceType === 2) return "host_info_ds.qml"
                return ""
            }

            onStatusChanged: {
                if (status === Loader.Ready && item) {
                    var selectedAddr = hostManager.selectedHostAddressInt
                    if (selectedAddr > 0) {
                        item.hostAddress = selectedAddr
                        console.log("Loader 加载完成，传递地址: 0x" + selectedAddr.toString(16))
                    }
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
                text: "请右键点击主机卡片选择「设为就绪」，然后左键选中查看详细信息"
                color: Theme.text
                opacity: 0.5
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // ========== 信号连接 ==========
    Connections {
        target: hostManager

        function onHostInfoChanged(address) {
            console.log("HostManager 通知: 主机信息已变更, 地址=0x" + address.toString(16))
            // 如果更新的是当前选中的主机，刷新Loader
            if (address === hostManager.selectedHostAddressInt || address === 0) {
                var temp = hostManager.selectedHostAddressInt
                if (temp > 0) {
                    // 重新加载属性组件以刷新显示
                    var currentSource = propertyLoader.source
                    propertyLoader.source = ""
                    propertyLoader.source = currentSource
                }
            }
        }

        function onSelectedHostChanged(oldAddr, newAddr) {
            console.log("选中主机变更: 0x" + oldAddr.toString(16) + " -> 0x" + newAddr.toString(16))
        }

        function onDetectedHostsResponseReceived(address) {
            console.log("探测到新主机: 0x" + address.toString(16))
        }
    }

    Component.onCompleted: {
        console.log("host_detecting 页面初始化完成")
        console.log("当前已连接主机列表:", hostManager.connectedHostList)
    }
}
