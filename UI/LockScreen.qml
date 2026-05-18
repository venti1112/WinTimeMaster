import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: lockScreenWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Window
    visibility: Window.Hidden
    color: "black"

    // 由 C++ 动态更新的属性
    property string currentTime: ""
    property string unlockTime: ""
    property string remainingTime: ""

    // 禁止关闭
    onClosing: function(close) {
        close.accepted = false;
    }

    // 中间内容
    Item {
        anchors.centerIn: parent
        width: parent.width * 0.7
        height: parent.height * 0.6

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30

            Label {
                text: qsTr("Device Locked")
                font.pixelSize: 48
                font.bold: true
                color: "white"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("Current Time: ") + currentTime
                font.pixelSize: 32
                color: "lightgray"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("Unlock Time: ") + unlockTime
                font.pixelSize: 32
                color: "lightgray"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("Remaining: ") + remainingTime
                font.pixelSize: 36
                font.bold: true
                color: "tomato"
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}