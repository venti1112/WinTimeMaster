import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: aboutDialog
    modal: true
    standardButtons: Dialog.NoButton

    title: qsTr("About")
    width: 520
    height: 380
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    property var aboutCtrl: null

    footer: DialogButtonBox {
        contentItem: RowLayout {
            spacing: 12
            Button {
                id: checkUpdateBtn
                text: UpdateChecker.checking ? qsTr("Checking...") : (isLatest ? qsTr("Up to Date") : qsTr("Check for Updates"))
                enabled: !UpdateChecker.checking && !isLatest
                onClicked: UpdateChecker.checkForUpdates()
            }
            Item { Layout.fillWidth: true }
            Label {
                id: updateStatus
                text: ""
                color: "#A0A0A0"
                font.pixelSize: 12
                font.family: "Microsoft YaHei UI"
                visible: text.length > 0
            }
            Button {
                text: qsTr("Download Update")
                visible: UpdateChecker.downloadUrl.length > 0
                onClicked: UpdateChecker.openDownloadUrl()
            }
            Button {
                text: qsTr("Close")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: aboutDialog.close()
            }
        }
    }

    property bool isLatest: false

    Dialog {
        id: errorDialog
        modal: true
        standardButtons: Dialog.NoButton
        title: qsTr("Update Check Failed")
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        implicitWidth: 480

        property string errorMessage: ""

        contentItem: Label {
            text: errorDialog.errorMessage
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            color: "#E0E0E0"
        }

        footer: DialogButtonBox {
            contentItem: RowLayout {
                spacing: 12
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("OK")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: errorDialog.close()
                }
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        anchors.topMargin: 0
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            spacing: 24

            // 左侧：Logo
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 140
                spacing: 12

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 120
                    source: "qrc:/icon.ico"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            // 分隔线
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: -12
                Layout.rightMargin: -12
                width: 1
                height: 200
                color: "#3F3F3F"
            }

            // 右侧：信息
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: "WinTimeMaster"
                        font.pixelSize: 22
                        font.family: "Microsoft YaHei UI"
                        font.bold: true
                        color: "#E0E0E0"
                    }

                    Text {
                        text: qsTr("Version") + " " + (aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.appVersion : qsTr("Unknown"))
                        font.pixelSize: 13
                        font.family: "Microsoft YaHei UI"
                        color: "#A0A0A0"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#3F3F3F"
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 16

                    Text {
                        text: qsTr("Developer:")
                        color: "#A0A0A0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }
                    Text {
                        text: aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.developers : ""
                        color: "#E0E0E0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        text: qsTr("Qt Version:")
                        color: "#A0A0A0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }
                    Text {
                        text: aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.qtVersion : ""
                        color: "#E0E0E0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }

                    Text {
                        text: qsTr("Compiler:")
                        color: "#A0A0A0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }
                    Text {
                        text: aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.compiler : ""
                        color: "#E0E0E0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }

                    Text {
                        text: qsTr("Build Type:")
                        color: "#A0A0A0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }
                    Text {
                        text: aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.buildType : ""
                        color: "#E0E0E0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }

                    Text {
                        text: qsTr("GitHub:")
                        color: "#A0A0A0"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                    }
                    Text {
                        text: aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.githubUrl : ""
                        color: "#58A6FF"
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally(aboutDialog.aboutCtrl ? aboutDialog.aboutCtrl.githubUrl : "")
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: UpdateChecker

        function onUpdateAvailable(version, url, message) {
            isLatest = false;
            updateStatus.text = qsTr("New version %1 available!").arg(version);
            updateStatus.color = "#4CAF50";
        }

        function onNoUpdateAvailable() {
            isLatest = true;
            updateStatus.text = qsTr("You are using the latest version.");
            updateStatus.color = "#A0A0A0";
        }

        function onCheckFailed(error) {
            isLatest = false;
            updateStatus.text = "";
            errorDialog.errorMessage = qsTr("Check failed: %1").arg(error);
            errorDialog.open();
        }
    }
}
