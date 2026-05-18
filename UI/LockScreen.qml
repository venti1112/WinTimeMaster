import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: lockScreenWindow
    // 关键标志：无边框、置顶、工具窗口（不出现任务栏，跨虚拟桌面）
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: false                           // 由 C++ 控制显示
    color: "black"

    property string currentTime: ""
    property string unlockTime: ""
    property string remainingTime: ""

    onClosing: function(close) {
        close.accepted = false;              // 禁止关闭
    }

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