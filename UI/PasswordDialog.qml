import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: passwordDialog
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: isSettingNew ? qsTr("Set Password") : qsTr("Enter Password")

    property bool isSettingNew: false       // true: 设置新密码; false: 验证
    property string resultPassword: ""      // 输出的密码（验证时不使用）

    width: 360
    height: 360
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            text: isSettingNew ? qsTr("Please set a password (leave empty to disable):")
                                : qsTr("Please enter password to unlock settings:")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        TextField {
            id: passwordField
            echoMode: TextInput.Password
            placeholderText: qsTr("Password")
            Layout.fillWidth: true
            inputMethodHints: Qt.ImhHiddenText
        }

        // 设置新密码时，增加确认框
        TextField {
            id: confirmField
            visible: isSettingNew
            echoMode: TextInput.Password
            placeholderText: qsTr("Confirm Password")
            Layout.fillWidth: true
        }
    }

    onAccepted: {
        if (isSettingNew) {
            if (passwordField.text !== confirmField.text) {
                // 弹出错误提示（简单处理）
                warningLabel.visible = true;
                return;
            }
            resultPassword = passwordField.text;
        } else {
            resultPassword = passwordField.text;
        }
        close();
    }

    onRejected: {
        resultPassword = "";
        close();
    }

    Label {
        id: warningLabel
        visible: false
        text: qsTr("Passwords do not match!")
        color: "red"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}