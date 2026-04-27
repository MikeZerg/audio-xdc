// /qml/host_settings_xdc.qml
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "./CustomWidget"

ColumnLayout {
    id: host_settings_xdc_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    property int flexWidth: parent.width / 30

    // [修复] 添加选中主机的属性 - 使用 selectedHostAddressInt
    property int selectedHostAddress: 0

    property var controller: null

    // 辅助属性用于强制刷新显示
    property bool forceUpdate: false

    // 获取各个设置对象（使用 view 前缀）
    property var volumeSettings: controller ? controller.viewVolumeSettings : null
    property var speechSettings: controller ? controller.viewSpeechSettings : null
    property var cameraSettings: controller ? controller.viewCameraSettings : null

    // [修复] 刷新控制器 - 使用 selectedHostAddressInt
    function refreshController() {
        if (typeof hostManager === 'undefined' || !hostManager) {
            controller = null;
            selectedHostAddress = 0;
            return;
        }
        // [关键修复] 使用 selectedHostAddressInt 替代 activeHostAddress
        var addr = hostManager.selectedHostAddressInt;
        selectedHostAddress = addr;
        if (addr > 0) {
            controller = hostManager.getController(addr);
            console.log("host_settings_xdc: 获取控制器, 地址=0x" + addr.toString(16));
        } else {
            controller = null;
            console.log("host_settings_xdc: 没有选中的主机");
        }
        // 刷新显示
        forceUpdate = !forceUpdate
    }

    Component.onCompleted: {
        refreshController()
    }

    // [新增] 监听选中主机变化
    Connections {
        target: hostManager
        enabled: hostManager !== null

        function onSelectedHostChanged(oldAddress, newAddress) {
            console.log("host_settings_xdc: 选中主机变更 0x" + oldAddress.toString(16) + " -> 0x" + newAddress.toString(16))
            refreshController()
        }

        function onHostInfoChanged(hostAddress) {
            if (hostAddress === selectedHostAddress && selectedHostAddress > 0) {
                refreshController()
            }
        }
    }

    // 顶部标题
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "主机设置与管理"
            font.pixelSize: 16
            color: Theme.text
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    Item { Layout.preferredHeight: 40 }

    // 主机信息（类型和地址）
    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        Layout.leftMargin: 20
        Layout.rightMargin: 20
        spacing:20

        Label {
            text: "XDC236有线无线融合会议主机"
            color: Theme.text
        }
        Label {
            text: "地址：" + (controller ? "0x" + controller.address.toString(16).toUpperCase().padStart(2, '0') : "---")
            color: Theme.text
        }

        // 当没有选中主机时显示提示
        Label {
            text: controller ? "" : " (请先在主机列表中选择一个已就绪的主机)"
            color: Theme.textDim
            font.pixelSize: 11
            visible: controller === null
        }
    }

    // 主机状态设置
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: 15
        Layout.rightMargin: 15
        color: "transparent"

        border.color: Qt.hsla(0, 0, 0.5, 0.15)
        border.width: 0.7

        // 当没有控制器时显示提示
        Rectangle {
            anchors.fill: parent
            color: Theme.background
            opacity: 0.7
            visible: controller === null
            z: 1

            Text {
                anchors.centerIn: parent
                text: ""  //"请先在主机列表中选择一个已就绪的 XDC236 主机"
                color: Theme.text
                opacity: 0.5
                font.pixelSize: 14
            }
        }

        ScrollView {
            id: scrollView
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth
            enabled: controller !== null
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: scrollView.availableWidth
                spacing: 0

                // 使用 GridLayout 布局所有设置行，每行5列（标签、值、控件、自适应间距、按钮）
                GridLayout {
                    id: gridLayout
                    Layout.fillWidth: true
                    Layout.leftMargin: 25
                    Layout.rightMargin: 25
                    Layout.topMargin: 25
                    Layout.bottomMargin: 25
                    columnSpacing: 15
                    rowSpacing: 8
                    columns: 5

                    // ========== 第1行：系统时间 ==========
                    Label {
                        text: "系统时间："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: systemTimeValue
                        text: {
                            if (controller && controller.viewSystemDateTime) {
                                var dt = controller.viewSystemDateTime
                                var dummy = host_settings_xdc_page.forceUpdate
                                return Qt.formatDateTime(dt, "yyyy-MM-dd hh:mm:ss")
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    Rectangle {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        border.color: Theme.borderLine
                        border.width: 0.5
                        radius: 2
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            TextInput {
                                id: inputSystemDatetime
                                text: systemTimeValue.text
                                color: Theme.text
                                Layout.fillWidth: true
                                Layout.preferredHeight: 26
                                leftPadding: 4
                                inputMask: "0000-00-00 00:00:00"
                                selectByMouse: true
                                verticalAlignment: TextInput.AlignVCenter
                                font.pixelSize: 11
                            }

                            Rectangle {
                                Layout.preferredWidth: 1
                                Layout.preferredHeight: 20
                                color: Theme.borderLine
                                opacity: 0.4
                            }

                            CustomDatetimePicker {
                                id: dateTimePicker
                                visible: false

                                onDateTimeConfirmed: {
                                    inputSystemDatetime.text = Qt.formatDateTime(date, "yyyy-MM-dd hh:mm:ss")
                                    close()
                                }
                            }
                            Button {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                icon.source: "qrc:/image/app-timer.svg"
                                icon.width: 16
                                icon.height: 16
                                display: AbstractButton.IconOnly

                                background: Rectangle {
                                    color: parent.hovered ? Theme.surfaceSelected : "transparent"
                                    radius: 2
                                }

                                onClicked: {
                                    if (!dateTimePickerLoader.active) {
                                        dateTimePickerLoader.active = true;
                                    }
                                    dateTimePickerLoader.item.open();
                                }
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                var dateTime = new Date(inputSystemDatetime.text.replace(/-/g, '/'));
                                if (dateTime instanceof Date && !isNaN(dateTime)) {
                                    var utcTime = Math.floor(dateTime.getTime() / 1000);
                                    controller.setSystemDateTime(utcTime);
                                    console.log("设置系统时间:", dateTime.toLocaleString());
                                    // [新增] 设置后延迟查询刷新
                                    Qt.callLater(function() {
                                        if (controller) controller.getSystemDateTime();
                                    });
                                } else {
                                    console.log("无效的日期时间格式:", inputSystemDatetime.text);
                                }
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第2行：发言模式 ==========
                    Label {
                        text: "发言模式："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: speechModeValue
                        text: {
                            if (speechSettings) {
                                var mode = speechSettings.speechMode
                                var dummy = host_settings_xdc_page.forceUpdate
                                if (mode === 0) return "先进先出"
                                if (mode === 1) return "自锁模式"
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    RowLayout {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 20

                        CustomRadioButton {
                            id: speechModeFIFO
                            text: "先进先出"
                            checked: {
                                if (speechSettings) {
                                    return speechSettings.speechMode === 0
                                }
                                return true
                            }
                            contentItem: Text {
                                text: speechModeFIFO.text
                                font.pixelSize: 11
                                color: speechModeFIFO.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: speechModeFIFO.indicator.width + speechModeFIFO.spacing
                            }
                        }
                        CustomRadioButton {
                            id: speechModeLimited
                            text: "自锁模式"
                            checked: {
                                if (speechSettings) {
                                    return speechSettings.speechMode === 1
                                }
                                return false
                            }
                            contentItem: Text {
                                text: speechModeLimited.text
                                font.pixelSize: 11
                                color: speechModeLimited.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: speechModeLimited.indicator.width + speechModeLimited.spacing
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                var mode = speechModeFIFO.checked ? 0 : 1
                                controller.setSpeechMode(mode)
                                console.log("设置发言模式为:", speechModeValue.text)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getSpeechMode();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第3行：最大发言数 ==========
                    Label {
                        text: "最大发言数："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: maxSpeakersValue
                        text: {
                            if (speechSettings && speechSettings.maxSpeechCount !== undefined) {
                                var dummy = host_settings_xdc_page.forceUpdate
                                return speechSettings.maxSpeechCount + " / " + speechSettings.hardwareMaxSpeech
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpin {
                        id: maxSpeakersSpin
                        from: 1
                        to: {
                            if (speechSettings && speechSettings.hardwareMaxSpeech) {
                                return speechSettings.hardwareMaxSpeech
                            }
                            return 4
                        }
                        step: 1
                        value: {
                            if (speechSettings && speechSettings.maxSpeechCount !== undefined) {
                                return speechSettings.maxSpeechCount
                            }
                            return 3
                        }
                        spinWidth: 200
                        spinHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        leftBorderWidth: 0.5; centerBorderWidth: 0.5; rightBorderWidth: 0.5
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                controller.setMaxSpeechCount(maxSpeakersSpin.value)
                                console.log("设置最大发言数为:", maxSpeakersSpin.value)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getMaxSpeechCount();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第4行：无线音量 ==========
                    Label {
                        text: "无线音量："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: wirelessVolumeValue
                        text: {
                            if (volumeSettings && volumeSettings.wirelessVolume !== undefined) {
                                var dummy = host_settings_xdc_page.forceUpdate
                                return (volumeSettings.wirelessVolume + 1) + " / " + volumeSettings.wirelessMaxVolume
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpin {
                        id: wirelessVolumeSpin
                        from: 1
                        to: {
                            if (volumeSettings && volumeSettings.wirelessMaxVolume) {
                                return volumeSettings.wirelessMaxVolume
                            }
                            return 9
                        }
                        step: 1
                        value: {
                            if (volumeSettings && volumeSettings.wirelessVolume !== undefined) {
                                return volumeSettings.wirelessVolume + 1
                            }
                            return 7
                        }
                        spinWidth: 200
                        spinHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        leftBorderWidth: 0.5; centerBorderWidth: 0.5; rightBorderWidth: 0.5
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                controller.setWirelessVolume(wirelessVolumeSpin.value - 1)
                                console.log("设置无线音量:", wirelessVolumeSpin.value)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getWirelessVolume();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第5行：有线音量 ==========
                    Label {
                        text: "有线音量："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: wiredVolumeValue
                        text: {
                            if (volumeSettings && volumeSettings.wiredVolume !== undefined) {
                                var dummy = host_settings_xdc_page.forceUpdate
                                return (volumeSettings.wiredVolume + 1) + " / " + volumeSettings.wiredMaxVolume
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpin {
                        id: wiredVolumeSpin
                        from: 1
                        to: {
                            if (volumeSettings && volumeSettings.wiredMaxVolume) {
                                return volumeSettings.wiredMaxVolume
                            }
                            return 7
                        }
                        step: 1
                        value: {
                            if (volumeSettings && volumeSettings.wiredVolume !== undefined) {
                                return volumeSettings.wiredVolume + 1
                            }
                            return 6
                        }
                        spinWidth: 200
                        spinHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        leftBorderWidth: 0.5; centerBorderWidth: 0.5; rightBorderWidth: 0.5
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                controller.setWiredVolume(wiredVolumeSpin.value - 1)
                                console.log("设置有线音量:", wiredVolumeSpin.value)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getWiredVolume();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第6行：天线盒音量 ==========
                    Label {
                        text: "天线盒音量："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: antennaVolumeValue
                        text: {
                            if (volumeSettings && volumeSettings.antennaVolume !== undefined) {
                                var dummy = host_settings_xdc_page.forceUpdate
                                return (volumeSettings.antennaVolume + 1) + " / " + volumeSettings.antennaMaxVolume
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpin {
                        id: antennaVolumeSpin
                        from: 1
                        to: {
                            if (volumeSettings && volumeSettings.antennaMaxVolume) {
                                return volumeSettings.antennaMaxVolume
                            }
                            return 9
                        }
                        step: 1
                        value: {
                            if (volumeSettings && volumeSettings.antennaVolume !== undefined) {
                                return volumeSettings.antennaVolume + 1
                            }
                            return 5
                        }
                        spinWidth: 200
                        spinHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        leftBorderWidth: 0.5; centerBorderWidth: 0.5; rightBorderWidth: 0.5
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                controller.setAntennaVolume(antennaVolumeSpin.value - 1)
                                console.log("设置天线盒音量:", antennaVolumeSpin.value)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getAntennaVolume();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第7行：无线音频路径 ==========
                    Label {
                        text: "无线音频路径："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: audioPathValue
                        text: {
                            if (speechSettings) {
                                var path = speechSettings.wirelessAudioPath
                                var dummy = host_settings_xdc_page.forceUpdate
                                if (path === 0) return "主机"
                                if (path === 1) return "天线盒"
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    RowLayout {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 20

                        CustomRadioButton {
                            id: audioPathHost
                            text: "主机"
                            checked: {
                                if (speechSettings) {
                                    return speechSettings.wirelessAudioPath === 0
                                }
                                return true
                            }
                            contentItem: Text {
                                text: audioPathHost.text
                                font.pixelSize: 11
                                color: audioPathHost.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: audioPathHost.indicator.width + audioPathHost.spacing
                            }
                        }
                        CustomRadioButton {
                            id: audioPathAntenna
                            text: "天线盒"
                            checked: {
                                if (speechSettings) {
                                    return speechSettings.wirelessAudioPath === 1
                                }
                                return false
                            }
                            contentItem: Text {
                                text: audioPathAntenna.text
                                font.pixelSize: 11
                                color: audioPathAntenna.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: audioPathAntenna.indicator.width + audioPathAntenna.spacing
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                var path = audioPathHost.checked ? 0 : 1
                                controller.setWirelessAudioPath(path)
                                console.log("设置无线音频路径:", audioPathHost.checked ? "主机" : "天线盒")
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getWirelessAudioPath();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第8行：摄像机地址 ==========
                    Label {
                        text: "摄像机地址："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: cameraAddrValue
                        text: {
                            if (cameraSettings && cameraSettings.address !== undefined) {
                                var dummy = host_settings_xdc_page.forceUpdate
                                return "0x" + cameraSettings.address.toString(16).toUpperCase().padStart(2, '0')
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpin {
                        id: cameraAddrSpin
                        from: 1
                        to: 255
                        step: 1
                        value: {
                            if (cameraSettings && cameraSettings.address !== undefined) {
                                return cameraSettings.address
                            }
                            return 1
                        }
                        spinWidth: 200
                        spinHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        leftBorderWidth: 0.5; centerBorderWidth: 0.5; rightBorderWidth: 0.5
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                controller.setCameraAddress(cameraAddrSpin.value)
                                console.log("设置摄像机地址:", cameraAddrSpin.value)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getCameraAddress();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第9行：摄像机协议 ==========
                    Label {
                        text: "摄像机协议："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: cameraProtocolValue
                        text: {
                            if (cameraSettings) {
                                var protocol = cameraSettings.protocol
                                var dummy = host_settings_xdc_page.forceUpdate
                                if (protocol === 0) return "NONE"
                                if (protocol === 1) return "Visca"
                                if (protocol === 2) return "Peleco-D"
                                if (protocol === 3) return "Peleco-P"
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    RowLayout {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 10

                        CustomRadioButton {
                            id: protocolNone
                            text: "NONE"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.protocol === 0
                                }
                                return false
                            }
                            contentItem: Text {
                                text: protocolNone.text
                                font.pixelSize: 11
                                color: protocolNone.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: protocolNone.indicator.width + protocolNone.spacing
                            }
                        }
                        CustomRadioButton {
                            id: protocolVisca
                            text: "Visca"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.protocol === 1
                                }
                                return true
                            }
                            contentItem: Text {
                                text: protocolVisca.text
                                font.pixelSize: 11
                                color: protocolVisca.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: protocolVisca.indicator.width + protocolVisca.spacing
                            }
                        }
                        CustomRadioButton {
                            id: protocolPelecoD
                            text: "Peleco-D"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.protocol === 2
                                }
                                return false
                            }
                            contentItem: Text {
                                text: protocolPelecoD.text
                                font.pixelSize: 11
                                color: protocolPelecoD.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: protocolPelecoD.indicator.width + protocolPelecoD.spacing
                            }
                        }
                        CustomRadioButton {
                            id: protocolPelecoP
                            text: "Peleco-P"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.protocol === 3
                                }
                                return false
                            }
                            contentItem: Text {
                                text: protocolPelecoP.text
                                font.pixelSize: 11
                                color: protocolPelecoP.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: protocolPelecoP.indicator.width + protocolPelecoP.spacing
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                var protocol = 1
                                if (protocolNone.checked) protocol = 0
                                else if (protocolVisca.checked) protocol = 1
                                else if (protocolPelecoD.checked) protocol = 2
                                else if (protocolPelecoP.checked) protocol = 3
                                controller.setCameraProtocol(protocol)
                                console.log("设置摄像机协议:", cameraProtocolValue.text)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getCameraProtocol();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }

                    // ========== 第10行：摄像机波特率 ==========
                    Label {
                        text: "摄像机波特率："
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    // [修改] 使用与 host_info_xdc 相同的简洁显示方式
                    Label {
                        id: cameraBaudrateValue
                        text: {
                            if (cameraSettings) {
                                var baudrate = cameraSettings.baudrate
                                var dummy = host_settings_xdc_page.forceUpdate
                                if (baudrate === 0) return "2400 bps"
                                if (baudrate === 1) return "4800 bps"
                                if (baudrate === 2) return "9600 bps"
                                if (baudrate === 3) return "115200 bps"
                            }
                            return "--"
                        }
                        font.pixelSize: 11
                        color: Theme.text
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 26
                        verticalAlignment: Text.AlignVCenter
                    }
                    RowLayout {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 10

                        CustomRadioButton {
                            id: baudrate2400
                            text: "2400"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.baudrate === 0
                                }
                                return false
                            }
                            contentItem: Text {
                                text: baudrate2400.text
                                font.pixelSize: 11
                                color: baudrate2400.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: baudrate2400.indicator.width + baudrate2400.spacing
                            }
                        }
                        CustomRadioButton {
                            id: baudrate4800
                            text: "4800"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.baudrate === 1
                                }
                                return false
                            }
                            contentItem: Text {
                                text: baudrate4800.text
                                font.pixelSize: 11
                                color: baudrate4800.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: baudrate4800.indicator.width + baudrate4800.spacing
                            }
                        }
                        CustomRadioButton {
                            id: baudrate9600
                            text: "9600"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.baudrate === 2
                                }
                                return false
                            }
                            contentItem: Text {
                                text: baudrate9600.text
                                font.pixelSize: 11
                                color: baudrate9600.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: baudrate9600.indicator.width + baudrate9600.spacing
                            }
                        }
                        CustomRadioButton {
                            id: baudrate115200
                            text: "115200"
                            checked: {
                                if (cameraSettings) {
                                    return cameraSettings.baudrate === 3
                                }
                                return true
                            }
                            contentItem: Text {
                                text: baudrate115200.text
                                font.pixelSize: 11
                                color: baudrate115200.checked ? Theme.textMenu : Theme.textDim
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: baudrate115200.indicator.width + baudrate115200.spacing
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                    }
                    CustomButton {
                        text: "设定"
                        font.pixelSize: 11
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: 10
                        borderWidth: 0.5
                        onClicked: {
                            if (controller) {
                                var baudrate = 3
                                if (baudrate2400.checked) baudrate = 0
                                else if (baudrate4800.checked) baudrate = 1
                                else if (baudrate9600.checked) baudrate = 2
                                else if (baudrate115200.checked) baudrate = 3
                                controller.setCameraBaudrate(baudrate)
                                console.log("设置摄像机波特率:", cameraBaudrateValue.text)
                                // [新增] 设置后延迟查询刷新
                                Qt.callLater(function() {
                                    if (controller) controller.getCameraBaudrate();
                                });
                            }
                        }
                    }

                    // 分隔线
                    Rectangle {
                        Layout.columnSpan: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0.5
                        Layout.leftMargin: -10
                        color: Theme.borderFilled
                        opacity: 0.4
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 110
        Layout.leftMargin: 15
        Layout.rightMargin: 15
        spacing: 10

        // 加载默认设置按钮
        CustomButton {
            id: loadDefaultSettings
            text: "加载默认设置"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 120
            Layout.preferredHeight: 28
            enabled: controller !== null

            onClicked: {
                if (controller) {
                    // 加载默认设置
                    controller.setWirelessVolume(6)      // 默认7 (索引6)
                    controller.setWiredVolume(5)         // 默认6 (索引5)
                    controller.setAntennaVolume(4)       // 默认5 (索引4)
                    controller.setSpeechMode(0)          // 先进先出
                    controller.setMaxSpeechCount(3)      // 最大3人
                    controller.setWirelessAudioPath(0)   // 主机
                    controller.setCameraProtocol(1)      // Visca
                    controller.setCameraAddress(1)       // 地址0x01
                    controller.setCameraBaudrate(3)      // 115200bps
                    console.log("加载默认设置完成")

                    // [新增] 延迟后批量查询刷新所有设置
                    Qt.callLater(function() {
                        if (controller) {
                            // 刷新音量设置
                            controller.getWirelessVolume();
                            controller.getWiredVolume();
                            controller.getAntennaVolume();
                            // 刷新发言设置
                            controller.getSpeechMode();
                            controller.getMaxSpeechCount();
                            controller.getWirelessAudioPath();
                            // 刷新摄像设置
                            controller.getCameraProtocol();
                            controller.getCameraAddress();
                            controller.getCameraBaudrate();
                            // 刷新时间
                            controller.getSystemDateTime();
                        }
                    }, 300);  // 稍长延时，等待默认设置生效
                }
            }
        }

        // 批量更新设置按钮
        CustomButton {
            id: batchSaveSettings
            text: "批量更新设置"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 120
            Layout.preferredHeight: 28
            enabled: controller !== null

            onClicked: {
                if (controller) {
                    // 批量应用当前界面设置
                    controller.setWirelessVolume(wirelessVolumeSpin.value - 1)
                    controller.setWiredVolume(wiredVolumeSpin.value - 1)
                    controller.setAntennaVolume(antennaVolumeSpin.value - 1)

                    var mode = speechModeFIFO.checked ? 0 : 1
                    controller.setSpeechMode(mode)

                    controller.setMaxSpeechCount(maxSpeakersSpin.value)

                    var path = audioPathHost.checked ? 0 : 1
                    controller.setWirelessAudioPath(path)

                    var protocol = 1
                    if (protocolNone.checked) protocol = 0
                    else if (protocolVisca.checked) protocol = 1
                    else if (protocolPelecoD.checked) protocol = 2
                    else if (protocolPelecoP.checked) protocol = 3
                    controller.setCameraProtocol(protocol)

                    controller.setCameraAddress(cameraAddrSpin.value)

                    var baudrate = 3
                    if (baudrate2400.checked) baudrate = 0
                    else if (baudrate4800.checked) baudrate = 1
                    else if (baudrate9600.checked) baudrate = 2
                    else if (baudrate115200.checked) baudrate = 3
                    controller.setCameraBaudrate(baudrate)

                    console.log("批量更新设置完成")

                    // [新增] 延迟后批量查询刷新所有设置
                    Qt.callLater(function() {
                        if (controller) {
                            controller.getWirelessVolume();
                            controller.getWiredVolume();
                            controller.getAntennaVolume();
                            controller.getSpeechMode();
                            controller.getMaxSpeechCount();
                            controller.getWirelessAudioPath();
                            controller.getCameraProtocol();
                            controller.getCameraAddress();
                            controller.getCameraBaudrate();
                            controller.getSystemDateTime();
                        }
                    }, 200);
                }
            }
        }
    }

    // 监听控制器信号，强制刷新显示
    Connections {
        target: controller
        enabled: controller !== null

        function onVolumeSettingsChanged() {
            forceUpdate = !forceUpdate
        }
        function onSpeechSettingsChanged() {
            forceUpdate = !forceUpdate
        }
        function onCameraSettingsChanged() {
            forceUpdate = !forceUpdate
        }
        function onSystemDateTimeUpdated(address, dateTime) {
            forceUpdate = !forceUpdate
        }
    }
}
