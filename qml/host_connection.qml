import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Audio.Hardware 1.0  // 导入注册的模块
import "./CustomWidget"

ColumnLayout {
    id: host_connection_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // ========== 属性定义 ==========
    property var currentConnectionType: ConnectionType.SerialPort  // 当前选择的连接类型
    property var serialConfig: ({})  // 串口配置
    property var tcpConfig: ({})     // TCP配置
    property bool hasPorts: false  // 是否有可用串口
    property bool isSerialSelected: serialRadio.checked  // 跟踪串口选中状态
    property string lastError: ""   // 存储最后一次错误
    property bool functionBtnEnable: false // 连接按钮可用与否

    // ========== 初始化 ==========
    Component.onCompleted: {
        // 页面加载时创建串口适配器
        connectionFactory.createNewAdapter(ConnectionType.SerialPort)

        // 监听连接状态变化
        connectionFactory.connectionStatusChanged.connect(function(status, type) {
            // console.log("连接状态变化:", status, "类型:", type)
            updateUIByStatus(status)
        })

        // 监听统计信息更新
        connectionFactory.statisticsUpdated.connect(function(time, sent, received) {
            //console.log("统计信息 - 时长:", time, "发送:", sent, "接收:", received)
        })

        // 立即刷新一次
        refreshSerialPorts()
    }

    // ========== UI状态更新函数 ==========
    function updateUIByStatus(status) {
        switch(status) {
        case ConnectionFactory.Connected:
            connectBtn.text = "断开连接"
            connectBtn.enabled = true
            statusText.text = "已连接"
            statusText.color = "#25D366"
            break
        case ConnectionFactory.Connecting:
            connectBtn.text = "连接中..."
            connectBtn.enabled = false
            statusText.text = "连接中"
            statusText.color = "orange"
            break
        case ConnectionFactory.Disconnecting:
            connectBtn.text = "断开中..."
            connectBtn.enabled = false
            statusText.text = "断开中"
            statusText.color = "orange"
            break
        case ConnectionFactory.Disconnected:
        default:
            connectBtn.text = "连接设备"
            connectBtn.enabled = true
            statusText.text = "未连接"
            statusText.color = "red"
            break
        }
    }

    // ========== 获取当前配置 ==========
    function getCurrentConfig() {
        if (serialRadio.checked) {
            return {
                portName: portCombo.currentText,
                baudRate: parseInt(baudRateCombo.currentText),
                dataBits: 8 - dataBitsCombo.currentIndex,   // 8,7,6,5
                parity: parityCombo.currentIndex,           // 0:无,1:奇,2:偶
                stopBits: stopBitsCombo.currentIndex === 0 ? 1 :
                         (stopBitsCombo.currentIndex === 1 ? 3 : 2)  // 1→1, 1.5→3, 2→2
            }
        } else if (tcpRadio.checked) {
            return {
                hostName: ipAddressField.text,
                port: parseInt(portNumber.text),
                timeout: parseInt(timeout.text),
                retryCount: retrySpin.value
            }
        }
        return {}
    }

    // ========== 刷新串口列表 ==========
    function refreshSerialPorts() {
        var ports = connectionFactory.getAdapterPorts(ConnectionType.SerialPort)
        portCombo.model = ports
        hasPorts = ports.length > 0
        // console.log("刷新串口列表:", ports, "是否有串口:", hasPorts)
    }

    // ========== 更新连接参数显示 ==========
    // 获取连接类型名称
    function getConnectionTypeName(type) {
        switch(type) {
        case ConnectionType.SerialPort: return "串口"
        case ConnectionType.TCPSocket: return "TCP/IP"
        case ConnectionType.Bluetooth: return "蓝牙"
        default: return "未知"
        }
    }

    // 获取连接状态文本
    function getConnectionStatusText(status) {
        switch(status) {
        case ConnectionFactory.Disconnected: return "未连接"
        case ConnectionFactory.Connecting: return "连接中..."
        case ConnectionFactory.Connected: return "已连接"
        case ConnectionFactory.Disconnecting: return "断开中..."
        default: return "未知"
        }
    }

    // 获取状态颜色
    function getStatusColor(status) {
        switch(status) {
        case ConnectionFactory.Connected: return "#00AF69"
        case ConnectionFactory.Connecting: return "orange"
        case ConnectionFactory.Disconnecting: return "orange"
        default: return "red"
        }
    }

    // 格式化时间
    function formatTime(seconds) {
        if (seconds < 60) return seconds + " 秒"
        if (seconds < 3600) {
            var minutes = Math.floor(seconds / 60)
            var secs = seconds % 60
            return minutes + " 分" //+ secs + "秒"
        }
        var hours = Math.floor(seconds / 3600)
        var mins = Math.floor((seconds % 3600) / 60)
        var sec = seconds % 60
        return hours + " 时" + mins + " 分" //+ sec + "秒"
    }

    // 格式化字节数
    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / (1024 * 1024)).toFixed(1) + " MB"
    }

    // 获取连接参数显示文本
    function getConnectionParamsDisplay() {
        var params = connectionFactory.currentConnectionParameters

        if (!params || Object.keys(params).length === 0) {
            return "未配置"
        }

        var type = connectionFactory.currentConnectedType
        if (type === ConnectionType.SerialPort) {
            // 校验位转换
            var parityMap = ["NoParity", "OddParity", "EvenParity"]
            var parityText = parityMap[params.parity || 0]

            // 停止位转换
            var stopBitsMap = {1: "OneStop", 3: "OneAndHalfStop", 2: "TwoStop"}
            var stopBitsText = stopBitsMap[params.stopBits || 1] || "OneStop"

            // 格式化输出：COM1 @ 115200， 8 Bit， NoParity，OneStop
            return params.portName + " @ " + params.baudRate +
                   "， " + (params.dataBits || 8) + " Bit" +
                   "， " + parityText +
                   "， " + stopBitsText
        }
        else if (type === ConnectionType.TCPSocket) {
            return params.hostName + ":" + params.port
        }

        return "已配置"
    }

    // 监听错误信号
    Connections {
        target: connectionFactory
        function onErrorOccurred(error) {
            lastError = error
            // 3秒后自动清除错误显示
            errorClearTimer.start()
        }
    }

    Timer {
        id: errorClearTimer
        interval: 3000
        onTriggered: lastError = ""
    }

    // ==============================
    // 顶部页面标题栏
    Item {
        id: pageTitle
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        // 标题
        Text {
            text: "设置连接参数"
            font.pixelSize: 16
            color: Theme.text

            anchors.left: parent.left
            anchors.leftMargin: 10  // 左边距20px
            anchors.top: parent.top
            anchors.topMargin: 10   // 上边距20px
        }
    }

    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 40
    }

    // 连接方式雷达按钮
    Item {
        id: radioGroup
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 40
        Layout.leftMargin: 30
        Layout.rightMargin: 20

        RowLayout {
            id: radioArea
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 6

            CustomRadioButton {
                id: serialRadio
                text: "串口连接"

                checked: true
                onCheckedChanged: {
                    if (checked) {
                        tcpRadio.checked = false
                        btRadio.checked = false
                        serialGroup.visible = true
                        tcpGroup.visible = false
                        currentConnectionType = ConnectionType.SerialPort
                        isSerialSelected = true  // 更新选中状态

                        // 切换适配器类型
                        connectionFactory.switchAdapter(ConnectionType.SerialPort)

                        // 切换到串口时刷新一次列表
                        refreshSerialPorts()
                    } else {
                        isSerialSelected = false
                    }
                }
            }

            Item {
                Layout.preferredWidth: 30
            }

            CustomRadioButton {
                id: tcpRadio
                text: "TCP/IP连接"

                onCheckedChanged: {
                    if (checked) {
                        serialRadio.checked = false;
                        btRadio.checked = false;
                        serialGroup.visible = false;
                        tcpGroup.visible = true;
                        currentConnectionType = ConnectionType.TCPSocket;
                        isSerialSelected = false  // 更新选中状态

                        connectionFactory.switchAdapter(ConnectionType.TCPSocket)   // 切换适配器类型
                    }
                }
            }

            Item {
                Layout.preferredWidth: 30
            }

            CustomRadioButton {
                id: btRadio
                text: "蓝牙连接"

                enabled: false
                onCheckedChanged: {
                    if (checked) {
                        serialRadio.checked = false;
                        tcpRadio.checked = false;
                        serialGroup.visible = false;
                        tcpGroup.visible = false;
                        currentConnectionType = ConnectionType.Bluetooth;
                        isSerialSelected = false  // 更新选中状态

                        connectionFactory.switchAdapter(ConnectionType.Bluetooth)   // 切换适配器类型
                    }
                }
            }
        }
    }
    // 分隔线
    Rectangle {
        Layout.preferredHeight: 0.5
        Layout.preferredWidth: parent.width
        color: Theme.borderFilled
        opacity: 0.7
    }
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 20
    }

    // 连接参数选择
    Item {
        id: parameterArea
        Layout.fillHeight: true
        Layout.preferredWidth: host_connection_page.width * 0.64

        // 串口参数选择控件
        ColumnLayout {
            id: serialGroup
            visible: serialRadio.checked
            Layout.fillWidth: true
            spacing: 15

            anchors.left: parent.left
            anchors.leftMargin: 40  // 左边距40px

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "串        口："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomCombox {
                    id: portCombo
                    model: []   // ["COM1", "COM2", "COM3", "COM4"]
                    currentIndex: 0
                    Layout.preferredWidth: 400
                    Layout.preferredHeight: 28
                }
                CustomButton {
                    id: refreshPort
                    text: "刷新"
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 28

                    onClicked: {
                        console.log("刷新串口号");
                        refreshSerialPorts()
                    }
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "波  特  率："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomCombox {
                    id: baudRateCombo
                    model: ["115200", "57600", "38400", "19200", "9600"]
                    currentIndex: 0
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "数  据  位："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomCombox {
                    id: dataBitsCombo
                    model: ["8", "7", "6", "5"]
                    currentIndex: 0
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "校  验  位："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomCombox {
                    id: parityCombo
                    model: ["无", "奇校验", "偶校验"]
                    currentIndex: 0
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "停  止  位："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomCombox {
                    id: stopBitsCombo
                    model: ["1", "1.5", "2"]
                    currentIndex: 0
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                }
            }
        }

        // TCP/IP参数输入控件
        ColumnLayout {
            id: tcpGroup
            visible: tcpRadio.checked
            Layout.fillWidth: true
            spacing: 15

            anchors.left: parent.left
            anchors.leftMargin: 40  // 左边距40px

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "IP  地   址："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomTextField {
                    id: ipAddressField
                    placeholderText: "192.168.1.100"
                    text: "192.168.1.100"
                    Layout.preferredWidth: 380
                    Layout.preferredHeight: 28
                    // leftPadding: 8
                    // rightPadding: 24
                }
                CustomButton {
                    id: refreshIP
                    text: "检测设备IP"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 28
                    enabled: false

                    onClicked: {
                        console.log("检测设备IP");
                    }
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "端  口  号："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomTextField {
                    id: portNumberField
                    placeholderText: "8080"
                    text: "8080"
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                    leftPadding: 8
                    rightPadding: 24
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "超       时："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomTextField {
                    id: timeoutField
                    placeholderText: "5000 ms"
                    text: "5000"
                    Layout.preferredWidth: 470
                    Layout.preferredHeight: 28
                    leftPadding: 8
                    rightPadding: 24
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Text { text: "重试次数："; color: Theme.text; Layout.preferredWidth: 80 }
                CustomSpin {
                    id: retrySpin
                    from: 1; to: 5; value: 3
                    Layout.preferredWidth: 245
                    Layout.preferredHeight: 28
                }
            }
        }

        // Bluetooth配对设置
        ColumnLayout {
            id: btGroup
            visible: btRadio.checked
            Layout.fillWidth: true
            spacing: 15
        }
    }

    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 45
    }

    // 功能按钮
    Item {
        id: functionArea
        Layout.preferredHeight: 40
        Layout.preferredWidth: host_connection_page.width * 0.64
        Layout.leftMargin: 130

        RowLayout {
            id: buttonArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // 加载配置表按钮
            CustomButton {
                id: paraLoadBtn
                text: "加载默认参数"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 120
                Layout.preferredHeight: 28


                onClicked: {
                    // 可以从配置文件加载，这里简单示例
                    var loadedConfig = {
                        portName: "COM1",
                        baudRate: 115200,
                        dataBits: 8,
                        parity: 0,
                        stopBits: 1
                    }
                    // 填充到UI控件
                    var index = portCombo.find(loadedConfig.portName)
                    if (index >= 0) portCombo.currentIndex = index

                    index = baudRateCombo.find(loadedConfig.baudRate.toString())
                    if (index >= 0) baudRateCombo.currentIndex = index

                    index = dataBitsCombo.find(loadedConfig.dataBits.toString())
                    if (index >= 0) dataBitsCombo.currentIndex = index

                    parityCombo.currentIndex = loadedConfig.parity

                    // 停止位转换
                    if (loadedConfig.stopBits === 1) {
                        stopBitsCombo.currentIndex = 0
                    } else if (loadedConfig.stopBits === 3) {
                        stopBitsCombo.currentIndex = 1
                    } else if (loadedConfig.stopBits === 2) {
                        stopBitsCombo.currentIndex = 2
                    }

                    console.log("参数加载完成")
                }

            }

            // 保存按钮
            CustomButton {
                id: paraSaveBtn
                text: "保存参数"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 120
                Layout.preferredHeight: 28

                onClicked: {
                    var config = getCurrentConfig()
                    var success = connectionFactory.saveAdapterConnectionParameters(
                        currentConnectionType,
                        config
                    )

                    if (!success) {
                        console.log("参数保存失败")
                    }
                }
            }

            // 连接/断开按钮
            CustomButton {
                id: connectBtn
                text: "连接设备"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 28

                backgroundColor:  Qt.darker(Theme.textMenu, 1.1)
                hoverColor: Theme.textMenu
                pressedColor: Qt.darker(Theme.textMenu, 1.3)

                onClicked: {
                    if (connectionFactory.isConnected) {
                        // 已连接 → 断开
                        connectionFactory.disconnectFromAdapter()

                    } else {
                        // 未连接 → 连接
                        var config = getCurrentConfig()

                        // 第一步：保存参数
                        connectionFactory.saveAdapterConnectionParameters(
                            currentConnectionType,
                            config
                        )

                        // 第二步：建立连接
                        var success = connectionFactory.connectToAdapter(
                            currentConnectionType,
                            config
                        )

                        if (!success) {
                            console.log("连接失败")
                        }
                    }
                }
            }
        }
    }

    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 110
    }
    Rectangle {
        Layout.preferredHeight: 0.5
        Layout.preferredWidth: parent.width
        color: Theme.borderFilled
        opacity: 0.8
    }
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 1
    }

    // 连接信息
    Item {
        id: dataArea
        Layout.fillHeight: true
        Layout.preferredWidth: parent.width
        Layout.leftMargin: 130


        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            // 连接类型显示
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "连接类型:"
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.preferredWidth: 55  // 固定标签宽度
                }

                Text {
                    id: typeText
                    text: getConnectionTypeName(connectionFactory.currentConnectedType)
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.fillWidth: true
                    elide: Text.ElideRight  // 如果太长就省略
                }
            }

            // 连接状态显示
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "连接状态:"
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.preferredWidth: 55  // 固定标签宽度
                }

                Text {
                    id:statusText
                    text: getConnectionStatusText(connectionFactory.connectionStatus)
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.fillWidth: true
                    elide: Text.ElideRight  // 如果太长就省略
                }
            }
            // 连接参数显示
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "连接参数:"
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.preferredWidth: 55  // 固定标签宽度
                }

                Text {
                    id: paraText
                    text: getConnectionParamsDisplay()
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.fillWidth: true
                    elide: Text.ElideRight  // 如果太长就省略
                }
            }

            // 通信数据显示
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "连接时长:"
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.preferredWidth: 55  // 固定标签宽度
                }

                Text {
                    id: dataText
                    text: formatTime(connectionFactory.connectedTime) + ", 发送字节："
                          + formatBytes(connectionFactory.bytesSent) + ", 接收字节："
                          + formatBytes(connectionFactory.bytesReceived) + ", 总字节："
                          + formatBytes(connectionFactory.bytesSent + connectionFactory.bytesReceived)
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.fillWidth: true
                    elide: Text.ElideRight  // 如果太长就省略
                }
            }
            // 连接参数显示
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "错误:"
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.preferredWidth: 55  // 固定标签宽度
                }

                Text {
                    id: errorText
                    text: lastError
                    color: Theme.text
                    font.pixelSize: 11
                    opacity: 0.7
                    Layout.fillWidth: true
                    elide: Text.ElideRight  // 如果太长就省略
                }
            }
        }
    }
}
