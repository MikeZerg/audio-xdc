import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./CustomWidget"

ColumnLayout {
    id: host_FrequencyHopping_page
    Layout.preferredWidth: parent.width * 0.90
    Layout.preferredHeight: parent.height * 0.90
    spacing: 0

    // 顶部标题部分
    Item {
        id: areaTitle
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25

        Text {
            text: "Frequency Hopping Monitoring"
            font.pixelSize: 16
            color: Theme.text

            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
        }
    }

    // 布局占位
    Item {
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 40
    }

    // 主要内容区域
    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 20
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 20

            // 使用GridLayout放置两行两列
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 5

                // 第一个单元 (第1行第1列)
                CustomAFHUnit {
                    id: unit1
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 140
                    Layout.alignment: Qt.AlignHCenter

                    volume: 72
                    batteryLevel: 68
                    leftVerticalLevel: 80
                    rightVerticalLevel: 35
                    unitText: "01"
                    groupText: "GROUP A"
                    channelText: "CH05"
                    frequencyText: "720.500MHz"
                    volumeIconSource: "qrc:/image/app-hostSpeaker.svg"
                }

                // 第二个单元 (第1行第2列)
                CustomAFHUnit {
                    id: unit2
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 140
                    Layout.alignment: Qt.AlignHCenter

                    volume: 45
                    batteryLevel: 92
                    leftVerticalLevel: 60
                    rightVerticalLevel: 90
                    unitText: "02"
                    groupText: "GROUP B"
                    channelText: "CH12"
                    frequencyText: "163.350MHz"
                    volumeIconSource: "qrc:/image/app-hostSpeaker.svg"
                }

                // 第三个单元 (第2行第1列)
                CustomAFHUnit {
                    id: unit3
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 140
                    Layout.alignment: Qt.AlignHCenter

                    volume: 90
                    batteryLevel: 45
                    leftVerticalLevel: 95
                    rightVerticalLevel: 20
                    unitText: "03"
                    groupText: "GROUP C"
                    channelText: "CH08"
                    frequencyText: "245.750MHz"
                    volumeIconSource: "qrc:/image/app-hostSpeaker.svg"
                }

                // 第四个单元 (第2行第2列)
                CustomAFHUnit {
                    id: unit4
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 140
                    Layout.alignment: Qt.AlignHCenter

                    volume: 30
                    batteryLevel: 85
                    leftVerticalLevel: 40
                    rightVerticalLevel: 75
                    unitText: "04"
                    groupText: "GROUP D"
                    channelText: "CH15"
                    frequencyText: "890.125MHz"
                    volumeIconSource: "qrc:/image/app-hostSpeaker.svg"
                }
            }

            // 控制面板
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "transparent"
                radius: 6
                Layout.topMargin: 20

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "Control Panel"
                        color: Theme.text
                        font.bold: true
                        font.pixelSize: 14
                    }

                    // 第一行控制按钮
                    RowLayout {
                        spacing: 10

                        CustomButton {
                            text: "Update Unit 1"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                unit1.volume = Math.floor(Math.random() * 101)
                                unit1.batteryLevel = Math.floor(Math.random() * 101)
                                unit1.leftVerticalLevel = Math.floor(Math.random() * 101)
                                unit1.rightVerticalLevel = Math.floor(Math.random() * 101)
                            }
                        }
                        CustomButton {
                            text: "Update Unit 2"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                unit2.volume = Math.floor(Math.random() * 101)
                                unit2.batteryLevel = Math.floor(Math.random() * 101)
                                unit2.leftVerticalLevel = Math.floor(Math.random() * 101)
                                unit2.rightVerticalLevel = Math.floor(Math.random() * 101)
                            }
                        }
                        CustomButton {
                            text: "Update Unit 3"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                unit3.volume = Math.floor(Math.random() * 101)
                                unit3.batteryLevel = Math.floor(Math.random() * 101)
                                unit3.leftVerticalLevel = Math.floor(Math.random() * 101)
                                unit3.rightVerticalLevel = Math.floor(Math.random() * 101)
                            }
                        }
                        CustomButton {
                            text: "Update Unit 4"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                unit4.volume = Math.floor(Math.random() * 101)
                                unit4.batteryLevel = Math.floor(Math.random() * 101)
                                unit4.leftVerticalLevel = Math.floor(Math.random() * 101)
                                unit4.rightVerticalLevel = Math.floor(Math.random() * 101)
                            }
                        }
                    }

                    // 第二行控制按钮
                    RowLayout {
                        spacing: 10

                        CustomButton {
                            text: "Reset All"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                // 单元1
                                unit1.volume = 72
                                unit1.batteryLevel = 68
                                unit1.leftVerticalLevel = 80
                                unit1.rightVerticalLevel = 35

                                // 单元2
                                unit2.volume = 45
                                unit2.batteryLevel = 92
                                unit2.leftVerticalLevel = 60
                                unit2.rightVerticalLevel = 90

                                // 单元3
                                unit3.volume = 90
                                unit3.batteryLevel = 45
                                unit3.leftVerticalLevel = 95
                                unit3.rightVerticalLevel = 20

                                // 单元4
                                unit4.volume = 30
                                unit4.batteryLevel = 85
                                unit4.leftVerticalLevel = 40
                                unit4.rightVerticalLevel = 75
                            }
                        }
                        CustomButton {
                            text: "Random All"
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 24
                            onClicked: {
                                var units = [unit1, unit2, unit3, unit4]
                                for (var i = 0; i < units.length; i++) {
                                    units[i].volume = Math.floor(Math.random() * 101)
                                    units[i].batteryLevel = Math.floor(Math.random() * 101)
                                    units[i].leftVerticalLevel = Math.floor(Math.random() * 101)
                                    units[i].rightVerticalLevel = Math.floor(Math.random() * 101)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        id: freqCharts
        Layout.preferredHeight: 300
        Layout.fillWidth: true
        spacing: 10

        // 这里对应四个CH通道，放置四个频率与时间响应的图表，横轴为时间，纵轴为频率
        // CH01 freq-time Chart

        // CH01 freq-time Chart

        // CH01 freq-time Chart

        // CH01 freq-time Chart

    }
}
