// SystemArchitecture.qml (简化版)
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: systemArchitecture
    width: parent.width
    height: parent.height

    // 背景
    Rectangle {
        anchors.fill: parent
        color: Theme.background
    }

    // 顶部标志区域
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 20
        height: 60

        // iMeeting Plat标志
        Image {
            source: "qrc:/image/app-iMeetingPlat.png"
            Layout.preferredWidth: 180
            Layout.preferredHeight: 40
            fillMode: Image.PreserveAspectFit
        }

        // 产品图片
        Image {
            source: "qrc:/image/app-product.jpg"
            Layout.preferredWidth: 120
            Layout.preferredHeight: 40
            fillMode: Image.PreserveAspectFit
        }
    }

    // 特性说明区域
    Rectangle {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: Theme.surface1
        border.color: Theme.strokeSoft
        border.width: 1
        radius: Theme.radiusS
        padding: 16

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "Characteristics"
                color: Theme.text
                font.bold: true
                font.pixelSize: 16
            }

            Text {
                text: "* Wired and wireless integration, where the host uniformly speaks, schedules, and manages wired and wireless units."
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: "* The wireless subsystem has audio clock synchronization transmission technology, with a delay of less than 4ms. The wired subsystem uses uncompressed audio transmission to ensure audio quality. The wired subsystem adopts customized shielded 6-core cables."
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: "* The host can connect wirelessly to the conference system's wireless gateway (AP), expanding the wireless access range."
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: "* MIX channel volume control. The wireless subsystem includes 4 independent channel outputs and 1 MIX output. One MIX audio output from the wired subsystem*"
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: "* Support 4-8 units to speak simultaneously, with two subsystems configured uniformly."
                color: Theme.textDim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }

    // 顶部分隔线
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: Theme.background
    }
}