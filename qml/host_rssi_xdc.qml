// /qml/host_rssi_xdc.qml
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Audio.Hardware 1.0
import "./CustomWidget"

ColumnLayout {
    id: host_rssi_testing_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    property var controller: null
    // [新增] 当前选中的主机地址
    property int currentHostAddress: 0

    // 列宽配置 - 动态计算
    property var columnWidths: [39, 95, 75, 95, 75, 95, 75, 95, 75, 95, 75]

    // 固定第一列宽度
    property int fixedFirstColumnWidth: 39

    // 剩余10列的比例 (FREQ: 95, RSSI: 75, 交替)
    property var columnRatios: [95, 75, 95, 75, 95, 75, 95, 75, 95, 75]

    property var columnHeaders: [
        { key: "fid", title: "FID" },
        { key: "monitorFreqKHz", title: "Frequency" },
        { key: "monitorRSSI", title: "RSSI" },
        { key: "wireless0FreqKHz", title: "FREQ 0" },
        { key: "wireless0RSSI", title: "RSSI 0" },
        { key: "wireless1FreqKHz", title: "FREQ 1" },
        { key: "wireless1RSSI", title: "RSSI 1" },
        { key: "wireless2FreqKHz", title: "FREQ 2" },
        { key: "wireless2RSSI", title: "RSSI 2" },
        { key: "wireless3FreqKHz", title: "FREQ 3" },
        { key: "wireless3RSSI", title: "RSSI 3" }
    ]

    // 计算动态列宽
    function calculateColumnWidths() {
        if (!tableContainer) return

        var totalWidth = tableContainer.width
        if (totalWidth <= 0) return

        var remainingWidth = totalWidth - fixedFirstColumnWidth
        if (remainingWidth <= 0) return

        // 计算比例总和
        var ratioSum = 0
        for (var i = 0; i < columnRatios.length; i++) {
            ratioSum += columnRatios[i]
        }

        // 计算每列宽度
        var newWidths = [fixedFirstColumnWidth]
        for (var j = 0; j < columnRatios.length; j++) {
            var colWidth = remainingWidth * columnRatios[j] / ratioSum
            newWidths.push(colWidth)
        }

        columnWidths = newWidths
    }

    // [修复] 刷新控制器 - 使用 selectedHostAddressInt
    function refreshController() {
        if (typeof hostManager === 'undefined' || !hostManager) {
            controller = null;
            currentHostAddress = 0;
            return;
        }
        // 使用新的 selectedHostAddressInt 替代 activeHostAddress
        var addr = hostManager.selectedHostAddressInt;
        currentHostAddress = addr;
        if (addr > 0) {
            controller = hostManager.getController(addr);
            console.log("RSSI页面: 获取控制器, 地址=0x" + addr.toString(16));
        } else {
            controller = null;
            console.log("RSSI页面: 没有选中的主机");
        }
    }

    function refreshTableData() {
        if (!controller) {
            tableModel.clear();
            return;
        }

        var freqPoints = controller.getAllFreqPoints();
        if (!freqPoints || freqPoints.length === 0) {
            tableModel.clear();
            return;
        }

        tableModel.clear();
        for (var i = 0; i < freqPoints.length; i++) {
            var point = freqPoints[i]
            var normalizedPoint = {
                fid: point.fid !== undefined ? point.fid : i,
                monitorFreqKHz: point.monitorFreqKHz !== undefined ? point.monitorFreqKHz : "0",
                monitorRSSI: point.monitorRSSI !== undefined ? point.monitorRSSI : 0,
                wireless0FreqKHz: point.wireless0FreqKHz !== undefined ? point.wireless0FreqKHz : "0",
                wireless0RSSI: point.wireless0RSSI !== undefined ? point.wireless0RSSI : 0,
                wireless1FreqKHz: point.wireless1FreqKHz !== undefined ? point.wireless1FreqKHz : "0",
                wireless1RSSI: point.wireless1RSSI !== undefined ? point.wireless1RSSI : 0,
                wireless2FreqKHz: point.wireless2FreqKHz !== undefined ? point.wireless2FreqKHz : "0",
                wireless2RSSI: point.wireless2RSSI !== undefined ? point.wireless2RSSI : 0,
                wireless3FreqKHz: point.wireless3FreqKHz !== undefined ? point.wireless3FreqKHz : "0",
                wireless3RSSI: point.wireless3RSSI !== undefined ? point.wireless3RSSI : 0
            }
            tableModel.append(normalizedPoint);
        }
    }

    function updateFreqPoint(fid) {
        if (!controller) return
        var point = controller.getFreqPoint(fid)
        if (point && fid >= 0 && fid < tableModel.count) {
            var normalizedPoint = {
                fid: point.fid !== undefined ? point.fid : fid,
                monitorFreqKHz: point.monitorFreqKHz !== undefined ? point.monitorFreqKHz : "0",
                monitorRSSI: point.monitorRSSI !== undefined ? point.monitorRSSI : 0,
                wireless0FreqKHz: point.wireless0FreqKHz !== undefined ? point.wireless0FreqKHz : "0",
                wireless0RSSI: point.wireless0RSSI !== undefined ? point.wireless0RSSI : 0,
                wireless1FreqKHz: point.wireless1FreqKHz !== undefined ? point.wireless1FreqKHz : "0",
                wireless1RSSI: point.wireless1RSSI !== undefined ? point.wireless1RSSI : 0,
                wireless2FreqKHz: point.wireless2FreqKHz !== undefined ? point.wireless2FreqKHz : "0",
                wireless2RSSI: point.wireless2RSSI !== undefined ? point.wireless2RSSI : 0,
                wireless3FreqKHz: point.wireless3FreqKHz !== undefined ? point.wireless3FreqKHz : "0",
                wireless3RSSI: point.wireless3RSSI !== undefined ? point.wireless3RSSI : 0
            }
            tableModel.set(fid, normalizedPoint)
        }
    }

    Connections {
        target: controller
        enabled: controller !== null

        function onAudioTestProgress(step, current, total, message) {
            if (step === 1) {
                statusLabel.text = "状态: 获取信道数量... (" + current + "/" + total + ")"
            } else if (step === 2) {
                statusLabel.text = "状态: 获取监测信道... (" + current + "/" + total + ")"
            } else if (step === 3) {
                statusLabel.text = "状态: 测试无线信道... (" + current + "/" + total + ")"
                progressBar.value = current / total
            }
        }

        function onAudioTestStepChanged(step) {
            progressBar.visible = (step === "testing_wireless_channels")
        }

        function onAudioTestCompleted(success, message) {
            if (success) {
                statusLabel.text = "状态: 测试完成 (" + controller.freqPointCount + "个频率点)"
                refreshTableData()
            } else {
                statusLabel.text = "状态: 测试失败 - " + message
            }
            progressBar.visible = false
            progressBar.value = 0
        }

        function onFreqPointUpdated(fid) {
            updateFreqPoint(fid)
        }

        function onFreqPointCountChanged() {
            refreshTableData()
        }

        function onUnifiedTableModelChanged() {
            refreshTableData()
        }
    }

    // [修复] 将旧的 onActiveHostAddressChanged 改为 onSelectedHostChanged
    Connections {
        target: typeof hostManager !== 'undefined' ? hostManager : null
        enabled: typeof hostManager !== 'undefined'

        // 使用新的信号名称 selectedHostChanged
        function onSelectedHostChanged(oldAddress, newAddress) {
            console.log("RSSI页面: 选中主机变更 0x" + oldAddress.toString(16) + " -> 0x" + newAddress.toString(16))
            refreshController()
            refreshTableData()
        }

        // 监听主机信息变化，当主机状态更新时刷新
        function onHostInfoChanged(address) {
            if (address === currentHostAddress || address === 0) {
                refreshTableData()
            }
        }
    }

    Component.onCompleted: {
        console.log("RSSI页面初始化完成")
        refreshController()
        refreshTableData()
        calculateColumnWidths()
    }

    // 监听容器宽度变化
    onWidthChanged: {
        calculateColumnWidths()
    }

    ListModel { id: tableModel }

    // ========== UI ==========
    // 页面标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25
        Text {
            text: "测试 “监控信道 / 无线信道” 信号强度"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredWidth: parent.width; Layout.preferredHeight: 40 }

    // 表格容器（表头，表主体）
    Rectangle {
        id: tableContainer
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "transparent"
        Layout.leftMargin: 15
        Layout.rightMargin: 15

        // 监听容器宽度变化
        onWidthChanged: {
            calculateColumnWidths()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            // 控制栏
            Rectangle {
                id: controlBar
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: Theme.panel
                border.color: Theme.borderLine
                border.width: 0.5

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15

                    CustomButton {
                        text: "开始测试"
                        font.pixelSize: Theme.fontSize12
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 100
                        borderWidth: 0.5
                        enabled: controller !== null
                        onClicked: {
                            if (controller) {
                                statusLabel.text = "状态: 正在启动测试..."
                                progressBar.visible = true
                                controller.startAudioTest()
                            } else {
                                statusLabel.text = "状态: 请先选中一个已就绪的主机"
                            }
                        }
                    }

                    CustomButton {
                        text: "停止测试"
                        font.pixelSize: Theme.fontSize12
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 100
                        borderWidth: 0.5
                        enabled: controller !== null
                        onClicked: {
                            if (controller) controller.stopAudioTest()
                        }
                    }

                    CustomSpin {
                        id: delaySpin
                        from: 500
                        to: 5000
                        step: 100
                        value: 2000
                        editable: true
                        Layout.preferredWidth: 160
                        leftBorderWidth: 0.5
                        centerBorderWidth: 0.5
                        rightBorderWidth: 0.5
                        onValueModified: {
                            if (controller) controller.setStabilizeDelayMs(value)
                        }
                    }

                    Item { Layout.fillWidth: true }

                    ProgressBar {
                        id: progressBar
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 13
                        visible: false
                        value: 0
                        opacity: 0.5
                    }

                    Label {
                        id: statusLabel
                        text: controller !== null ? "状态: 等待测试" : "状态: 请先选中一个主机"
                        color: Theme.text
                        font.pixelSize: 11
                        font.weight: 11
                    }
                }
            }

            // 表头
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.tableBackground
                border.color: Theme.borderLine
                border.width: 0.5

                ScrollView {
                    anchors.fill: parent
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                    Row {
                        height: parent.height
                        Repeater {
                            model: columnHeaders
                            delegate: Rectangle {
                                width: columnWidths[index]
                                height: parent.height
                                color: Theme.tableBackground
                                border.color: Theme.borderLine
                                border.width: 0.5
                                Text {
                                    text: modelData.title
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textMenu
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            // 数据行
            ListView {
                id: dataListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: tableModel

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // 设置RSSI字体颜色
                function getRssiColor(val) {
                    if (val === "--" || val === undefined || val === 0) return Theme.textDim
                    var num = parseInt(val)
                    if (isNaN(num)) return Theme.text
                    if (num > -70) return Theme.tableFontColorHig
                    if (num > -90) return Theme.tableFontColorMid
                    return Theme.tableFontColorLow
                }

                function formatDisplay(columnKey, val) {
                    if (val === "--" || val === undefined || val === 0) return "--"
                    if (columnKey === "fid") return val
                    if (columnKey.indexOf("Freq") >= 0 || columnKey === "monitorFreqKHz") {
                        return val + " KHz"
                    }
                    if (columnKey.indexOf("RSSI") >= 0 || columnKey === "monitorRSSI") {
                        return val + " dBm"
                    }
                    return val
                }

                delegate: Item {
                    id: delegateItem
                    width: dataListView.width
                    height: 26

                    property color rowColor: index % 2 === 0 ? Theme.background : Theme.panel
                    property var currentRowData: model

                    Row {
                        height: parent.height

                        Repeater {
                            model: columnHeaders
                            delegate: Rectangle {
                                width: columnWidths[index]
                                height: parent.height
                                color: delegateItem.rowColor
                                border.color: Theme.borderLine
                                border.width: 0.5

                                property var cellValue: delegateItem.currentRowData ? delegateItem.currentRowData[modelData.key] : "--"

                                Text {
                                    text: dataListView.formatDisplay(modelData.key, cellValue)
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: 10
                                    font.bold: modelData.key.indexOf("RSSI") >= 0 || modelData.key === "monitorRSSI"
                                    color: (modelData.key.indexOf("RSSI") >= 0 || modelData.key === "monitorRSSI") ?
                                           dataListView.getRssiColor(cellValue) : Theme.text
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                // 空数据提示 - 作为 ListView 的子项
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: Theme.borderLine
                    border.width: 0.5
                    visible: dataListView.count === 0
                    z: 1

                    Column {
                        anchors.centerIn: parent
                        width: parent.width * 0.6
                        spacing: 0

                        component StyledText: Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            color: Theme.textDim
                            font.pixelSize: 12
                            opacity: 0.5
                            horizontalAlignment: Text.AlignLeft
                            lineHeight: 24
                            lineHeightMode: Text.FixedHeight
                        }
                        component HighlightText: Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            color: Theme.error
                            font.pixelSize: 12
                            opacity: 0.5
                            horizontalAlignment: Text.AlignLeft
                            lineHeight: 24
                            lineHeightMode: Text.FixedHeight
                        }

                        StyledText { text: "暂无数据，请点击“开始测试”按钮测试信号强度" }
                        Item { width: 1; height: 12 }
                        HighlightText { text: "•   无线音频路径必须设置为“主机”;" }
                        StyledText { text: "•   调节（+/-）可设置延时发送指令，待系统稳定后测试信号;" }
                        StyledText { text: "•   点击“开始测试”，将自动完成三步流程: " }
                        StyledText { text: " 1. 获取监测信道数量"; leftPadding: 10 }
                        StyledText { text: " 2. 获取监测信道频率和RSSI"; leftPadding: 10 }
                        StyledText { text: " 3. 测试4个无线信道的RSSI"; leftPadding: 10 }
                        StyledText { text: "•   请先在主机列表中选择一个已就绪的主机"; leftPadding: 10 }
                    }
                }
            }
        }
    }
}
