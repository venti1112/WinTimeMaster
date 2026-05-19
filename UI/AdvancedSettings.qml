import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: advancedSettingsDialog
    modal: true
    standardButtons: Dialog.Ok

    title: qsTr("Advanced Settings")
    width: 640
    height: 420
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    // 背景模式索引：0=纯色, 1=透明, 2=自定义图片, 3=自定义视频
    property int bgModeIndex: {
        let bg = SettingsController.lockScreenBackground;
        if (bg === "transparent") return 1;
        if (bg === "") return 0;
        let lowerBg = bg.toLowerCase();
        if (lowerBg.endsWith(".png") || lowerBg.endsWith(".jpg") || lowerBg.endsWith(".jpeg") || lowerBg.endsWith(".bmp"))
            return 2;
        if (lowerBg.endsWith(".mp4") || lowerBg.endsWith(".avi") || lowerBg.endsWith(".mov") || lowerBg.endsWith(".wmv") || lowerBg.endsWith(".mkv"))
            return 3;
        return 0;
    }

    property string currentColor: {
        let bg = SettingsController.lockScreenBackground;
        if (bg.startsWith("#") && bg.length >= 7) return bg;
        return "#ff000000";
    }

    // ── 左右分区布局 ──
    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 24

        // 左侧：安全选项复选框
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillHeight: true
            spacing: 12
            CheckBox {
                text: qsTr("Auto Start")
                checked: SettingsController.autostartEnabled
                onToggled: SettingsController.setAutostartEnabled(checked)
            }
            CheckBox {
                text: qsTr("Disable Task Manager")
                checked: SettingsController.disableTaskManager
                onToggled: SettingsController.setDisableTaskManager(checked)
            }
            CheckBox {
                text: qsTr("Enable Input Block")
                checked: SettingsController.enableInputBlock
                onToggled: SettingsController.setEnableInputBlock(checked)
            }
            CheckBox {
                text: qsTr("Force Close Taskmgr.exe")
                checked: SettingsController.killTaskmgr
                onToggled: SettingsController.setKillTaskmgr(checked)
            }
            CheckBox {
                text: qsTr("Auto Time Sync")
                checked: SettingsController.autoTimeSync
                onToggled: SettingsController.setAutoTimeSync(checked)
            }
        }

        // 右侧：锁屏背景 + 语言设置
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillHeight: true
            spacing: 16

            // ── 锁屏背景设置 ──
            ColumnLayout {
                spacing: 8

                Label {
                    text: qsTr("Lock Screen Background")
                    font.bold: true
                }

                ComboBox {
                    id: bgModeCombo
                    Layout.fillWidth: true
                    model: [qsTr("Solid Color"), qsTr("Transparent"), qsTr("Custom Image"), qsTr("Custom Video")]
                    currentIndex: bgModeIndex
                    onActivated: function(index) {
                        if (index === 0) {
                            SettingsController.setLockScreenBackground(currentColor);
                        } else if (index === 1) {
                            SettingsController.setLockScreenBackground("transparent");
                        } else {
                            let oldBg = SettingsController.lockScreenBackground;
                            let keep = false;
                            if (index === 2) {
                                let lower = oldBg.toLowerCase();
                                keep = lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp");
                            } else if (index === 3) {
                                let lower = oldBg.toLowerCase();
                                keep = lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mov") || lower.endsWith(".wmv") || lower.endsWith(".mkv");
                            }
                            if (!keep)
                                SettingsController.setLockScreenBackground(currentColor);
                        }
                    }
                }

                // 纯色选择器
                RowLayout {
                    visible: bgModeCombo.currentIndex === 0
                    spacing: 8
                    Rectangle {
                        width: 30; height: 30; radius: 4
                        color: currentColor
                        border.color: palette.mid
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        text: currentColor
                    }
                    Button {
                        text: qsTr("Pick Color")
                        onClicked: {
                            colorDialog.selectedColor = currentColor;
                            colorDialog.open();
                        }
                    }
                }

                // 自定义图片选择器
                RowLayout {
                    visible: bgModeCombo.currentIndex === 2
                    spacing: 8
                    TextField {
                        id: imagePathField
                        Layout.fillWidth: true
                        readOnly: true
                        text: {
                            let bg = SettingsController.lockScreenBackground;
                            let lower = bg.toLowerCase();
                            let isImage = lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp");
                            return isImage ? bg : qsTr("No image selected");
                        }
                    }
                    Button {
                        text: qsTr("Choose Image")
                        onClicked: imageFileDialog.open()
                    }
                    Button {
                        text: qsTr("Clear")
                        onClicked: SettingsController.setLockScreenBackground(currentColor)
                    }
                }

                // 自定义视频选择器
                RowLayout {
                    visible: bgModeCombo.currentIndex === 3
                    spacing: 8
                    TextField {
                        id: videoPathField
                        Layout.fillWidth: true
                        readOnly: true
                        text: {
                            let bg = SettingsController.lockScreenBackground;
                            let lower = bg.toLowerCase();
                            let isVideo = lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mov") || lower.endsWith(".wmv") || lower.endsWith(".mkv");
                            return isVideo ? bg : qsTr("No video selected");
                        }
                    }
                    Button {
                        text: qsTr("Choose Video")
                        onClicked: videoFileDialog.open()
                    }
                    Button {
                        text: qsTr("Clear")
                        onClicked: SettingsController.setLockScreenBackground(currentColor)
                    }
                }
            }

            // ── 语言设置 ──
            ColumnLayout {
                spacing: 8

                Label {
                    text: qsTr("Language")
                    font.bold: true
                }

                ComboBox {
                    id: languageCombo
                    Layout.fillWidth: true
                    model: ListModel {
                        ListElement { text: qsTr("English"); code: "en_US" }
                        ListElement { text: qsTr("简体中文"); code: "zh_CN" }
                        ListElement { text: qsTr("繁體中文"); code: "zh_TW" }
                        ListElement { text: qsTr("한국어"); code: "ko" }
                        ListElement { text: qsTr("日本語"); code: "ja_JP" }
                    }
                    textRole: "text"
                    Component.onCompleted: {
                        let curLang = LanguageSwitcher.currentLanguage;
                        for (let i = 0; i < model.count; ++i) {
                            if (model.get(i).code === curLang) {
                                currentIndex = i;
                                break;
                            }
                        }
                    }
                    onActivated: function(index) {
                        LanguageSwitcher.switchLanguage(model.get(index).code);
                    }
                }
            }
            // 导出导入按钮
            RowLayout {
                spacing: 8
                Button {
                    text: qsTr("Export Settings")
                    onClicked: exportFileDialog.open()
                }
                Button {
                    text: qsTr("Import Settings")
                    onClicked: importFileDialog.open()
                }
            }
        }
    }

    // 对话框级的文件/颜色选择器
    ColorDialog {
        id: colorDialog
        title: qsTr("Select Background Color")
        onAccepted: {
            let fullColor = selectedColor.toString();
            currentColor = fullColor;
            if (bgModeCombo.currentIndex === 0)
                SettingsController.setLockScreenBackground(fullColor);
        }
    }

    FileDialog {
        id: imageFileDialog
        title: qsTr("Select Background Image")
        nameFilters: [ "Image files (*.png *.jpg *.jpeg *.bmp)" ]
        onAccepted: SettingsController.setLockScreenBackground(selectedFile.toString())
    }

    FileDialog {
        id: videoFileDialog
        title: qsTr("Select Background Video")
        nameFilters: [ "Video files (*.mp4 *.avi *.mov *.wmv *.mkv)" ]
        onAccepted: SettingsController.setLockScreenBackground(selectedFile.toString())
    }
    FileDialog {
        id: exportFileDialog
        title: qsTr("Export Settings")
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let path = selectedFile.toString().replace("file:///", "");
            // Windows 路径可能以 / 开头，需处理
            if (Qt.platform.os === "windows") {
                if (path.startsWith("/"))
                    path = path.substring(1);
            }
            let err = SettingsController.exportSettings(path);
            if (err) {
                importErrorDialog.text = err;
                importErrorDialog.title = qsTr("Export Error");
                importErrorDialog.open();
            } else {
                importErrorDialog.text = qsTr("Settings exported successfully.");
                importErrorDialog.title = qsTr("Success");
                importErrorDialog.open();
            }
        }
    }

    FileDialog {
        id: importFileDialog
        title: qsTr("Import Settings")
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            let path = selectedFile.toString().replace("file:///", "");
            if (Qt.platform.os === "windows") {
                if (path.startsWith("/"))
                    path = path.substring(1);
            }
            let err = SettingsController.importSettings(path);
            if (err) {
                importErrorDialog.text = err;
                importErrorDialog.title = qsTr("Import Error");
                importErrorDialog.open();
            } else {
                LanguageSwitcher.reloadLanguage();
                SettingsController.timeRuleModel.reload();
                importErrorDialog.text = qsTr("Settings imported successfully.");
                importErrorDialog.title = qsTr("Success");
                importErrorDialog.open();
            }
        }
    }

    MessageDialog {
        id: importErrorDialog
        buttons: MessageDialog.Ok
    }
}