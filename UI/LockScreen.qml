import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Window {
    id: lockScreenWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: false
    // 始终使用透明色创建窗口，使原生 HWND 一开始就具备 alpha 合成能力。
    // 非透明模式下，由下面的 Rectangle 提供不透明的黑色填充。
    color: "transparent"

    property string currentTime: ""
    property string unlockTime: ""
    property string remainingTime: ""
    property string backgroundImage: ""

    // 文字与隐藏控制
    // 每行文字颜色独立可配置
    property string promptColor: "#ffffffff"
    property string currentTimeColor: "#ffd3d3d3"
    property string unlockTimeColor: "#ffd3d3d3"
    property string remainingTimeColor: "#ffff6347"
    // 取值：top-left, top-center, top-right, center-left, center,
    //       center-right, bottom-left, bottom-center, bottom-right
    property string textPosition: "center"
    property string promptText: ""
    property bool hideCurrentTime: false
    property bool hideUnlockTime: false
    property bool hideRemainingTime: false

    // 类型判断 - 清理尾部空格
    property bool isTransparent: backgroundImage === "transparent"
    property bool isSolidColor: backgroundImage.startsWith("#") && backgroundImage.length >= 7
    property bool isImage: {
        if (isTransparent || isSolidColor) return false
        let lower = backgroundImage.toLowerCase()
        return lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp")
    }
    property bool isVideo: {
        if (isTransparent || isSolidColor || isImage) return false
        let lower = backgroundImage.toLowerCase()
        return lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mov") || lower.endsWith(".wmv") || lower.endsWith(".mkv")
    }

    // ===== 背景层 =====
    // 默认黑色填充：仅在非透明模式下显示，给图片/视频/未识别背景兜底
    Rectangle {
        anchors.fill: parent
        visible: !isTransparent && !isSolidColor
        color: "black"
    }

    Rectangle {
        anchors.fill: parent
        visible: isSolidColor
        color: isSolidColor ? backgroundImage : "black"
    }

    Image {
        anchors.fill: parent
        source: isImage ? backgroundImage : ""
        fillMode: Image.PreserveAspectFit
        visible: isImage
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignVCenter
    }

    // ===== 视频背景 - 核心修复：MediaPlayer 与 VideoOutput 同级，通过 id 引用 =====
    // 1. VideoOutput 先声明（作为可视化 Item）
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        visible: isVideo
    }

    // 2. MediaPlayer 后声明，通过 videoOutput 属性绑定到上面的 videoOutput
    MediaPlayer {
        id: mediaPlayer
        source: isVideo ? backgroundImage : ""
        audioOutput: AudioOutput { muted: false }
        loops: MediaPlayer.Infinite
        autoPlay: false
        videoOutput: videoOutput  // ✅ 正确：绑定已声明的 VideoOutput 实例 id

        onErrorOccurred: console.error("MediaPlayer 错误:", error, errorString)
    }

    // ===== 前景 UI 层 =====
    // 行内对齐方式跟随位置
    readonly property int textHAlign: {
        if (textPosition === "top-left" || textPosition === "center-left" || textPosition === "bottom-left")
            return Qt.AlignLeft
        if (textPosition === "top-right" || textPosition === "center-right" || textPosition === "bottom-right")
            return Qt.AlignRight
        return Qt.AlignHCenter
    }

    Item {
        id: textContainer
        z: 1
        readonly property int pad: 32

        // 内部 ColumnLayout 已根据父容器宽度自适应；这里让外层 Item 充满，
        // 仅在内部对 ColumnLayout 做 9 个位置的锚定，避免父子尺寸循环依赖。
        anchors.fill: parent
        anchors.leftMargin: pad
        anchors.rightMargin: pad
        anchors.topMargin: pad
        anchors.bottomMargin: pad

        ColumnLayout {
            id: textColumn
            spacing: 30

            // 9 种位置：通过水平 / 垂直锚点组合实现
            anchors.horizontalCenter: (textPosition === "top-center"
                                       || textPosition === "center"
                                       || textPosition === "bottom-center")
                                      ? parent.horizontalCenter : undefined
            anchors.verticalCenter: (textPosition === "center-left"
                                     || textPosition === "center"
                                     || textPosition === "center-right")
                                    ? parent.verticalCenter : undefined
            anchors.left: (textPosition === "top-left"
                           || textPosition === "center-left"
                           || textPosition === "bottom-left")
                          ? parent.left : undefined
            anchors.right: (textPosition === "top-right"
                            || textPosition === "center-right"
                            || textPosition === "bottom-right")
                           ? parent.right : undefined
            anchors.top: (textPosition === "top-left"
                          || textPosition === "top-center"
                          || textPosition === "top-right")
                         ? parent.top : undefined
            anchors.bottom: (textPosition === "bottom-left"
                             || textPosition === "bottom-center"
                             || textPosition === "bottom-right")
                            ? parent.bottom : undefined

            Label {
                text: promptText.length > 0 ? promptText : qsTr("Device Locked")
                font.pixelSize: 48
                font.bold: true
                color: promptColor
                wrapMode: Text.WordWrap
                horizontalAlignment: textHAlign
                Layout.alignment: textHAlign
                Layout.maximumWidth: textContainer.width
            }
            Label {
                visible: !hideCurrentTime
                text: qsTr("Current Time: ") + currentTime
                font.pixelSize: 32
                color: currentTimeColor
                horizontalAlignment: textHAlign
                Layout.alignment: textHAlign
            }
            Label {
                visible: !hideUnlockTime
                text: qsTr("Unlock Time: ") + unlockTime
                font.pixelSize: 32
                color: unlockTimeColor
                horizontalAlignment: textHAlign
                Layout.alignment: textHAlign
            }
            Label {
                visible: !hideRemainingTime
                text: qsTr("Remaining: ") + remainingTime
                font.pixelSize: 36
                font.bold: true
                color: remainingTimeColor
                horizontalAlignment: textHAlign
                Layout.alignment: textHAlign
            }
        }
    }

    // ===== 播放控制 =====
    onVisibleChanged: {
        if (visible && isVideo && mediaPlayer.source !== "") {
            if (mediaPlayer.playbackState !== MediaPlayer.PlayingState)
                mediaPlayer.play()
        } else {
            mediaPlayer.stop()
        }
    }

    onBackgroundImageChanged: {
        if (isVideo && lockScreenWindow.visible) mediaPlayer.play()
        else mediaPlayer.stop()
    }

    onClosing: function(close) { close.accepted = false }
}