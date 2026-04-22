import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15


ColumnLayout {
    id: mainColumn
    Layout.preferredWidth: parent.width * 0.95
    Layout.preferredHeight: parent.height * 0.95
    spacing: 20

    // 当前选中的产品索引（0 或 1）
    property int selectedProductIndex: 0

    // 产品数据数组
    readonly property var productsData: [
        {
            title: "iweex® | iMeetingPlot™",
            features: "<p><b>产品特性</b></p>" +
                    "<ul>" +
                     "<li>有线无线融合，主机对有线单元、无线单元统一发言调度、管理。</li>" +
                    "<li>语音U段数字通信; 管理2.4G数字通信。语音PI/4 DQPSK调制 ；管理GFSK调制。</li>" +
                    "<li>无线子系统具有音频时钟同步传输技术，延时小于 4ms。有线子系统音频采用非压缩音频传输，确保音频质量。有线子系统采用定制屏蔽6芯线缆。</li>" +
                    "<li>主机可有线连接会议系统无线网关（AP），扩展无线接入范围。</li>" +
                    "<li>MIX声道音量控制。无线子系统包含 4路独立通道输出和1路MIX输出。有线子系统一路MIX音频输出。 *</li>" +
                    "<li>支持4~8个单元同时讲话，两个子系统统一配置。</li>" +
                    "<li>基于iMeetingPlat™技术平台，带来全新用户体验。会议状态、配置一目了然。</li>" +
                    "<li>会议模式：先进先出 / 数量限制。</li>" +
                    "<li>日期、时间、星期设置 / 音量和亮度设置 / 语言设置 / 单元编号 / 无线环境扫描 / Visca、Peleco-D、Peleco-P 摄像机协议。</li>" +
                    "<li>支持主席、VIP、列席三种角色，软件定义，成员属性管理轻松便捷。</li>" +
                    "<li>音频信道、管理信道、信号强度可视化</li>" +
                    "<li>系统终端特有的长横条钢琴键发言按键，静音设计，极佳体验。</li>" +
                    "</ul>",
            wired: "<p><b>有线子系统</b></p>" +
                    "<ul>" +
                    "<li>采用高速6芯数字电缆传输数据及供电，传输距离最远可达150米。</li>" +
                    "<li>采样频率 48KHz，频率响应20Hz-20kHz，达到 CD 级音质效果。</li>" +
                    "<li>有线子系统4路输入，每路最多接32台，最大支持128台单元。</li>" +
                    "<li>支持环型手拉手功能。具备热插拔保护，短路保护功能。</li>" +
                    "<li>系统终端采用金属结构屏蔽设计和电路抗干扰设计，不易受信号干扰。</li>" +
                    "</ul>",
            wireless: "<p><b>无线子系统</b></p>" +
                    "<ul>" +
                    "<li>实时扫频，自动切频技术。单元在每次发言时使用最优频点。</li>" +
                    "<li>支持无线网关接入，无线接入范围不再受主机位置局限。</li>" +
                    "<li>无线网关支持最多256个无线发言单元。</li>" +
                    "<li>支持电容咪芯手持式发言单元，手持式终端自动休眠，3D动作唤醒。</li>" +
                    "<li>无线终端采用'混合动力'专利电源技术，锂电池、5号电池自由切换使用。</li>" +
                    "</ul>"
        },
        {
            title: "iweex® | XDC系列",
            features: "<p><b>产品特性</b></p>" +
                    "<ul>" +
                    "<li>全数字音频处理技术，提供卓越的音质表现。</li>" +
                    "<li>智能音频路由，支持多通道灵活配置。</li>" +
                    "<li>内置DSP处理器，支持EQ、压限、反馈抑制等音频处理。</li>" +
                    "<li>网络化管理，支持远程监控和配置。</li>" +
                    "<li>模块化设计，易于扩展和维护。</li>" +
                    "<li>低功耗设计，节能环保。</li>" +
                    "<li>工业级品质，7×24小时稳定运行。</li>" +
                    "</ul>",
            wired: "<p><b>有线连接</b></p>" +
                    "<ul>" +
                    "<li>支持多种接口类型：XLR、TRS、Phoenix等。</li>" +
                    "<li>平衡传输，抗干扰能力强。</li>" +
                    "<li>支持长线传输，最远可达300米。</li>" +
                    "<li>即插即用，自动识别设备类型。</li>" +
                    "</ul>",
            wireless: "<p><b>无线功能</b></p>" +
                    "<ul>" +
                    "<li>2.4GHz频段，自动跳频技术。</li>" +
                    "<li>低延迟传输，延时小于5ms。</li>" +
                    "<li>支持多点并发，最多16个无线通道。</li>" +
                    "<li>智能功率控制，延长电池寿命。</li>" +
                    "<li>加密传输，保障信息安全。</li>" +
                    "</ul>"
        },
        {
            title: "iweex® | DS240 AFH数字无线系统",
            features: "<p><b>产品特性</b></p>" +
                    "<ul>" +
                    "<li>基于DS240自适应跳频数字U段技术平台（AFH），GAODIMIC® 2025年最新数字音频U段无线技术。</li>" +
                    "<li>支持手动跳频与自动跳频双模式：手动跳频可人工发起，无线信号自动跳转到干净频点；自动跳频模式下系统自动识别干扰并自动跳转。</li>" +
                    "<li>超宽频段覆盖：510MHz~698MHz，可根据不同国家和地区设置，提供不同的无线频率组合以符合当地法规。</li>" +
                    "<li>超长距离无线覆盖：通过无线接入点设备（AP）实现无线信号远距离覆盖，轻松实现140米无线覆盖范围。</li>" +
                    "<li>超远距离信号传输：使用网线实现有线+无线混合传输，最远可达90米（有线）+50米（无线）。</li>" +
                    "<li>混合动力专利电源技术（Patent V2.0）：支持14500锂电池、18650锂电池、5号电池自由切换使用，无需手动设置。</li>" +
                    "<li>第4代数字U段无线技术，自适应跳频（AFH）确保通信稳定可靠。</li>" +
                    "<li>高性价比，工程项目的最佳选择，易于部署，为演讲者带来专业音效。</li>" +
                    "<li>自动频率跳频工作模式可解决断线问题，即使现场无维护也能稳定运行。</li>" +
                    "</ul>",
            wireless: "<p><b>无线子系统</b></p>" +
                    "<ul>" +
                    "<li>DS243M 手持式无线话筒：通信信噪比106dB，本底噪声<18uV，频响50Hz~16kHz，数字分辨率24bit，系统延时4.17ms，灵敏度-51±2dBV/Pa。适用于会议室、教室、操场、超市。</li>" +
                    "<li>DS245M 手持式无线话筒：通信信噪比112dB，本底噪声<12uV，频响50Hz~16kHz，数字分辨率24bit，系统延时4.17ms，灵敏度-51±2dBV/Pa。适用于会议室、教室、操场、超市、直播、大型会议、小型剧院、小型专业演出。</li>" +
                    "<li>DS244M-TS 直杆式无线话筒：通信信噪比102dB，本底噪声<18uV，频响40Hz~16kHz，数字分辨率24bit，系统延时4.17ms，灵敏度-45±2dBV/Pa。适用于高端会议室。</li>" +
                    "<li>支持2通道、4通道、8通道套装配置（DS243MG/DS245MG/DS244MG-TG系列）。</li>" +
                    "<li>无线接入点（AP）支持510-698MHz U段无线接入，单AP无线覆盖半径>50米。</li>" +
                    "</ul>",
            wired: "<p><b>有线连接</b></p>" +
                    "<ul>" +
                    "<li>主机通过RJ45接口连接无线接入点（AP），支持网线供电（PoE）。</li>" +
                    "<li>网线传输距离：Network Cable 1 支持>90米（带供电），Network Cable 2 支持>40米。</li>" +
                    "<li>主机具备MIX 8路输出和1路AP（RJ45）接口。</li>" +
                    "<li>DC 12V 1A电源供电。</li>" +
                    "</ul>"
        }
    ]

    // 1. 图标部分
    Rectangle {
        id: iconSection
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 25
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            spacing: 10

            // LOGO图标
            Image {
                id: logoProduct
                source: "qrc:/image/app-iMeetingPlat.png"
                Layout.preferredWidth: 300/1.2
                Layout.preferredHeight: 64/1.2
                fillMode: Image.PreserveAspectFit
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 5
    }

    // 2. 产品图和箭头部分
    Rectangle {
        id: productImageSection
        Layout.preferredWidth: parent.width
        Layout.preferredHeight: 44.6*4
        color: Theme.surface
        border.color: Theme.borderLine
        border.width: 1
        radius: 2

        // 滚动位置（像素偏移）
        property real scrollOffset: 0
        property real maxScrollOffset: Math.max(0, productRow.implicitWidth - (productImageSection.width - 36))

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 左侧箭头
            Rectangle {
                id: preButton
                Layout.preferredWidth: 18
                Layout.preferredHeight: parent.height -1
                Layout.alignment: Qt.AlignVCenter

                color: "transparent"
                border.color: Theme.borderLine
                border.width: 0.5

                Text {
                    text: String.fromCharCode(237)
                    font.family: "Wingdings 3"
                    font.pixelSize: 24
                    color: Theme.text
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (productImageSection.scrollOffset > 0) {
                            productImageSection.scrollOffset = Math.max(0, productImageSection.scrollOffset - 100)
                        }
                    }

                    onEntered: parent.border.color = Theme.borderLineL
                    onExited: parent.border.color = Theme.borderLine
                }
            }

            // 产品图滚动区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"
                clip: true

                RowLayout {
                    id: productRow
                    anchors.left: parent.left
                    anchors.leftMargin: -productImageSection.scrollOffset
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    // 产品图 0
                    Image {
                        id: productImage0
                        source: "qrc:/image/app-product1.png"
                        Layout.preferredWidth: 139.5*4
                        Layout.preferredHeight: 44.6*4-2
                        fillMode: Image.Stretch

                        // 添加半透明遮罩层 - 未选中时显示遮罩，选中时隐藏
                        Rectangle {
                            anchors.fill: parent
                            color: Theme.imageMask
                            opacity: selectedProductIndex === 0 ? 0 : 0.3
                            radius: 2
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectedProductIndex = 0
                            }

                            hoverEnabled: true
                            onEntered: {
                                if (selectedProductIndex !== 0) {
                                    parent.opacity = 0.85
                                }
                            }
                            onExited: {
                                parent.opacity = 1.0
                            }
                        }
                    }

                    // 产品图 1
                    Image {
                        id: productImage1
                        source: "qrc:/image/app-product.jpg"
                        Layout.preferredWidth: 72.6*4
                        Layout.preferredHeight: 44.6*4-2
                        fillMode: Image.Stretch

                        // 添加半透明遮罩层 - 未选中时显示遮罩，选中时隐藏
                        Rectangle {
                            anchors.fill: parent
                            color: Theme.imageMask
                            opacity: selectedProductIndex === 1 ? 0 : 0.5
                            radius: 2
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectedProductIndex = 1
                            }

                            hoverEnabled: true
                            onEntered: {
                                if (selectedProductIndex !== 1) {
                                    parent.opacity = 0.85
                                }
                            }
                            onExited: {
                                parent.opacity = 1.0
                            }
                        }
                    }

                    // 产品图 2
                    Image {
                        id: productImage2
                        source: "qrc:/image/app-product2.jpg"
                        Layout.preferredHeight: 44.6*4-2
                        Layout.preferredWidth: Layout.preferredHeight * (900 / 506)
                        fillMode: Image.PreserveAspectFit

                        // 添加半透明遮罩层 - 未选中时显示遮罩，选中时隐藏
                        Rectangle {
                            anchors.fill: parent
                            color: Theme.imageMask
                            opacity: selectedProductIndex === 2 ? 0 : 0.5
                            radius: 2
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectedProductIndex = 2
                            }

                            hoverEnabled: true
                            onEntered: {
                                if (selectedProductIndex !== 2) {
                                    parent.opacity = 0.85
                                }
                            }
                            onExited: {
                                parent.opacity = 1.0
                            }
                        }
                    }
                }
            }

            // 右侧箭头
            Rectangle {
                id: nextButton
                Layout.preferredWidth: 18
                Layout.preferredHeight: parent.height -1
                Layout.alignment: Qt.AlignVCenter

                color: "transparent"
                border.color: Theme.borderLine
                border.width: 0.5

                Text {
                    text: String.fromCharCode(238)
                    font.family: "Wingdings 3"
                    font.pixelSize: 24
                    color: Theme.text
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (productImageSection.scrollOffset < productImageSection.maxScrollOffset) {
                            productImageSection.scrollOffset = Math.min(productImageSection.maxScrollOffset, productImageSection.scrollOffset + 100)
                        }
                    }

                    onEntered: parent.border.color = Theme.borderLineL
                    onExited: parent.border.color = Theme.borderLine
                }
            }
        }
    }

    // 3. 文本部分 - 填充剩余空间
    Rectangle {
        id: textSection
        Layout.preferredWidth: parent.width
        Layout.fillHeight: true

        color: Theme.surface
        border.color: Theme.borderLine
        border.width: 1
        radius: 2

        ScrollView {
            id: textScrollView
            anchors.fill: parent
            anchors.margins: 10

            ScrollBar.vertical: ScrollBar {
                parent: textScrollView
                x: textScrollView.mirrored ? 0 : textScrollView.width - width
                y: textScrollView.topPadding
                height: textScrollView.availableHeight
                active: textScrollView.ScrollBar.horizontal.active

                background: Rectangle {
                    color: "transparent"
                }

                contentItem: Rectangle {
                    color: "transparent"
                    border.color: "transparent"
                }
            }

            ScrollBar.horizontal: ScrollBar {
                parent: textScrollView
                x: textScrollView.leftPadding
                y: textScrollView.height - height
                width: textScrollView.availableWidth
                active: textScrollView.ScrollBar.vertical.active

                background: Rectangle {
                    color: "transparent"
                }

                contentItem: Rectangle {
                    color: "transparent"
                    border.color: "transparent"
                }
            }

            TextArea {
                id: descriptionText
                width: parent.width
                wrapMode: Text.WordWrap
                readOnly: true
                textFormat: Text.RichText
                font.pixelSize: 12
                color: Theme.text

                // 根据选中的产品索引动态显示文本
                text: {
                    if (selectedProductIndex >= 0 && selectedProductIndex < productsData.length) {
                        const product = productsData[selectedProductIndex]
                        return "<style>" +
                               "body { line-height: 1.1; margin: 0; padding: 0; }" +
                               "h2 { font-size: 14px; font-weight: bold; margin-top: 15px; margin-bottom: 10px; color: #D4BE98; }" +
                               "ul { margin: 5px 0; padding-left: 20px; }" +
                               "li { margin-bottom: 8px; }" +
                               ".section-title { font-weight: bold; color: #D4BE98; margin-top: 20px; margin-bottom: 10px; font-size: 12px; }" +
                               "</style>" +
                               "<p><b><span style='font-size:18px;'>" + product.title + "</span></b></p>" +
                               product.features +
                               product.wired +
                               product.wireless
                    }
                    return ""
                }

                horizontalAlignment: Text.AlignLeft
                background: null

                // 当文本变化时滚动到顶部 - 使用正确的属性
                onTextChanged: {
                    // ScrollView 的 contentItem 才是实际的内容区域
                    if (textScrollView.contentItem) {
                        textScrollView.contentItem.contentY = 0
                    }
                }
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
    }
}