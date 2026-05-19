import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Window {
    id: lockScreenWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: false
    color: "black"

    property string currentTime: ""
    property string unlockTime: ""
    property string remainingTime: ""
    property string backgroundImage: ""

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

    Component.onCompleted: updateColor()
    function updateColor() {
        lockScreenWindow.color = isTransparent ? "transparent" : "black"
    }

    // ===== 背景层 =====
    Rectangle {
        anchors.fill: parent
        visible: isSolidColor
        color: backgroundImage
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
    Item {
        anchors.centerIn: parent
        width: parent.width * 0.7
        height: parent.height * 0.6
        z: 1

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30
            Label { text: qsTr("Device Locked"); font.pixelSize: 48; font.bold: true; color: "white"; Layout.alignment: Qt.AlignHCenter }
            Label { text: qsTr("Current Time: ") + currentTime; font.pixelSize: 32; color: "lightgray"; Layout.alignment: Qt.AlignHCenter }
            Label { text: qsTr("Unlock Time: ") + unlockTime; font.pixelSize: 32; color: "lightgray"; Layout.alignment: Qt.AlignHCenter }
            Label { text: qsTr("Remaining: ") + remainingTime; font.pixelSize: 36; font.bold: true; color: "tomato"; Layout.alignment: Qt.AlignHCenter }
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
        updateColor()
        if (isVideo && lockScreenWindow.visible) mediaPlayer.play()
        else mediaPlayer.stop()
    }

    onClosing: function(close) { close.accepted = false }
}