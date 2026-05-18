import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: settingsWindow
    visible: true
    width: 800
    height: 480
    title: qsTr("Settings")

    property string currentLang: LanguageSwitcher.currentLanguage
    property var timeRuleModel: SettingsController.timeRuleModel

    onClosing: function(close) {
        close.accepted = false;
        TrayManager.hideSettingsWindow();
        TrayManager.showTrayIcon();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // ── 锁机规则标题 ──
        Label {
            text: qsTr("Lock Time Rules")
            font.pixelSize: 16
            font.bold: true
        }

        // ── 带外框的规则列表（占据主要空间） ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true          // 关键：填满剩余高度
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
                            let mode = ["Once","Daily","Weekdays"][model.repeatMode];
                            let status = model.enabled ? qsTr("Enabled") : qsTr("Disabled");
                            return "%1 - %2  %3  [%4]".arg(start).arg(end).arg(mode).arg(status);
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

        // ── 底部操作栏：左按钮，右语言设置 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                text: qsTr("Add Lock Time")
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
                text: LockController.checking ? qsTr("Stop Checking") : qsTr("Start Checking")
                onClicked: {
                    LockController.toggleChecking();
                }
            }

            // 占位，把语言设置推到右侧
            Item { Layout.fillWidth: true }

            // 语言设置（紧凑排列）
            RowLayout {
                spacing: 8
                Label {
                    text: qsTr("Language")
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBox {
                    id: languageCombo
                    Layout.preferredWidth: 130
                    model: [qsTr("English"), qsTr("Chinese")]

                    Component.onCompleted: function() {
                        currentIndex = currentLang.startsWith("zh") ? 1 : 0;
                    }

                    onActivated: function(index) {
                        let newLang = index === 0 ? "en_US" : "zh_CN";
                        LanguageSwitcher.switchLanguage(newLang);
                    }
                }
            }
        }

        // ── 编辑对话框 ──
        RuleEditor {
            id: ruleEditorDialog
            ruleModel: timeRuleModel
        }
    }
}