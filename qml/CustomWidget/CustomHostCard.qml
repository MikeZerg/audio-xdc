import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: hostModel
    width: 210
    height: 140

    // ========== 外部可调节属性 ==========
    // 数据模型属性（参照 CustomHostUnit.qml，增加 unitCount）
    property var hostInfo: ({
        "address": "00",
        "aliasName": "XDC HOST",
        "status": 0,  // 0:离线 1:在线 2:活动
        "addressInt": 0,
        "lastSeen": "",
        "unitCount": 0,
        "volume": 50,
        "batteryLevel": 80
    })

    // 计算属性（参照 CustomHostUnit.qml）
    property string addressStr: hostInfo.address
    property string hostName: hostInfo.aliasName
    property int status: hostInfo.status
    property int addressInt: hostInfo.addressInt
    property int unitCount: hostInfo.unitCount
    property int volume: hostInfo.volume
    property int batteryLevel: hostInfo.batteryLevel
    property string hostAddress: "0x" + addressStr

    // 根据状态计算是否在线和是否活动
    property bool isOnline: status === 1 || status === 2
    property bool isActive: status === 2
    property color statusColor: {
        switch(status) {
            case 0: return "#708090";  // 离线 - 灰色
            case 1: return "#7393B3";  // 在线 - 青色
            case 2: return "#2E8B57";  // 活动 - 绿色
            default: return "pink";
        }
    }

    // Deleted:property string hostName: "XDC HOST"
    // Deleted:property string hostAddress: "01"              // 右中文本

    // ========== 主布局：上中下三部分 ==========
    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        // ---------- 上部 (浅色) ----------
        CustomRectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.mPanel
            radius: 2
            angle1: true
            angle2: true
            angle3: false
            angle4: false

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 16

                Text {
                    text: hostName
                    color: Theme.mText
                    font.pixelSize: 11
                    font.bold: true
                    opacity: 0.7
                }

                Item { Layout.fillWidth: true }  // 弹性撑开

                Text {
                    text: volume + "%"
                    color: Theme.mText
                    font.pixelSize: 12
                    font.bold: true
                    opacity: 0.7
                }

                Rectangle {
                    Layout.preferredHeight: 12
                    Layout.preferredWidth: 12
                    radius: 6
                    color: statusColor
                }
            }
        }

        // ---------- 中部 (深色) 主要部分 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.mBackground

            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 4
                anchors.bottomMargin: 12
                anchors.leftMargin: 10
                anchors.rightMargin: 20
                spacing: 10

                Image {
                    width: 64
                    height: 56
                    source: isOnline ? "qrc:/image/app-hostActive.svg" : "qrc:/image/app-hostInactive.svg"
                    sourceSize.width: 60
                    sourceSize.height: 52
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.9
                }

                // 中间文本区域 (从下到上三个文本标签)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6

                    Item {
                        Layout.fillHeight: 1
                        Layout.fillWidth: true
                    }

                    // 状态标签及文本
                    RowLayout {
                        Label {
                            text: "状态: "
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.mText
                        }
                        Text {
                            text: {
                                if (addressInt === 0) {
                                    "禁用"
                                } else {
                                    switch(status) {
                                        case 0: return "离线";
                                        case 1: return "在线";
                                        case 2: return "活动";
                                        default: return "未知";
                                    }
                                }
                            }
                            font.pixelSize: 11
                            font.bold: true
                            color: addressInt === 0 ? Theme.mTextDim : Theme.mText
                        }
                    }

                    // 单元数量显示
                    RowLayout {
                        Label {
                            text: "单元数: "
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.mText
                        }
                        Text {
                            text: unitCount > 0 ? unitCount : "--"
                            font.pixelSize: 11
                            font.bold: true
                            color: addressInt === 0 ? Theme.mTextDim : Theme.mText
                        }
                    }
                }

                // 右侧单元模块编号（地址信息）
                Item {
                    Layout.preferredWidth: 10
                }
                Text {
                    text: hostAddress
                    color: Theme.mText
                    font.pixelSize: 18
                    font.bold: true
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight
                    Layout.preferredWidth: 16
                }
            }
        }

        // ---------- 下部 (浅色) ----------
        CustomRectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.mPanel
            radius: 2
            angle1: false
            angle2: false
            angle3: true
            angle4: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                spacing: 16
                opacity: 0.8

                // 左边：电池图标 (绿色填充表示电量)
                RowLayout {
                    spacing: 8

                    Rectangle {
                        width: 26
                        height: 12
                        color: "transparent"
                        border.color: Theme.mBorderLine
                        border.width: 1
                        radius: 3

                        // 电池头
                        Rectangle {
                            width: 2
                            height: 4
                            color: Theme.mBorderLine
                            anchors.left: parent.right
                            anchors.leftMargin: 1
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 绿色电量填充
                        Rectangle {
                            width: parent.width * 0.8 * (batteryLevel / 100)
                            height: parent.height - 4
                            color: Theme.mProgressBar
                            anchors.left: parent.left
                            anchors.leftMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            radius: 2
                        }
                    }

                    // 右侧文本标签："Unit Battery"
                    Text {
                        text: "Speaker Id"
                        color: Theme.mText
                        font.pixelSize: 10
                        font.bold: true

                    }
                }

                //Item { Layout.fillWidth: true }

                // 右下角显示组号
                // Text {
                //     text: "TX"
                //     color: Theme.mText
                //     font.pixelSize: 12
                //     font.bold: true
                // }
            }
        }
    }
}
