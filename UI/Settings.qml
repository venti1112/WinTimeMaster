import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: settingsWindow
    visible: !startMinimized
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
            text: qsTr("Rules")
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
                            let modeStr;
                            if (model.repeatMode === 2) {   // WeekDays
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
            spacing: 16

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
                onClicked: {
                    LockController.toggleChecking();
                }
            }
            // 状态指示灯（红/绿圆点）
            Rectangle {
                width: 14
                height: 14
                radius: 7
                color: LockController.checking ? "#4CAF50" : "#F44336"
                Layout.alignment: Qt.AlignVCenter
            }

            // 状态文字
            Label {
                text: LockController.checking ? qsTr("Service Running") : qsTr("Service Stopped")
                Layout.alignment: Qt.AlignVCenter
                font.pixelSize: 13
            }

            CheckBox {
                id: autostartCheck
                text: qsTr("Auto Start")
                checked: SettingsController.autostartEnabled
                onToggled: SettingsController.setAutostartEnabled(checked)
                Layout.alignment: Qt.AlignVCenter
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

                    // 语言模型：显示文本需在翻译文件中定义
                    ListModel {
                        id: languageModel
                        ListElement { text: qsTr("English"); code: "en_US" }
                        ListElement { text: qsTr("简体中文"); code: "zh_CN" }
                        ListElement { text: qsTr("繁體中文"); code: "zh_TW" }
                        ListElement { text: qsTr("한국어"); code: "ko" }
                        ListElement { text: qsTr("日本語"); code: "ja_JP" }
                    }
                    model: languageModel
                    textRole: "text"

                    // 根据当前语言代码设置初始索引
                    Component.onCompleted: function() {
                        for (let i = 0; i < languageModel.count; ++i) {
                            if (languageModel.get(i).code === currentLang) {
                                currentIndex = i;
                                break;
                            }
                        }
                    }

                    onActivated: function(index) {
                        let langCode = languageModel.get(index).code;
                        LanguageSwitcher.switchLanguage(langCode);
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