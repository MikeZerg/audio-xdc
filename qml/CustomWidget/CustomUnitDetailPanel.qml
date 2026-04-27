// qml/CustomWidget/CustomUnitDetailPanel.qml
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


Popup {
    id: root

    // 定义数据模型，单元Id
    property var unitModel
    property int unitId: 0
    
    // 使用动态属性存储单元数据，确保实时更新
    property var _unitDataCache: ({})
    property var unitData: _unitDataCache

    // 刷新单元数据函数
    function refreshUnitData() {
        if (unitModel && unitId !== 0) {
            _unitDataCache = unitModel.getUnitInfo(unitId)
            var hexId = "0x" + unitId.toString(16).toUpperCase().padStart(4, '0')
            console.debug("CustomUnitDetailPanel: 刷新单元数据 - ID:", hexId)
        } else {
            _unitDataCache = {}
        }
    }

    // 单元ID变化时执行
    onUnitIdChanged: {
        refreshUnitData()
        if (unitModel && unitId !== 0) {
            unitModel.refreshWirelessState(unitId)
        }
    }

    // 单元数据变化时执行
    onUnitModelChanged: {
        refreshUnitData()
    }

    // 监听单元数据变化信号
    Connections {
        target: unitModel
        enabled: unitModel !== null && unitId !== 0

        function onUnitDataChanged(changedUnitId) {
            if (changedUnitId === unitId) {
                console.debug("CustomUnitDetailPanel: 检测到单元数据变化，刷新显示")
                refreshUnitData()
            }
        }

        function onUnitListChanged() {
            if (unitId !== 0) {
                refreshUnitData()
            }
        }
    }

    // 面板尺寸设计
    width: parent.width * 0.4
    height: parent.height
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: parent.width
    y: (parent.height - height) / 2
    padding: 0
    modal: true

    // 从右往左展开面板动画
    enter: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width + 15 //初始位置右偏移15
            to: parent.width + 15 - root.width
            easing.type: Easing.OutCubic
            duration: 350
        }
    }
    // 从左往右折叠面板动画
    exit: Transition {
        NumberAnimation {
            property: "x"
            from: parent.width+15 - root.width
            to: parent.width+15
            easing.type: Easing.InCubic
            duration: 200
        }
    }
    // 设置面板背景色
    background: Rectangle {     // 设置面板背景色
        color: Theme.background
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 15
        clip: true
        opacity: 0.8
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            id: contentColumn
            width: scrollView.availableWidth
            spacing: 16

            // Popup页面标题栏
            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "单元详情 - " + (unitData.unitIdHex || "未知")
                    font.bold: true
                    font.pixelSize: 16
                    font.family: Theme.fontFamily
                    color: Theme.text
                }

                Item { Layout.fillWidth: true }

                // 页面关闭按钮
                CustomButton {
                    text: "✕"
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    Layout.rightMargin: 4
                    backgroundColor: "transparent"
                    hoverColor: Theme.surfaceSelected
                    pressedColor: Theme.surfaceSelected
                    borderColor: "transparent"
                    onClicked: root.close()
                }
            }

            Rectangle {
                height: 1
                Layout.fillWidth: true
                color: Theme.borderLine
            }

            // ========== 基本信息区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "基本信息"
                    font.bold: true
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                    color: Theme.text
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: basicInfoGrid.implicitHeight + 24
                    color: Theme.panel
                    border.color: Theme.borderLine
                    border.width: 0.5
                    radius: 4

                    GridLayout {
                        id: basicInfoGrid
                        anchors.fill: parent
                        anchors.margins: 12
                        columnSpacing: 16
                        rowSpacing: 8
                        columns: 2

                        Label {
                            text: "单元ID:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.unitIdHex || "--"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }

                        Label {
                            text: "物理地址:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.physicalAddr || "--"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }

                        Label {
                            text: "类型:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.typeName || "--"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }

                        Label {
                            text: "别名:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        RowLayout {
                            CustomTextField {
                                id: aliasInput
                                text: unitData.alias || ""
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 26
                            }
                            CustomButton {
                                text: "保存"
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 26
                                onClicked: {
                                    if (unitModel && unitId !== 0) {
                                        unitModel.setUnitAlias(unitId, aliasInput.text)
                                    }
                                }
                            }
                        }

                        Label {
                            text: "身份:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        RowLayout {
                            CustomCombox {
                                id: identityCombo
                                model: ["代表", "VIP", "主席"]
                                currentIndex: {
                                    if (unitData.identity === 0x20) return 1
                                    if (unitData.identity === 0x30) return 2
                                    return 0
                                }
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 26
                            }
                            CustomButton {
                                text: "保存"
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 26
                                onClicked: {
                                    if (unitModel && unitId !== 0) {
                                        var identity = [0x10, 0x20, 0x30][identityCombo.currentIndex]
                                        unitModel.setUnitIdentity(unitId, identity)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ========== 状态区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "状态"
                    font.bold: true
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                    color: Theme.text
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusGrid.implicitHeight + 24
                    color: Theme.panel
                    border.color: Theme.borderLine
                    border.width: 0.5
                    radius: 4

                    GridLayout {
                        id: statusGrid
                        anchors.fill: parent
                        anchors.margins: 12
                        columns: 3
                        rowSpacing: 8
                        columnSpacing: 16

                        Label {
                            text: "在线状态:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.isOnline ? "在线" : "离线"
                            color: unitData.isOnline ? Theme.success : Theme.error
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: "发言状态:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.isSpeaking ? "发言中" : (unitData.isOpened ? "已开启" : "已注册")
                            color: unitData.isSpeaking ? Theme.warning : (unitData.isOpened ? Theme.success : Theme.text)
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: "最后通信:"
                            font.bold: true
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Label {
                            text: unitData.lastSeenTime || "--"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // ========== 电池与信号区域（无线单元专属） ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: !!unitData.isWireless

                Text {
                    text: "电池与信号"
                    font.bold: true
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                    color: Theme.text
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: batteryColumn.implicitHeight + 24
                    color: Theme.panel
                    border.color: Theme.borderLine
                    border.width: 0.5
                    radius: Theme.radiusL

                    ColumnLayout {
                        id: batteryColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12


                        // 电池电压值，充电状态，无线信号强度
                        GridLayout {
                            columns: 2
                            rowSpacing: 8
                            columnSpacing: 16
                            // 电压值
                            Label {
                                text: "电池电压:"
                                font.bold: true
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                            Label {
                                text: (unitData.batteryVoltage || 0) + " mV"
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                            // 充电状态
                            Label {
                                text: "充电状态:"
                                font.bold: true
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                            Label {
                                text: unitData.chargingStatusText || "未知"
                                color: unitData.chargingStatus === 1 ? Theme.success : Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }

                            // 电池电量标签
                            Label {
                                text: "电池电量:"
                                font.bold: true
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                            // 电池图标
                            RowLayout {
                                spacing: 10
                                // 电池图形
                                Item {
                                    Layout.preferredWidth: 24
                                    Layout.preferredHeight: 13
                                    Layout.alignment: Qt.AlignVCenter

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 3
                                        border.color: {
                                            if (unitData.isLowBattery) return Theme.error
                                            if (unitData.batteryPercent > 50) return Theme.success
                                            return Theme.warning
                                        }
                                        border.width: 1.2
                                        color: "transparent"

                                        // 电池头
                                        Rectangle {
                                            width: 3
                                            height: 4
                                            radius: 0.7
                                            color: {
                                                if (unitData.isLowBattery) return Theme.error
                                                if (unitData.batteryPercent > 50) return Theme.success
                                                return Theme.warning
                                            }
                                            anchors {
                                                right: parent.right
                                                rightMargin: -3
                                                verticalCenter: parent.verticalCenter
                                            }
                                        }

                                        // 电池电量填充
                                        Rectangle {
                                            width: (parent.width - 6) * (unitData.batteryPercent / 100)
                                            height: parent.height - 6
                                            radius: 1.5
                                            color: {
                                                if (unitData.isLowBattery) return Theme.error
                                                if (unitData.batteryPercent > 50) return Theme.success
                                                return Theme.warning
                                            }
                                            anchors {
                                                left: parent.left
                                                leftMargin: 3
                                                verticalCenter: parent.verticalCenter
                                            }
                                        }

                                        // 电池充电图标
                                        Text {
                                            id:inChargeIcon
                                            anchors.centerIn: parent
                                            text: String.fromCharCode(126)
                                            font.family: "Webdings"
                                            font.pixelSize: 11
                                            color: Theme.warning
                                            visible:  unitData.chargingStatus === 1
                                        }
                                    }
                                }
                                // 电池百分比文本
                                Label {
                                    text: (unitData.batteryPercent || 0) + " %"
                                    color: unitData.isLowBattery ? Theme.error : Theme.text
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 11
                                }
                            }
                            // 无线信号强度
                            Label {
                                text: "信号强度:"
                                font.bold: true
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                            RowLayout {
                                spacing: 10
                                // 信号图标
                                Image {
                                    source: {
                                        var rssi = unitData.rssi || -100
                                        var level = Math.floor((rssi + 100) / 12)
                                        level = Math.max(0, Math.min(5, level))
                                        switch(level) {
                                            case 5: return "qrc:/image/app-signal5.svg"
                                            case 4: return "qrc:/image/app-signal4.svg"
                                            case 3: return "qrc:/image/app-signal3.svg"
                                            case 2: return "qrc:/image/app-signal2.svg"
                                            default: return "qrc:/image/app-signal1.svg"
                                        }
                                    }
                                    sourceSize.width: 24
                                    sourceSize.height: 13
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 14

                                }

                                // 信号强度文本
                                Label {
                                    text: (unitData.rssi || 0) + " dBm"
                                    color: Theme.text
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 11
                                }
                            }
                        }

                        CustomButton {
                            text: "刷新状态"
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 26
                            onClicked: {
                                if (unitModel && unitId !== 0) {
                                    unitModel.refreshWirelessState(unitId)
                                }
                            }
                        }
                    }
                }
            }

            // ========== 单元控制区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "单元控制"
                    font.bold: true
                    font.pixelSize: 11
                    font.family: Theme.fontFamily
                    color: Theme.text
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 56
                    color: Theme.panel
                    border.color: Theme.borderLine
                    border.width: 0.5
                    radius: Theme.radiusL

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        CustomButton {
                            text: unitData.isOpened ? "关闭发言" : "打开发言"
                            enabled: !!unitData.isOnline
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 26
                            backgroundColor: unitData.isOpened ? Theme.surface : Qt.darker(Theme.textMenu, 1.1)
                            hoverColor: Theme.textMenu
                            pressedColor: Qt.darker(Theme.textMenu, 1.3)

                            onClicked: {
                                if (unitData.isOpened)
                                    unitModel.closeUnit(unitId)
                                else
                                    unitModel.openUnit(unitId)
                            }
                        }

                        CustomButton {
                            text: "删除单元"
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 26
                            backgroundColor: Theme.surface
                            hoverColor: Theme.surfaceSelected
                            pressedColor: Theme.surfaceSelected
                            borderColor: Theme.error
                            textColor: Theme.error
                            onClicked: {
                                unitModel.deleteUnit(unitId)
                                root.close()
                            }
                        }

                        Item { Layout.fillWidth: true }

                    }
                }
            }

            // 底部留白
            Item { Layout.preferredHeight: 20 }
        }
    }
}
