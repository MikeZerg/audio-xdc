// host_speech_status_xdc.qml - XDC236 主机单元状态显示组件
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Audio.Hardware 1.0
import "./CustomWidget"

ColumnLayout {
    id: xdcSpeechStatus
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // ==================== 控制器与模型管理 ====================

    // 当前活动主机地址
    property int currentHostAddress: 0

    // 当前主机的控制器对象
    property var currentController: null

    // 单元数据模型（从控制器获取）
    property var unitModel: currentController ? currentController.unitModel : null

    // 发言设置对象（用于绑定发言模式和人数）
    property var speechSettings: currentController ? currentController.viewSpeechSettings : null

    // 辅助属性：强制刷新界面显示
    property bool forceUpdate: false

    // ==================== 单元数据模型 ====================

    // 所有注册单元的数据列表
    property var allUnitsModel: []

    // 发言单元列表（正在发言的单元 + 主席单元）
    property var speakingUnitsModel: []

    // ========== 更新所有单元列表 ==========
    function updateAllUnitsModel() {
        var list = []
        if (!unitModel) {
            allUnitsModel = list
            return
        }
        var ids = unitModel.getUnitIds()
        for (var i = 0; i < ids.length; ++i) {
            var item = unitModel.getUnitInfo(ids[i])
            list.push(item)
        }
        allUnitsModel = list
        console.debug("更新所有单元模型，数量:", allUnitsModel.length)
        // 同步更新发言单元列表
        updateSpeakingUnitsModel()
    }

    // ========== 更新发言单元列表（正在发言的单元 + 主席单元） ==========
    function updateSpeakingUnitsModel() {
        var list = []
        if (!unitModel) {
            speakingUnitsModel = list
            return
        }
        var ids = unitModel.getUnitIds()
        for (var i = 0; i < ids.length; ++i) {
            var item = unitModel.getUnitInfo(ids[i])
            // 正在发言 或者 主席单元（身份为0x30表示主席）
            if (item.isSpeaking === true || item.identity === 0x30) {
                list.push(item)
            }
        }
        // 按发言状态排序：正在发言的排在前面
        list.sort(function(a, b) {
            if (a.isSpeaking === b.isSpeaking) return 0
            if (a.isSpeaking) return -1
            return 1
        })
        speakingUnitsModel = list
        console.debug("更新发言单元模型，数量:", speakingUnitsModel.length)
    }

    // ========== 刷新所有模型 ==========
    function refreshModels() {
        updateAllUnitsModel()
    }

    // ==================== 控制器管理函数 ====================

    // ========== 初始化当前活动主机 ==========
    function initCurrentHost() {
        if (!hostManager) {
            console.debug("XdcSpeechStatus: hostManager 为空")
            return
        }

        var addrStr = hostManager.activeHostAddress
        console.debug("XdcSpeechStatus: 当前活动主机地址:", addrStr)

        if (addrStr && addrStr !== "") {
            var addr = parseInt(addrStr)
            if (!isNaN(addr) && addr > 0) {
                currentHostAddress = addr
                refreshController()
            }
        } else {
            currentHostAddress = 0
            currentController = null
            unitModel = null
            refreshModels()
        }

        forceUpdate = !forceUpdate
    }

    // ========== 刷新控制器 ==========
    function refreshController() {
        if (currentHostAddress > 0 && hostManager) {
            var ctrl = hostManager.getController(currentHostAddress)
            console.debug("XdcSpeechStatus: 刷新控制器 - 地址:",
                         currentHostAddress,
                         "控制器:", ctrl ? "有效" : "无效")

            if (ctrl !== currentController) {
                currentController = ctrl
                unitModel = ctrl ? ctrl.unitModel : null
                speechSettings = ctrl ? ctrl.viewSpeechSettings : null
                refreshModels()
                forceUpdate = !forceUpdate
            }
        } else {
            if (currentController !== null) {
                currentController = null
                unitModel = null
                speechSettings = null
                refreshModels()
                forceUpdate = !forceUpdate
            }
        }
    }

    // ==================== 统计信息属性 ====================

    // 注册单元总数
    property int registeredCount: unitModel ? unitModel.unitCount : 0

    // 当前发言单元数
    property int speakingCount: unitModel ? unitModel.speakingCount : 0

    // 发言模式文本（先进先出 / 自锁模式）
    property string speechModeText: {
        if (!speechSettings || speechSettings.speechMode === undefined) return "未知"
        var mode = speechSettings.speechMode
        var dummy = xdcSpeechStatus.forceUpdate  // 触发强制刷新
        return mode === 0 ? "先进先出" : "自锁模式"
    }

    // 最大发言人数
    property int maxSpeechCountValue: {
        if (!speechSettings || speechSettings.maxSpeechCount === undefined) return 0
        var dummy = xdcSpeechStatus.forceUpdate  // 触发强制刷新
        return speechSettings.maxSpeechCount
    }

    // ==================== 布局计算属性 ====================
    // 发言单元数量
    property int speakingUnitCount: speakingUnitsModel.length

    // 每个单元卡片的高度
    property int cardHeight: 80

    // 容器内边距和间距
    property int containerPadding: 20
    property int rowSpacing: 8

    // 计算需要的行数：每行2个单元，最多2行（只显示前4个）
    property int rowsNeeded: {
        if (speakingUnitCount === 0) return 0
        var rows = Math.ceil(Math.min(speakingUnitCount, 4) / 2)
        return Math.min(rows, 2)
    }

    // 计算发言区域高度
    property int speakingAreaHeight: {
        if (speakingUnitCount === 0) return 60  // 空状态高度
        return containerPadding + (rowsNeeded * cardHeight) + ((rowsNeeded - 1) * rowSpacing)
    }

    // ==================== 信号连接 ====================

    // 监听活动主机变化
    Connections {
        target: hostManager
        enabled: hostManager !== null

        function onActiveHostChanged(oldAddress, newAddress) {
            console.debug("XdcSpeechStatus: 活动主机变更",
                         "旧地址:", oldAddress,
                         "新地址:", newAddress)

            if (newAddress > 0) {
                currentHostAddress = newAddress
                refreshController()
            } else {
                currentHostAddress = 0
                currentController = null
                unitModel = null
                speechSettings = null
                refreshModels()
            }
            forceUpdate = !forceUpdate
        }

        function onHostInfoChanged(hostAddress) {
            if (hostAddress === currentHostAddress && currentHostAddress > 0) {
                refreshController()
            }
        }
    }

    // 监听 UnitModel 信号
    Connections {
        target: unitModel
        enabled: unitModel !== null

        function onUnitListChanged() {
            console.debug("XdcSpeechStatus: 单元列表变化")
            refreshModels()
            forceUpdate = !forceUpdate
        }

        function onUnitDataChanged(unitId) {
            console.debug("XdcSpeechStatus: 单元数据变化 - ID:", unitId)
            refreshModels()
            forceUpdate = !forceUpdate
        }
    }

    // 监听控制器发言设置变化信号
    Connections {
        target: currentController
        enabled: currentController !== null

        function onSpeechSettingsChanged() {
            console.debug("XdcSpeechStatus: 发言设置变化")
            forceUpdate = !forceUpdate
        }
    }

    // ==================== 组件初始化 ====================
    Component.onCompleted: {
        console.debug("XdcSpeechStatus: 组件初始化")
        initCurrentHost()
    }

    // ==================== 页面标题 ====================
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25
        Text {
            text: "XDC236 融合主机单元"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 20 }

    // ==================== 单元显示区域 ====================
    Rectangle {
        id: unitsContainer
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "transparent"
        Layout.leftMargin: 15
        Layout.rightMargin: 15

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            // 控制栏
            Rectangle {
                id: controlBar
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: Theme.panel
                border.color: Theme.borderLine
                border.width: 0.5
                radius: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 20

                    Label { text: "注册单元："; color: Theme.text; font.pixelSize: 12 }
                    Label { text: registeredCount; color: Theme.text; font.pixelSize: 12 }
                    Item { Layout.preferredWidth: 20 }
                    Label { text: "发言模式："; color: Theme.text; font.pixelSize: 12 }
                    Label { text: speechModeText; color: Theme.text; font.pixelSize: 12 }
                    Item { Layout.preferredWidth: 20 }
                    Label { text: "发言人数："; color: Theme.text; font.pixelSize: 12 }
                    Label { text: maxSpeechCountValue; color: Theme.text; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }

                    // 刷新单元列表图标按钮
                    Item {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28

                        Image {
                            id: refreshIcon
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: "qrc:/image/app-refreshUnit.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                if (currentController) {
                                    currentController.getAllUnits()
                                } else {
                                    refreshController()
                                    if (currentController) {
                                        currentController.getAllUnits()
                                    } else {
                                        console.debug("无活动主机,请先检测主机。")
                                    }
                                }
                            }

                            onEntered: {
                                refreshIcon.source = "qrc:/image/app-refreshUnithover.svg"
                            }

                            onExited: {
                                refreshIcon.source = "qrc:/image/app-refreshUnit.svg"
                            }
                        }

                        ToolTip {
                            delay: 500
                            text: "刷新单元列表"
                        }
                    }
                }
            }

            RowLayout {
                Layout.bottomMargin: 1
                Text {
                    text: "  主席/发言单元"
                    font.pixelSize: 11
                    color: Theme.text
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: speakingUnitsModel.length > 0 ?
                          "共 " + speakingUnitsModel.length + " 个单元" : ""
                    font.pixelSize: 10
                    color: Theme.textDim
                    visible: speakingUnitsModel.length > 0
                    opacity: 0.5
                }
            }

            // 发言单元区域（正在发言的单元 + 主席单元）- 网格布局
            Rectangle {
                id: speakingArea
                Layout.fillWidth: true
                Layout.preferredHeight: speakingAreaHeight
                color: Theme.panel
                border.color: Theme.borderLine
                border.width: 0.5
                radius: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    // 网格布局容器
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: speakingUnitsModel.length > 0

                        Grid {
                            id: speakingGrid
                            anchors.fill: parent
                            spacing: 8
                            columns: 2
                            rows: rowsNeeded
                            flow: Grid.TopToBottom

                            Repeater {
                                model: speakingUnitsModel.length > 4 ?
                                       speakingUnitsModel.slice(0, 4) : speakingUnitsModel

                                delegate: Item {
                                    width: (speakingGrid.width - 8) / 2
                                    height: cardHeight

                                    CustomUnitCard {
                                        width: parent.width
                                        height: parent.height
                                        unitData: modelData
                                        unitModel: xdcSpeechStatus.unitModel
                                        onShowDetail: {
                                            detailPanel.unitId = modelData.unitId
                                            detailPanel.open()
                                        }
                                    }
                                }
                            }
                        }

                        // 超过4个单元的提示
                        Rectangle {
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.topMargin: 5
                            anchors.rightMargin: 5
                            width: 60
                            height: 20
                            visible: speakingUnitsModel.length > 4
                            color: Theme.warning
                            radius: 10
                            Text {
                                anchors.centerIn: parent
                                text: "+" + (speakingUnitsModel.length - 4)
                                font.pixelSize: 10
                                color: "white"
                            }
                            ToolTip {
                                visible: parent.visible && parent.containsMouse
                                text: "还有 " + (speakingUnitsModel.length - 4) + " 个单元未显示"
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }

                    // 空状态提示
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: speakingUnitsModel.length === 0
                        color: "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "暂无发言单元和主席单元"
                            color: Theme.text
                            font.pixelSize: 11
                        }
                    }
                }
            }

            RowLayout {
                Layout.bottomMargin: 1
                Text {
                    text: "  全部注册单元"
                    font.pixelSize: 11
                    color: Theme.text
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: allUnitsModel.length > 0 ?
                          "共 " + allUnitsModel.length + " 个单元" : ""
                    font.pixelSize: 10
                    color: Theme.textDim
                    visible: allUnitsModel.length > 0
                    opacity: 0.5
                }
            }

            // 所有单元区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.panel
                border.color: Theme.borderLine
                border.width: 0.5
                radius: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    ListView {
                        id: allUnitsListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 5
                        model: xdcSpeechStatus.allUnitsModel

                        delegate: CustomUnitCard {
                            width: ListView.view.width
                            unitData: modelData
                            unitModel: xdcSpeechStatus.unitModel
                            onShowDetail: {
                                detailPanel.unitId = modelData.unitId
                                detailPanel.open()
                            }
                        }

                        // 空状态提示
                        Rectangle {
                            visible: allUnitsListView.count === 0
                            anchors.centerIn: parent
                            width: parent.width - 20
                            height: 60
                            color: "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "暂无单元"
                                color: Theme.text
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: 20 }
        }
    }

    // 单元详情面板
    CustomUnitDetailPanel {
        id: detailPanel
        unitModel: xdcSpeechStatus.unitModel
    }
}
