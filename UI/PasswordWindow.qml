import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: passwordWindow
    objectName: "passwordWindow"
    visible: false
    width: 380
    height: 140
    minimumWidth: 380
    maximumWidth: 380
    minimumHeight: 140
    maximumHeight: 140
    title: qsTr("Password Verification")
    flags: Qt.Window

    property string actionType: "settings"

    function verifyAndProceed() {
        if (SettingsController.verifyPassword(passwordField.text)) {
            if (actionType === "quit") {
                Qt.quit();
            } else if (actionType === "service") {
                passwordWindow.hide();
                LockController.toggleChecking();
            } else {
                passwordWindow.hide();
                SettingsWindow.show();
                SettingsWindow.raise();
                SettingsWindow.requestActivate();
            }
        } else {
            SettingsController.showPasswordError();
            passwordField.text = "";
            passwordField.forceActiveFocus();
        }
    }

    function showForAction(type) {
        actionType = type;
        passwordField.text = "";
        passwordWindow.show();
        passwordWindow.raise();
        passwordWindow.requestActivate();
        passwordField.forceActiveFocus();
    }

    onClosing: function(close) {
        close.accepted = false;
        passwordWindow.hide();
        if (actionType === "settings") {
            TrayManager.hideSettingsWindow();
            TrayManager.showTrayIcon();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.topMargin: 12
        anchors.bottomMargin: 16
        spacing: 16

        Label {
            text: actionType === "quit" ? qsTr("Enter password to quit:") : actionType === "service" ? qsTr("Enter password to toggle service:") : qsTr("Enter password to access settings:")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 15
        }

        TextField {
            id: passwordField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            placeholderText: qsTr("Password")
            onAccepted: passwordWindow.verifyAndProceed()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                onClicked: passwordWindow.close()
            }

            Button {
                text: qsTr("OK")
                onClicked: passwordWindow.verifyAndProceed()
            }
        }
    }
}
