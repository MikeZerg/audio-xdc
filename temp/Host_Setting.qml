import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Theme 1.0

Item {
    id: page2_HostManagement_HostSetting
    anchors.centerIn: parent
    width: parent.width
    height: parent.height

    property int flexWidth: parent.width / 12
    property int spacing: 20

    ColumnLayout {
    anchors.fill: parent
    anchors.bottomMargin: Theme.margin10
    anchors.topMargin: Theme.margin10
    spacing: Theme.spacing2

        // 显示主机属性
        Rectangle {
            id: hostPropertySettings
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.margin10       // 将页面嵌入到走马灯式界面中，边距设置为10
            Layout.rightMargin: Theme.margin10      // 将页面嵌入到走马灯式界面中，边距设置为10
            Layout.topMargin: Theme.margin5         // 将页面嵌入到走马灯式界面中，边距设置为5
            color: Theme.bodyColor
            border.width: Theme.borderWidthThin
            border.color: Theme.borderColorDark
            radius: Theme.borderRadius3
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.margin15
                spacing: Theme.spacing10

                // 0x0104-(38 6C D3 00) 设置主机系统时间
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "系统时间："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.systemDatetime
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: 1

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                border.width: Theme.borderWidthThin
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                TextInput {
                                    id: inputSystemDatetime
                                    text: hostManager.currentHost.systemDatetime || Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
                                    anchors.fill: parent
                                    anchors.margins: Theme.margin5
                                    anchors.leftMargin: Theme.margin5
                                    inputMask: "0000-00-00 00:00:00"
                                    font.family: Theme.fontName
                                    font.pixelSize: Theme.fontSizeBody
                                    font.weight: Theme.fontWeightLight
                                    selectByMouse: true
                                    verticalAlignment: TextInput.AlignVCenter
                                }
                            }
                            Loader {
                                id: dateTimePickerLoader
                                source: "qrc:/components/component/CustomDateTimePickerDialog.qml"
                                active: false
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredHeight: Theme.controllerHeight // 明确高度

                                onLoaded: {
                                    item.dateTimeConfirmed.connect(function(date) {
                                        inputSystemDatetime.text = Qt.formatDateTime(date, "yyyy-MM-dd hh:mm:ss")
                                    })
                                }
                            }
                            Button {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                Layout.fillWidth: false

                                icon.source: "qrc:/image/icon/calendar.svg"
                                icon.width: 20
                                icon.height: 20
                                display: AbstractButton.IconOnly
                                palette.button: Theme.backgroundColor
                                palette.buttonText: Theme.primaryColor
                                palette.dark: "#17a81a"

                                onClicked: {
                                    if (!dateTimePickerLoader.active) {
                                        dateTimePickerLoader.active = true;
                                    }
                                    dateTimePickerLoader.item.open();
                                }
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredWidth: -1
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            var dateTime = new Date(inputSystemDatetime.text);
                            if (hostManager.currentHost && dateTime instanceof Date && !isNaN(dateTime)) {
                                hostManager.currentHost.hostSet_systemDatetime(dateTime);
                                // console.log("设置日期时间为:", inputSystemDatetime.text);
                            } else {
                                // console.log("无效的日期时间格式:", inputSystemDatetime.text);
                            }
                        }
                    }

                }
                // 分隔线
                Image {
                    Layout.fillWidth: true
                    height: 1
                    source: "qrc:/image/icon/divider.svg"  // 1px 灰色图片
                    fillMode: Image.Stretch
                    smooth: false
                    antialiasing: false
                }

                // 0x0108-(    ) 设置发言模式
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "发言模式："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.speechMode      //"0x0208"   //"先进先出"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            RadioButton {
                                id: speechModeFIFO
                                text: "先进先出"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.speechMode === "先进先出" || hostManager.currentHost.speechMode === "FIFO"
                            }
                            RadioButton {
                                id: speechModeLimited
                                text: "自锁模式"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.speechMode === "自锁模式" || hostManager.currentHost.speechMode === "Self Lock"
                            }
                            Item {
                                Layout.columnSpan: 2
                                Layout.preferredWidth: flexWidth * 3
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            var selectedSpeechMode = "";
                            if (speechModeFIFO.checked) {
                                selectedSpeechMode = speechModeFIFO.text;
                                hostManager.currentHost.hostSet_speechMode("FIFO")
                            } else if (speechModeLimited.checked) {
                                selectedSpeechMode = speechModeLimited.text;
                                hostManager.currentHost.hostSet_speechMode("Self Lock")
                            }
                            // console.log("设置发言模式为:", selectedSpeechMode)
                        }
                    }
                }
                // 分隔线
                Image {
                    Layout.fillWidth: true
                    height: 1
                    source: "qrc:/image/icon/divider.svg"  // 1px 灰色图片
                    fillMode: Image.Stretch
                    smooth: false
                    antialiasing: false
                }

                // 0x0109-(    ) 设置最大发言数量
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "最大发言数："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.maxSpeechUnits //"0x0209"  //"1/4"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            Rectangle {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                border.width: Theme.borderWidthNone
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                SpinBox {
                                    id: speechUnits
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    editable: true
                                    from: 1
                                    to: 4
                                    value: {
                                        var maxSpeechUnits = hostManager.currentHost.maxSpeechUnits;
                                        if (maxSpeechUnits) {
                                            // 分割字符串并提取第一个数字
                                            var parts = maxSpeechUnits.split('/');
                                            if (parts.length > 0) {
                                                var val = parseInt(parts[0].trim());
                                                if (!isNaN(val) && val >= 1 && val <= 4) {
                                                    return val;
                                                }
                                            }
                                        }
                                        return 1; // 默认值
                                    }

                                    validator: IntValidator {
                                        bottom: 1
                                        top: 4
                                    }
                                    textFromValue: function(value) {
                                        return (value < 10 ? "0" + value : value);
                                    }
                                    valueFromText: function(text) {
                                        return parseInt(text.replace("0x", ""), 10);
                                    }
                                }
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            // console.log("最大发言数设定为:", speechUnits.value)
                            hostManager.currentHost.hostSet_maxSpeechUnits(speechUnits.value)
                        }
                    }
                }
                // 分隔线
                Image {
                    Layout.fillWidth: true
                    height: 1
                    source: "qrc:/image/icon/divider.svg"  // 1px 灰色图片
                    fillMode: Image.Stretch
                    smooth: false
                    antialiasing: false
                }

                // 0x0105-(    ) 设置无线音量
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "无线音量："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.wirelessVolume  //"0x0205"   //"7"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            Rectangle {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                border.width: Theme.borderWidthNone
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                SpinBox {
                                    id: spinWirelessVolume
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    editable: true
                                    from: 1
                                    to: 9
                                    value: {
                                        var wlessVolume = hostManager.currentHost.wirelessVolume + 1;
                                        if (wlessVolume) {
                                            var parts = wlessVolume.split('/');
                                            if (parts.length > 0) {
                                                var val = parseInt(parts[0].trim());
                                                if (!isNaN(val) && val >= 1 && val <= 9) {
                                                    return val;
                                                }
                                            }
                                        }
                                        return 1; // 默认值
                                    }

                                    validator: IntValidator {
                                        bottom: 1
                                        top: 9
                                    }
                                    textFromValue: function(value) {
                                        return (value < 10 ? "0" + value : value);
                                    }
                                    valueFromText: function(text) {
                                        return parseInt(text.replace("0x", ""), 10);
                                    }
                                }
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            // console.log("无线音量设定为:", spinWirelessVolume.value)
                            hostManager.currentHost.hostSet_wirelessVolume(spinWirelessVolume.value -1)
                        }
                    }

                }
                // 0x0106-(    ) 设置有线音量
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "有线音量："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.wiredVolume  //"0x0207"  //"7"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            Rectangle {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                border.width: Theme.borderWidthNone
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                SpinBox {
                                    id: spinWiredVolume
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    editable: true
                                    from: 1
                                    to: 7
                                    value:  {
                                        var wdVolume = hostManager.currentHost.wiredVolume + 1;
                                        if (wdVolume) {
                                            var parts = wdVolume.split('/');
                                            if (parts.length > 0) {
                                                var val = parseInt(parts[0].trim());
                                                if (!isNaN(val) && val >= 1 && val <= 7) {
                                                    return val;
                                                }
                                            }
                                        }
                                        return 1; // 默认值
                                    }

                                    validator: IntValidator {
                                        bottom: 1
                                        top: 7
                                    }
                                    textFromValue: function(value) {
                                        return (value < 10 ? "0" + value : value);
                                    }
                                    valueFromText: function(text) {
                                        return parseInt(text.replace("0x", ""), 10);
                                    }
                                }
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            // console.log("有线音量设定为:", spinWiredVolume.value)
                            hostManager.currentHost.hostSet_wiredVolume(spinWiredVolume.value -1)
                        }
                    }
                }
                // 0x0107-(    ) 设置天线盒音量
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "天线盒音量："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.antaBoxVolume  // "0x0107"  //"7"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            Rectangle {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                border.width: Theme.borderWidthNone
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                SpinBox {
                                    id: spinAntaboxVolume
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    editable: true
                                    from: 1
                                    to: 9
                                    value: {
                                        var aBoxVolume = hostManager.currentHost.antaBoxVolume + 1;
                                        if (aBoxVolume) {
                                            var parts = aBoxVolume.split('/');
                                            if (parts.length > 0) {
                                                var val = parseInt(parts[0].trim());
                                                if (!isNaN(val) && val >= 1 && val <= 9) {
                                                    return val;
                                                }
                                            }
                                        }
                                        return 1; // 默认值
                                    }

                                    validator: IntValidator {
                                        bottom: 1
                                        top: 9
                                    }
                                    textFromValue: function(value) {
                                        return (value < 10 ? "0" + value : value);
                                    }
                                    valueFromText: function(text) {
                                        return parseInt(text.replace("0x", ""), 10);
                                    }
                                }
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            //console.log("天线盒音量设定为:", spinAntaboxVolume.value)
                            hostManager.currentHost.hostSet_antaBoxVolume(spinAntaboxVolume.value -1)
                        }
                    }
                }
                // 分隔线
                Image {
                    Layout.fillWidth: true
                    height: 1
                    source: "qrc:/image/icon/divider.svg"  // 1px 灰色图片
                    fillMode: Image.Stretch
                    smooth: false
                    antialiasing: false
                }

                // 0x010D-(    ) 设置无线音频路径
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "无线音频路径："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.wirelessAudioPath  // "0x020D"   // "主机/天线盒"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            RadioButton {
                                id: pathHost
                                text: "主机"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.wirelessAudioPath === "Host"
                            }
                            RadioButton {
                                id: pathAntabox
                                text: "天线盒"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.wirelessAudioPath === "Antenna Box"
                            }
                            Item {
                                Layout.columnSpan: 2
                                Layout.preferredWidth: flexWidth*3
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            var selectedAudioPath = "";
                            if (pathHost.checked) {
                                selectedAudioPath = pathHost.text;
                                hostManager.currentHost.hostSet_wirelessAudioPath("Host")
                            } else if (pathAntabox.checked) {
                                selectedAudioPath = pathAntabox.text;
                                hostManager.currentHost.hostSet_wirelessAudioPath("Antenna Box")
                            }
                            // console.log("音频路径设置为:", selectedAudioPath)
                        }
                    }
                }
                // 分隔线
                Image {
                    Layout.fillWidth: true
                    height: 1
                    source: "qrc:/image/icon/divider.svg"  // 1px 灰色图片
                    fillMode: Image.Stretch
                    smooth: false
                    antialiasing: false
                }

                // 0x010B-(    ) 设置摄像跟踪地址
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "摄像机地址："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.cameraTrackingAddress     // "0x020B"   //"0x01"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            Rectangle {
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: Theme.controllerHeight
                                Layout.alignment: Qt.AlignVCenter
                                border.width: Theme.borderWidthNone
                                border.color: Theme.inputBorder
                                color: Theme.inputBackground
                                radius: Theme.borderRadius2

                                SpinBox {
                                    id: cameraAddress
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    editable: true
                                    from: 1
                                    to: 255
                                    value: {
                                        var address = hostManager.currentHost.cameraTrackingAddress;
                                        if (address) {
                                            // 如果是十六进制格式 (如 "0x04")
                                            if (address.startsWith("0x")) {
                                                var val = parseInt(address.substring(2), 16);
                                                if (!isNaN(val) && val >= 1 && val <= 255) {
                                                    return val;
                                                }
                                            }
                                            // 如果是十进制数字字符串
                                            var decVal = parseInt(address);
                                            if (!isNaN(decVal) && decVal >= 1 && decVal <= 255) {
                                                return decVal;
                                            }
                                        }
                                        return 1; // 默认值
                                    }

                                    validator: IntValidator {
                                        bottom: 1
                                        top: 255
                                    }
                                    textFromValue: function(value) {
                                        return "0x" + (value < 16 ? "0" + value.toString(16).toUpperCase() : value.toString(16).toUpperCase());
                                    }
                                    valueFromText: function(text) {
                                        if (text.startsWith("0x")) {
                                            return parseInt(text.substring(2), 16);
                                        }
                                        return parseInt(text, 10);
                                    }
                                }
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            // console.log("摄像机地址设定为:", cameraAddress.value)
                            hostManager.currentHost.hostSet_cameraTrackingAddress(cameraAddress.value)
                        }
                    }
                }
                // 0x010A-(    ) 设置摄像跟踪协议
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    // 第一行
                    Label {
                        text: "摄像机协议："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.cameraTrackingProtocol    // "0x020A"    // "PeLeCo-D"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            RadioButton {
                                id: cameraProtocol
                                text: "NONE"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingProtocol === "NONE"
                            }
                            RadioButton {
                                id: cameraProtocolVisca
                                text: "Visca"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingProtocol === "VISCA"
                            }
                            RadioButton {
                                id: cameraProtocolPelecoD
                                text: "Peleco-D"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingProtocol === "PELCO_D"
                            }
                            RadioButton {
                                id: cameraProtocolPelecoP
                                text: "Peleco-P"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingProtocol === "PELCO_P"
                            }
                            Item {
                                Layout.columnSpan: 0
                                Layout.preferredWidth: flexWidth*3
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            var selectedProtocol = "";
                            if (cameraProtocol.checked) {
                                selectedProtocol = cameraProtocol.text;
                                hostManager.currentHost.hostSet_cameraTrackingProtocol("NONE")
                            } else if (cameraProtocolVisca.checked) {
                                selectedProtocol = cameraProtocolVisca.text;
                                hostManager.currentHost.hostSet_cameraTrackingProtocol("VISCA")
                            }else if (cameraProtocolPelecoD.checked) {
                                selectedProtocol = cameraProtocolPelecoD.text;
                                hostManager.currentHost.hostSet_cameraTrackingProtocol("PELCO_D")
                            }else if (cameraProtocolPelecoP.checked) {
                                selectedProtocol = cameraProtocolPelecoP.text;
                                hostManager.currentHost.hostSet_cameraTrackingProtocol("PELCO_P")
                            }
                            // console.log("设置发言模式为:", selectedProtocol)
                        }
                    }
                }
                // 0x010C-(    ) 设置摄像跟踪波特率
                GridLayout {
                    columns: 12
                    columnSpacing: Theme.spacing15
                    rowSpacing: Theme.spacing15
                    Layout.fillWidth: true

                    Label {
                        text: "波特率："
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        text: hostManager.currentHost.cameraTrackingBaudrate + " bps"    // "0x020C"  //"115200"
                        font.family: Theme.fontName
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Theme.fontWeightLight

                        Layout.columnSpan: 2
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.preferredWidth: flexWidth * 2
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item {
                        Layout.columnSpan: 7
                        Layout.preferredWidth: flexWidth * 7
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.spacing5

                            RadioButton {
                                id: cameraBaudrate2400
                                text: "2400"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingBaudrate === "2400"
                            }
                            RadioButton {
                                id: cameraBaudrate4800
                                text: "4800"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingBaudrate === "4800"
                            }
                            RadioButton {
                                id: cameraBaudrate9600
                                text: "9600"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingBaudrate === "9600"
                            }
                            RadioButton {
                                id: cameraBaudrate115200
                                text: "115200"
                                font.family: Theme.fontName
                                font.pixelSize: Theme.fontSizeBody
                                font.weight: Theme.fontWeightLight
                                Layout.preferredWidth: flexWidth
                                checked: hostManager.currentHost.cameraTrackingBaudrate === "115200"
                            }
                            Item {
                                Layout.columnSpan: 0
                                Layout.preferredWidth: flexWidth * 3
                            }
                        }
                    }
                    Button {
                        Layout.columnSpan: 1
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: Theme.controllerHeight
                        Layout.alignment: Qt.AlignVCenter

                        text: "设定"
                        font.pixelSize: Theme.primaryColor
                        palette.button: Theme.backgroundColor
                        palette.buttonText: Theme.secondaryColor
                        palette.dark: "#17a81a"

                        onClicked: {
                            var selectedMode = "";
                            if (cameraBaudrate2400.checked) {
                                selectedMode = cameraBaudrate2400.text;
                                hostManager.currentHost.hostSet_cameraTrackingBaudrate(2400)
                            } else if (cameraBaudrate4800.checked) {
                                selectedMode = cameraBaudrate4800.text;
                                hostManager.currentHost.hostSet_cameraTrackingBaudrate(4800)
                            }else if (cameraBaudrate9600.checked) {
                                selectedMode = cameraBaudrate9600.text;
                                hostManager.currentHost.hostSet_cameraTrackingBaudrate(9600)
                            }else if (cameraBaudrate115200.checked) {
                                selectedMode = cameraBaudrate115200.text;
                                hostManager.currentHost.hostSet_cameraTrackingBaudrate(115200)
                            }
                            // console.log("设置发言模式为:", selectedMode)
                        }
                    }
                }
            }
        }

        // 显示主机高级功能
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.leftMargin:  Theme.margin10
            Layout.rightMargin: Theme.margin10
            Layout.topMargin: Theme.margin10
            color: Theme.bodyColor
            border.width: Theme.borderWidthThin
            border.color: Theme.borderColorDark
            radius: Theme.borderRadius3

            // 高级功能按钮
            RowLayout {
                height:parent.height
                width: parent.width
                spacing: Theme.spacing20

                Item {
                    Layout.preferredWidth: 5
                }

                Button {
                    text: "同步主机"
                    font.family: Theme.fontName
                    font.pixelSize: Theme.fontSizeNote
                    font.weight: Theme.fontWeightNormal
                    Layout.preferredHeight: Theme.controllerHeight
                    Layout.preferredWidth: 90

                    enabled: hostManager.currentHost !== null   // 必须有当前主机才可用

                    palette.buttonText: Theme.secondaryColor      // 设置按钮文字颜色

                    onClicked: {
                        // 检查是否存在当前主机
                        if (hostManager.currentHost) {
                            // 调用当前主机的查询版本方法
                            hostManager.currentHost.initializeHostProperties()
                            // console.log("已发送初始化主机请求")
                        } else {
                            // 如果没有当前主机，显示提示信息
                            // console.log("错误：没有设置当前主机，请先选择一个主机")
                        }
                    }
                }

                Button {
                    text:"保存设置"
                    font.family: Theme.fontName
                    font.pixelSize: Theme.fontSizeNote
                    font.weight: Theme.fontWeightNormal
                    Layout.preferredHeight: Theme.controllerHeight
                    Layout.preferredWidth: 90

                    enabled: hostManager.currentHost !== null   // 必须有当前主机才可用

                    palette.buttonText: Theme.secondaryColor      // 设置按钮文字颜色为深灰色

                    onClicked: {
                        if (hostManager.currentHost){
                            hostManager.currentHost.saveToCache()
                            // console.log("保存最新属性值")
                        } else {
                            // 如果没有当前主机，显示提示信息
                            // console.log("错误：没有设置当前主机，请先选择一个主机")
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
