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
                console.warn("Quitting program (password verified)");
                Qt.quit();
            } else if (actionType === "service") {
                passwordWindow.hide();
                LockController.toggleChecking();
            } else {
                console.warn("Opening settings (password verified)");
                passwordWindow.hide();
                SettingsWindow.show();
                SettingsWindow.raise();
                SettingsWindow.requestActivate();
            }
        } else {
            console.warn("Password verification failed, access denied");
            SettingsController.showPasswordError();
            passwordField.text = "";
            passwordField.forceActiveFocus();
        }
    }

    function showForAction(type) {
        actionType = type;
        passwordField.text = "";
        if (type === "quit") console.warn("Attempting to quit the program, password verification required");
        else if (type === "service") console.warn("Attempting to toggle service, password verification required");
        else console.warn("Attempting to open settings, password verification required");
        passwordWindow.show();
        passwordWindow.raise();
        passwordWindow.requestActivate();
        passwordField.forceActiveFocus();
    }

    onClosing: function(close) {
        close.accepted = false;
        if (actionType === "quit") console.warn("Quit operation cancelled, password verification not passed");
        else if (actionType === "service") console.warn("Service toggle cancelled, password verification not passed");
        else console.warn("Settings access cancelled, password verification not passed");
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
