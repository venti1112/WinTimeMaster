import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: settingsWindow
    visible: false
    width: 900
    height: 520
    minimumWidth: 900
    maximumWidth: 900
    minimumHeight: 520
    maximumHeight: 520
    title: qsTr("Settings")

    property string currentLang: LanguageSwitcher.currentLanguage
    property var timeRuleModel: SettingsController.timeRuleModel
    signal requestShowSettings()
    signal requestQuit()

    onClosing: function(close) {
        close.accepted = false;
        TrayManager.hideSettingsWindow();
        TrayManager.showTrayIcon();
    }

    function showWithPasswordCheck() {
        if (SettingsController.password === "") {
            settingsWindow.show();
            settingsWindow.raise();
            settingsWindow.requestActivate();
        } else {
            AuthWindow.showForAction("settings");
        }
    }

    function quitWithPasswordCheck() {
        if (SettingsController.password === "") {
            Qt.quit();
        } else {
            AuthWindow.showForAction("quit");
        }
    }

    function toggleServiceWithPasswordCheck() {
        if (SettingsController.password === "") {
            LockController.toggleChecking();
        } else {
            AuthWindow.showForAction("service");
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // ── 锁机规则标题 ──
        Label {
            text: qsTr("Rules")
            font.pixelSize: 16
            font.bold: true
        }

        // ── 带外框的规则列表（占据主要空间） ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.color: palette.mid
            border.width: 2
            radius: 4
            color: "transparent"

            ListView {
                id: ruleListView
                anchors.fill: parent
                anchors.margins: 1
                model: timeRuleModel
                clip: true
                spacing: 0

                ScrollBar.vertical: ScrollBar {
                    contentItem: Rectangle {
                        color: "#CCFFFFFF"   // 半透明深灰，比默认主题明显
                        radius: 4
                    }
                    width: 6
                }

                delegate: Rectangle {
                    width: ruleListView.width
                    height: 48
                    border.color: palette.mid
                    border.width: 1
                    color: "transparent"

                    ItemDelegate {
                        id: itemDelegate
                        anchors.fill: parent
                        text: {
                            let start = model.startTime.toLocaleTimeString(Qt.locale(), "HH:mm");
                            let end = model.endTime.toLocaleTimeString(Qt.locale(), "HH:mm");
                            let modeStr;
                            if (model.repeatMode === 2) {
                                let dayNames = [
                                    qsTr("Mon"), qsTr("Tue"), qsTr("Wed"),
                                    qsTr("Thu"), qsTr("Fri"), qsTr("Sat"), qsTr("Sun")
                                ];
                                let selected = [];
                                for (let i = 0; i < 7; ++i) {
                                    if (model.weekDays & (1 << i))
                                        selected.push(dayNames[i]);
                                }
                                modeStr = selected.join(", ");
                            } else {
                                modeStr = [qsTr("Once"), qsTr("Daily")][model.repeatMode];
                            }
                            let status = model.enabled ? qsTr("Enabled") : qsTr("Disabled");
                            return "%1 - %2  %3  [%4]".arg(start).arg(end).arg(modeStr).arg(status);
                        }
                        onClicked: {
                            ruleEditorDialog.ruleIndex = index;
                            ruleEditorDialog.ruleId = model.ruleId;
                            ruleEditorDialog.startTime = model.startTime;
                            ruleEditorDialog.endTime = model.endTime;
                            ruleEditorDialog.repeatMode = model.repeatMode;
                            ruleEditorDialog.weekDays = model.weekDays;
                            ruleEditorDialog.ruleEnabled = model.enabled;
                            ruleEditorDialog.open();
                        }
                    }

                    // 右键菜单
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: function(mouse) {
                            contextMenu.popup(itemDelegate, mouse.x, mouse.y);
                        }
                    }

                    Menu {
                        id: contextMenu
                        MenuItem {
                            text: qsTr("Edit")
                            onTriggered: {
                                ruleEditorDialog.ruleIndex = index;
                                ruleEditorDialog.ruleId = model.ruleId;
                                ruleEditorDialog.startTime = model.startTime;
                                ruleEditorDialog.endTime = model.endTime;
                                ruleEditorDialog.repeatMode = model.repeatMode;
                                ruleEditorDialog.weekDays = model.weekDays;
                                ruleEditorDialog.ruleEnabled = model.enabled;
                                ruleEditorDialog.open();
                            }
                        }
                        MenuItem {
                            text: qsTr("Delete")
                            onTriggered: {
                                SettingsController.timeRuleModel.removeRule(index);
                            }
                        }
                    }
                }
            }
        }

        // ── 底部操作栏 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: qsTr("Add Rule")
                onClicked: {
                    ruleEditorDialog.ruleIndex = -1;
                    ruleEditorDialog.ruleId = -1;
                    ruleEditorDialog.startTime = null;
                    ruleEditorDialog.endTime = null;
                    ruleEditorDialog.repeatMode = 1;
                    ruleEditorDialog.weekDays = 0;
                    ruleEditorDialog.ruleEnabled = true;
                    ruleEditorDialog.open();
                }
            }

            Button {
                text: LockController.checking ? qsTr("Stop Service") : qsTr("Start Service")
                onClicked: LockController.toggleChecking()
            }

            Rectangle {
                width: 14
                height: 14
                radius: 7
                color: LockController.checking ? "#4CAF50" : "#F44336"
                Layout.alignment: Qt.AlignVCenter
            }

            Label {
                text: LockController.checking ? qsTr("Service Running") : qsTr("Service Stopped")
                Layout.alignment: Qt.AlignVCenter
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Advanced Settings")
                onClicked: advancedSettingsDialog.open()
            }

            Button {
                text: qsTr("About")
                onClicked: aboutDialog.open()
            }
        }

        // 对话框实例
        RuleEditor {
            id: ruleEditorDialog
            ruleModel: timeRuleModel
        }
        AdvancedSettings {
            id: advancedSettingsDialog
        }
        AboutDialog {
            id: aboutDialog
            aboutCtrl: AboutController
        }
    }
}