import QtQuick 2.15

Item {
    id: root

    property color color: "#2196F3"
    property color borderColor: "transparent"
    property int borderWidth: 0
    property int radius: 10

    property bool angle1: true
    property bool angle2: true
    property bool angle3: true
    property bool angle4: true

    readonly property real r1: angle1 ? radius : 0
    readonly property real r2: angle2 ? radius : 0
    readonly property real r3: angle3 ? radius : 0
    readonly property real r4: angle4 ? radius : 0

    implicitWidth: 100
    implicitHeight: 100

    // Canvas 自带抗锯齿，通常比 Shape 更清晰
    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        renderTarget: Canvas.FramebufferObject  // 使用 FBO 渲染

        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;

            ctx.clearRect(0, 0, w, h);

            ctx.beginPath();

            // 左上
            ctx.moveTo(r1, 0);
            if (r1 > 0) {
                ctx.arc(r1, r1, r1, -Math.PI/2, Math.PI, true);
            } else {
                ctx.lineTo(0, 0);
            }

            // 左下
            ctx.lineTo(0, h - r4);
            if (r4 > 0) {
                ctx.arc(r4, h - r4, r4, Math.PI, Math.PI/2, true);
            } else {
                ctx.lineTo(0, h);
            }

            // 右下
            ctx.lineTo(w - r3, h);
            if (r3 > 0) {
                ctx.arc(w - r3, h - r3, r3, Math.PI/2, 0, true);
            } else {
                ctx.lineTo(w, h);
            }

            // 右上
            ctx.lineTo(w, r2);
            if (r2 > 0) {
                ctx.arc(w - r2, r2, r2, 0, -Math.PI/2, true);
            } else {
                ctx.lineTo(w, 0);
            }

            ctx.closePath();

            // 填充
            ctx.fillStyle = root.color;
            ctx.fill();

            // 边框
            if (root.borderWidth > 0) {
                ctx.lineWidth = root.borderWidth;
                ctx.strokeStyle = root.borderColor;
                ctx.stroke();
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // 属性变化时重绘
    onColorChanged: canvas.requestPaint()
    onBorderColorChanged: canvas.requestPaint()
    onBorderWidthChanged: canvas.requestPaint()
    onRadiusChanged: canvas.requestPaint()
    onAngle1Changed: canvas.requestPaint()
    onAngle2Changed: canvas.requestPaint()
    onAngle3Changed: canvas.requestPaint()
    onAngle4Changed: canvas.requestPaint()
}
