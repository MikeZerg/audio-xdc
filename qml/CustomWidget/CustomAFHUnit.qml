// qml/CustomWidget/CustomAFHUnit.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: frequencyHoppingUnit
    width: 210
    height: 125

    // ========== 外部可调节属性 ==========
    property int volume: 68                     // 音量值 0-100
    property int batteryLevel: 75               // 电量 0-100
    property int leftVerticalLevel: 70          // 左侧竖直进度条填充值 0-100 (绿色)
    property int rightVerticalLevel: 45         // 右侧竖直进度条填充值 0-100 (绿色)
    property string unitText: "01"              // 右中文本
    property string groupText: "GROUP1"         // 右下角组号文本
    property string channelText: "CH01"         // 中间靠上文本
    property string frequencyText: "510.000MHz" // 中间下方文本

    // 音量图标源 (请替换为实际图标路径)
    property string volumeIconSource: "qrc:/icons/volume-high.svg"

    // ========== 主布局：上中下三部分 ==========
    ColumnLayout {
        anchors.fill: parent
        spacing: 3

        // ---------- 上部 (浅色) ----------
        CustomRectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.mPanel
            radius: 4
            angle1: true
            angle2: true
            angle3: false
            angle4: false

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 16

               // Item { Layout.preferredWidth: 2 }  // 弹性撑开

                // 左边两个文本标签: RF 和 AF
                RowLayout {
                    spacing: 4

                    Text {
                        text: "RF"
                        color: Theme.mText
                        font.pixelSize: 11
                        opacity: 0.7
                    }
                    Text {
                        text: "AF"
                        color: Theme.mText
                        font.pixelSize: 11
                        opacity: 0.7
                    }
                }

                Item { Layout.fillWidth: true }  // 弹性撑开

                // 右边: 音量图标 + 音量值
                RowLayout {
                    spacing: 6
                    Image {
                        width: 20
                        height: 20
                        source: "qrc:/image/app-hostSpeaker.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.7
                    }
                    Text {
                        text: volume + "%"
                        color: Theme.mText
                        font.pixelSize: 11
                        font.bold: true
                        opacity: 0.7
                    }
                }
                // Item { Layout.preferredWidth: 2 }  // 从4减小到2
            }
        }

        // ---------- 中部 (深色) 主要部分 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.mBackground

            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.bottomMargin: 1
                anchors.leftMargin: 10
                anchors.rightMargin: 14
                spacing: 12

                // 左侧两个竖直进度条 (各10格)
                RowLayout {
                    spacing: 4
                    Layout.preferredWidth: 36

                    // 左侧竖直进度条 (绿色) - 从下往上填充
                    VerticalBar10 {
                        id: leftBar
                        level: leftVerticalLevel
                        barColor: Theme.mProgressBar
                        Layout.fillHeight: true
                        Layout.preferredWidth: 13
                    }

                    // 右侧竖直进度条 (绿色) - 从下往上填充
                    VerticalBar10 {
                        id: rightBar
                        level: rightVerticalLevel
                        barColor: Theme.mProgressBar
                        Layout.fillHeight: true
                        Layout.preferredWidth: 13
                    }
                }

                // 中间文本区域 (从下到上三个文本标签)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 1

                    Item { Layout.fillHeight: true }

                    // 中间文本 (组号)
                    Text {
                        text: groupText
                        color: Theme.mText
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    }

                    // 顶部文本 (CH01)
                    Text {
                        text: channelText
                        color: Theme.mText
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    }

                    // 底部文本 (频率)
                    Text {
                        text: frequencyText
                        color: Theme.mHighText
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    }
                }

                // 右侧单元模块编号
                Item {
                    Layout.preferredWidth: 10
                }
                Text {
                    text: unitText
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
            Layout.preferredHeight: 24
            color: Theme.mPanel
            radius: 4
            angle1: false
            angle2: false
            angle3: true
            angle4: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                spacing: 16
                opacity: 0.8

                // 左边: 电池图标 (绿色填充表示电量)
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

                    // 右侧文本标签: "Unit Battery"
                    Text {
                        text: "TX Device"
                        color: Theme.mText
                        font.pixelSize: 10
                        font.bold: true
                        // opacity: 0.7
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

    // ========== 自定义10格竖直进度条组件 ==========
    component VerticalBar10 : Item {
        property int level: 0
        property color barColor: Theme.mProgressBar

        ColumnLayout {
            anchors.fill: parent
            spacing: 1

            // 从下往上构建10个格子
            Repeater {
                model: 10
                delegate: Rectangle {
                    Layout.preferredWidth: parent.width
                    Layout.fillHeight: true
                    color: {
                        // 最底部格子(索引9)阈值0%，最顶部格子(索引0)阈值90%
                        var threshold = (9 - index) * 10;

                        // 如果level超过了当前格子的阈值，进度条填充色
                        if (level > threshold) return barColor
                        else return Theme.mBackgroudBar  // 进度条背景
                    }
                    radius: 1
                }
            }
        }
    }
}
