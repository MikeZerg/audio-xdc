pragma Singleton
import QtQuick

QtObject {
    //  ---------------- 主样式配置 --------------------
    // Main tones
    readonly property color background: "#161718"   // 主程序背景色
    readonly property color panel: "#262728"        // 主程序各个显示区域背景色

    readonly property color surface: "#2D2E2F"      // 控件背景色（按钮，文本框，下拉框等）
    readonly property color surfaceSelected: "#4B4A4A"  // 控件选择或者展开时的背景色

    // Border & line
    readonly property color seperatorLine: "#207D90"// 分隔线颜色
    readonly property color borderFilled: "#777777" // 填充矩形的分隔线颜色

    readonly property color borderLine: "#444242"   // 边框颜色
    readonly property color borderLineL: "#888484"  // 边框颜色(浅色)
    readonly property color borderLineW: "#CCC2C2"  // 边框颜色(浅白色)

    // Text
    readonly property color text: "#D4BE98"     // 主文本颜色
    readonly property color textDim: "#E1D2B7"  // 次要文本颜色
    readonly property color textMenu: "#207D90"  // 菜单文本颜色

    // Font
    readonly property string fontFamily: "Microsoft YaHei"
    readonly property int fontSize14: 14
    readonly property int fontSize12: 12
    readonly property int fontWeight12: 12

    // Table
    readonly property color tableBackground: "#232323"
    readonly property color tableFontColorHig: "#70AD47"
    readonly property color tableFontColorMid: "#0070C0"
    readonly property color tableFontColorLow: "#C65911"

    // Accent (即将废弃)
    readonly property color accent: "#207D90"
    readonly property color accent1: "#2ED9C3"       // 使用
    readonly property color accent2: "#1C6E7E"
    readonly property color accent3: "#2A4B47"

    // States
    readonly property color success: "#00AF69"
    readonly property color error: "#E05757"
    readonly property color warning: "#FFC000"
    readonly property color disabled: "#58616B"

    // Image mask color - 图片遮罩颜色（用于未选中状态）
    readonly property color imageMask: "#AAAAAA"    // 图片遮罩色（用于未选中图片的半透明覆盖）

    // Radius
    readonly property int radiusL: 4       // 使用
    readonly property int radiusS: 2       // 使用

    // ---------------- 投票选项颜色池 --------------------
    // 前3个为表决投票专用（赞成/弃权/反对），后面为自定义选项颜色
    readonly property var votingColorPalette: [
        "#4CAF50",      // [0] 赞成 - 绿色， value= -1
        "#FF9800",      // [1] 弃权 - 橙色， value= 0
        "#F44336",      // [2] 反对 - 红色， value= 1
        "#5BA0D9",      // [3] 自定义1 - 明亮蓝色， value= 2
        "#6BBF6B",      // [4] 自定义2 - 明亮绿色， value= 3
        "#D9A05B",      // [5] 自定义3 - 明亮橙黄， value= 4
        "#B87CD9",      // [6] 自定义4 - 明亮紫色， value= 5
        "#5BC8C8",      // [7] 自定义5 - 明亮青色， value= 6
        "#D97A5B",      // [8] 自定义6 - 明亮珊瑚， value= 7
        "#7AB87C",      // [9] 自定义7 - 柔和草绿， value= 8
        "#D95BA0",      // [10] 自定义8 - 明亮粉色， value= 9
        "#5B8CB8"       // [11] 自定义9 - 柔和天蓝， value= 10
    ]

    // ---------------- 主机模型样式配置 --------------------
    // Progress Bar
    readonly property color progressBar: "#4CAF50"
    readonly property color backgroudBar: "#333333"

    // 主文本 - 冷白带金属光泽
    readonly property color mText: "#D4BE98"
    readonly property color mTextDim: "#CDAA7D"
    readonly property color mTextHim: "#DBC9A9"

    // 高亮文本/警告 - 工业警示橙
    readonly property color mHighText: "#E05757"

    // 面板色 - 深灰拉丝金属质感
    readonly property color mPanel: "#656364"

    // 背景色 - 更深的工业灰
    readonly property color mBackground: "#191718"

    // 边框线条 - 冷银灰
    readonly property color mBorderLine: "#989898"

    // 进度条填充 - 工业蓝绿
    readonly property color mProgressBar: "#3A9E9E"

    // 进度条背景 - 深灰工业质感
    readonly property color mBackgroudBar: "#3A3A3A"
}
