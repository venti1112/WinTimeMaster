import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: advancedSettingsDialog
    modal: true
    standardButtons: Dialog.NoButton

    title: qsTr("Advanced Settings")
    width: 740
    height: 500
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    footer: DialogButtonBox {
        contentItem: RowLayout {
            spacing: 12
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("OK")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: advancedSettingsDialog.accept()
            }
        }
    }

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

    // 每行文字颜色（UI 反应式镜像，避免每次重绘都重新格式化）
    function _validHex(c, fallback) {
        return (c && c.startsWith("#") && c.length >= 7) ? c : fallback;
    }
    property string promptColor:        _validHex(SettingsController.lockPromptColor,        "#ffffffff")
    property string currentTimeColor:   _validHex(SettingsController.lockCurrentTimeColor,   "#ffd3d3d3")
    property string unlockTimeColor:    _validHex(SettingsController.lockUnlockTimeColor,    "#ffd3d3d3")
    property string remainingTimeColor: _validHex(SettingsController.lockRemainingTimeColor, "#ffff6347")

    // 文字位置：内部以英文键存储，UI 上展示本地化标签
    readonly property var textPositionKeys: [
        "top-left", "top-center", "top-right",
        "center-left", "center", "center-right",
        "bottom-left", "bottom-center", "bottom-right"
    ]
    readonly property var textPositionLabels: [
        qsTr("Top Left"), qsTr("Top Center"), qsTr("Top Right"),
        qsTr("Middle Left"), qsTr("Center"), qsTr("Middle Right"),
        qsTr("Bottom Left"), qsTr("Bottom Center"), qsTr("Bottom Right")
    ]

    // 把 FileDialog 给的 file:/// URL 转成本机原生路径
    function urlToNativePath(url) {
        let s = url.toString();
        if (s.startsWith("file:///")) {
            if (Qt.platform.os === "windows") {
                s = s.substring(8);  // 去掉 file:///
            } else {
                s = s.substring(7);  // 去掉 file:// — 保留 leading /
            }
        } else if (s.startsWith("file://")) {
            s = s.substring(7);
        }
        return decodeURIComponent(s);
    }

    onAccepted: {
        let bg = SettingsController.lockScreenBackground;
        let lower = bg.toLowerCase();
        if (bgModeCombo.currentIndex === 2) {
            let valid = lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp");
            if (!valid) {
                SettingsController.setLockScreenBackground(currentColor);
                bgModeCombo.currentIndex = 0;
            }
        } else if (bgModeCombo.currentIndex === 3) {
            let valid = lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mov") || lower.endsWith(".wmv") || lower.endsWith(".mkv");
            if (!valid) {
                SettingsController.setLockScreenBackground(currentColor);
                bgModeCombo.currentIndex = 0;
            }
        }
    }

    // ── 左右分区布局 ──
    // 用 Flickable 做可滚动容器，滚动条作为独立子项，不受 Style 覆盖
    Flickable {
        id: contentFlick
        anchors.fill: parent
        anchors.margins: 16
        anchors.leftMargin: 2
        anchors.rightMargin: 6
        anchors.topMargin: 8
        anchors.bottomMargin: 16
        clip: true

        contentWidth: rowLayout.width
        contentHeight: rowLayout.height

        // 与规则列表保持一致的滚动条外观
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                color: "#CCFFFFFF"
                radius: 4
            }
            width: 6
        }

        RowLayout {
            id: rowLayout
            width: contentFlick.width - (contentFlick.ScrollBar.vertical.visible ? contentFlick.ScrollBar.vertical.width : 0)
            spacing: 24

            // 左侧：安全选项复选框
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 320
                Layout.minimumWidth: 320
                Layout.maximumWidth: 320
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

            // ── 紧急退出 ──
            CheckBox {
                id: emergencyExitCheck
                text: qsTr("Enable Emergency Exit")
                checked: SettingsController.emergencyExitEnabled
                onToggled: SettingsController.setEmergencyExitEnabled(checked)
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                spacing: 6
                enabled: emergencyExitCheck.checked
                opacity: enabled ? 1.0 : 0.4
                Label {
                    text: qsTr("Click count:")
                    Layout.alignment: Qt.AlignVCenter
                }
                SpinBox {
                    id: exitClickSpin
                    from: 1
                    to: 99
                    value: SettingsController.emergencyExitClickCount
                    editable: true
                    onValueModified: SettingsController.setEmergencyExitClickCount(value)
                }
            }

            // ── 自动校时设置 ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 0
                spacing: 6

                CheckBox {
                    id: autoSyncCheck
                    text: qsTr("Auto Time Sync")
                    checked: TimeSyncManager.enabled
                    onToggled: TimeSyncManager.setEnabled(checked)
                }

                Label {
                    text: qsTr("Time Server")
                    Layout.leftMargin: 8
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    spacing: 6
                    TextField {
                        id: serverField
                        Layout.fillWidth: true
                        placeholderText: qsTr("e.g. time.windows.com")
                        text: TimeSyncManager.server
                        onEditingFinished: TimeSyncManager.setServer(text)
                    }
                    Button {
                        text: TimeSyncManager.syncing ? qsTr("Syncing...") : qsTr("Sync Now")
                        enabled: autoSyncCheck.checked && !TimeSyncManager.syncing
                        onClicked: {
                            TimeSyncManager.setServer(serverField.text);
                            TimeSyncManager.syncNow();
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    spacing: 6
                    Label {
                        text: qsTr("Sync Interval (minutes)")
                        Layout.alignment: Qt.AlignVCenter
                    }
                    SpinBox {
                        id: intervalSpin
                        from: 1
                        to: 10080
                        value: TimeSyncManager.intervalMinutes
                        editable: true
                        onValueModified: TimeSyncManager.setIntervalMinutes(value)
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    wrapMode: Text.WordWrap
                    text: TimeSyncManager.lastSyncTime.length > 0
                        ? qsTr("Last sync: %1 - %2")
                            .arg(TimeSyncManager.lastSyncTime)
                            .arg(TimeSyncManager.lastSyncStatus)
                        : qsTr("Last sync: never")
                    color: TimeSyncManager.lastSyncSuccess ? "#2e7d32" : palette.windowText
                    font.pixelSize: 12
                }
            }
            // ── 密码设置 ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 8

                Label {
                    text: qsTr("Password Settings")
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: passwordField
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        placeholderText: qsTr("Enter new password (empty to disable)")
                    }

                    Button {
                        text: qsTr("OK")
                        onClicked: {
                            SettingsController.setPassword(passwordField.text);
                        }
                    }
                }

                Label {
                    text: SettingsController.password.length > 0 ? qsTr("Password is set") : qsTr("No password set")
                    color: SettingsController.password.length > 0 ? "#2e7d32" : palette.windowText
                    font.pixelSize: 12
                }
            }

            // ── 远程配置更新 ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 6

                CheckBox {
                    id: remoteConfigCheck
                    text: qsTr("Auto Remote Config Update")
                    checked: RemoteConfigUpdater.enabled
                    onToggled: RemoteConfigUpdater.setEnabled(checked)
                }

                Label {
                    text: qsTr("Remote Config URL")
                    Layout.leftMargin: 8
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    spacing: 6
                    TextField {
                        id: remoteUrlField
                        Layout.fillWidth: true
                        placeholderText: qsTr("e.g. https://example.com/config.json")
                        text: RemoteConfigUpdater.remoteUrl
                        onEditingFinished: RemoteConfigUpdater.setRemoteUrl(text)
                    }
                    Button {
                        text: qsTr("Update Now")
                        enabled: remoteConfigCheck.checked && remoteUrlField.text.length > 0
                        onClicked: {
                            RemoteConfigUpdater.setRemoteUrl(remoteUrlField.text);
                            RemoteConfigUpdater.syncNow();
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    spacing: 6
                    Label {
                        text: qsTr("Update Interval (minutes)")
                        Layout.alignment: Qt.AlignVCenter
                    }
                    SpinBox {
                        id: remoteIntervalSpin
                        from: 1
                        to: 10080
                        value: RemoteConfigUpdater.intervalMinutes
                        editable: true
                        onValueModified: RemoteConfigUpdater.setIntervalMinutes(value)
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    wrapMode: Text.WordWrap
                    text: RemoteConfigUpdater.lastSyncTime.length > 0
                        ? qsTr("Last update: %1 - %2")
                            .arg(RemoteConfigUpdater.lastSyncTime)
                            .arg(RemoteConfigUpdater.lastSyncStatus)
                        : qsTr("Last update: never")
                    color: RemoteConfigUpdater.lastSyncSuccess ? "#2e7d32" : palette.windowText
                    font.pixelSize: 12
                }
            }

            // ── 语言设置 ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
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

            // ── 导出 / 导入 ──
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
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

        // 右侧：锁屏背景 + 锁屏文字
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
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
                        id: colorHexField
                        Layout.fillWidth: true
                        onVisibleChanged: if (visible) text = currentColor
                        onEditingFinished: {
                            let v = text.trim();
                            if (v.startsWith("#") && v.length >= 7) {
                                SettingsController.setLockScreenBackground(v);
                            } else {
                                // 不符合 hex 格式，立即还原
                                text = currentColor;
                            }
                        }
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
                        onVisibleChanged: if (visible) text = SettingsController.lockScreenBackground
                        onEditingFinished: {
                            let v = text.trim();
                            if (v.length === 0) {
                                // 空输入 → 还原
                                text = SettingsController.lockScreenBackground;
                                return;
                            }
                            let lower = v.toLowerCase();
                            if (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp")) {
                                bgModeCombo.currentIndex = 2;
                                SettingsController.setLockScreenBackground(v);
                            } else {
                                // 不符合图片后缀，还原
                                text = SettingsController.lockScreenBackground;
                            }
                        }
                    }
                    Button {
                        text: qsTr("Choose Image")
                        onClicked: imageFileDialog.open()
                    }
                }

                // 自定义视频选择器
                RowLayout {
                    visible: bgModeCombo.currentIndex === 3
                    spacing: 8
                    TextField {
                        id: videoPathField
                        Layout.fillWidth: true
                        onVisibleChanged: if (visible) text = SettingsController.lockScreenBackground
                        onEditingFinished: {
                            let v = text.trim();
                            if (v.length === 0) {
                                text = SettingsController.lockScreenBackground;
                                return;
                            }
                            let lower = v.toLowerCase();
                            if (lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mov") || lower.endsWith(".wmv") || lower.endsWith(".mkv")) {
                                bgModeCombo.currentIndex = 3;
                                SettingsController.setLockScreenBackground(v);
                            } else {
                                text = SettingsController.lockScreenBackground;
                            }
                        }
                    }
                    Button {
                        text: qsTr("Choose Video")
                        onClicked: videoFileDialog.open()
                    }
                }

                CheckBox {
                    visible: bgModeCombo.currentIndex === 3
                    text: qsTr("Play Video Sound")
                    checked: SettingsController.lockBackgroundVideoSound
                    onToggled: SettingsController.setLockBackgroundVideoSound(checked)
                }
            }

            // ── 锁屏文字设置 ──
            ColumnLayout {
                spacing: 8

                Label {
                    text: qsTr("Lock Screen Text")
                    font.bold: true
                }

                // 自定义提示词
                Label { text: qsTr("Custom Prompt") }
                TextField {
                    id: promptField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Default: Device Locked")
                    text: SettingsController.lockScreenPromptText
                    onEditingFinished: SettingsController.setLockScreenPromptText(text)
                }

                // 各行文字颜色（提示词 / 当前时间 / 解锁时间 / 剩余时间）
                Label {
                    text: qsTr("Text Color")
                    Layout.topMargin: 4
                }
                Repeater {
                    model: [
                        { key: "prompt",    label: qsTr("Prompt") },
                        { key: "current",   label: qsTr("Current Time") },
                        { key: "unlock",    label: qsTr("Unlock Time") },
                        { key: "remaining", label: qsTr("Remaining Time") }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        // 反应式颜色：跟随父级镜像属性的变化即时刷新预览与文本框
                        readonly property string itemColor: {
                            if (modelData.key === "prompt")    return promptColor;
                            if (modelData.key === "current")   return currentTimeColor;
                            if (modelData.key === "unlock")    return unlockTimeColor;
                            if (modelData.key === "remaining") return remainingTimeColor;
                            return "#ffffffff";
                        }
                        Label {
                            text: modelData.label
                            Layout.preferredWidth: 96
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: itemColor
                            border.color: palette.mid
                        }
                        TextField {
                            id: colorField
                            Layout.fillWidth: true
                            text: itemColor
                            onEditingFinished: {
                                let v = text.trim();
                                if (v.startsWith("#") && v.length >= 7) {
                                    if (modelData.key === "prompt")
                                        SettingsController.setLockPromptColor(v);
                                    else if (modelData.key === "current")
                                        SettingsController.setLockCurrentTimeColor(v);
                                    else if (modelData.key === "unlock")
                                        SettingsController.setLockUnlockTimeColor(v);
                                    else if (modelData.key === "remaining")
                                        SettingsController.setLockRemainingTimeColor(v);
                                }
                                // 用户输入会破坏 text 与 itemColor 的绑定 —— 重新建立
                                text = Qt.binding(function() { return itemColor });
                            }
                        }
                        Button {
                            text: qsTr("Pick Color")
                            onClicked: {
                                textColorDialog.targetKey = modelData.key;
                                textColorDialog.selectedColor = itemColor;
                                textColorDialog.open();
                            }
                        }
                    }
                }

                // 文字位置
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Text Position")
                        Layout.alignment: Qt.AlignVCenter
                    }
                    ComboBox {
                        id: textPositionCombo
                        Layout.fillWidth: true
                        model: textPositionLabels
                        currentIndex: {
                            let idx = textPositionKeys.indexOf(SettingsController.lockScreenTextPosition);
                            return idx >= 0 ? idx : 4; // default to "center"
                        }
                        onActivated: function(index) {
                            SettingsController.setLockScreenTextPosition(textPositionKeys[index]);
                        }
                    }
                }

                // 隐藏各时间项
                CheckBox {
                    text: qsTr("Hide Current Time")
                    checked: SettingsController.hideCurrentTime
                    onToggled: SettingsController.setHideCurrentTime(checked)
                }
                CheckBox {
                    text: qsTr("Hide Unlock Time")
                    checked: SettingsController.hideUnlockTime
                    onToggled: SettingsController.setHideUnlockTime(checked)
                }
                CheckBox {
                    text: qsTr("Hide Remaining Time")
                    checked: SettingsController.hideRemainingTime
                    onToggled: SettingsController.setHideRemainingTime(checked)
                }
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

    ColorDialog {
        id: textColorDialog
        title: qsTr("Select Text Color")
        // 由调用方设置：prompt / current / unlock / remaining
        property string targetKey: ""
        onAccepted: {
            let fullColor = selectedColor.toString();
            if (targetKey === "prompt")
                SettingsController.setLockPromptColor(fullColor);
            else if (targetKey === "current")
                SettingsController.setLockCurrentTimeColor(fullColor);
            else if (targetKey === "unlock")
                SettingsController.setLockUnlockTimeColor(fullColor);
            else if (targetKey === "remaining")
                SettingsController.setLockRemainingTimeColor(fullColor);
        }
    }

    FileDialog {
        id: imageFileDialog
        title: qsTr("Select Background Image")
        nameFilters: [ "Image files (*.png *.jpg *.jpeg *.bmp)" ]
        onAccepted: SettingsController.setLockScreenBackground(urlToNativePath(selectedFile))
    }

    FileDialog {
        id: videoFileDialog
        title: qsTr("Select Background Video")
        nameFilters: [ "Video files (*.mp4 *.avi *.mov *.wmv *.mkv)" ]
        onAccepted: SettingsController.setLockScreenBackground(urlToNativePath(selectedFile))
    }
    FileDialog {
        id: exportFileDialog
        title: qsTr("Export Settings")
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let path = urlToNativePath(selectedFile);
            let err = SettingsController.exportSettings(path);
            if (err) {
                messageDialog.dialogTitle = qsTr("Export Error");
                messageDialog.dialogText = err;
                messageDialog.open();
            } else {
                messageDialog.dialogTitle = qsTr("Success");
                messageDialog.dialogText = qsTr("Settings exported successfully.");
                messageDialog.open();
            }
        }
    }

    FileDialog {
        id: importFileDialog
        title: qsTr("Import Settings")
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            let path = urlToNativePath(selectedFile);
            let err = SettingsController.importSettings(path);
            if (err) {
                messageDialog.dialogTitle = qsTr("Import Error");
                messageDialog.dialogText = err;
                messageDialog.open();
            } else {
                LanguageSwitcher.reloadLanguage();
                SettingsController.timeRuleModel.reload();
                messageDialog.dialogTitle = qsTr("Success");
                messageDialog.dialogText = qsTr("Settings imported successfully.");
                messageDialog.open();
            }
        }
    }

    Dialog {
        id: messageDialog
        modal: true
        standardButtons: Dialog.NoButton
        title: dialogTitle
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 360
        height: 200

        property string dialogTitle: ""
        property string dialogText: ""

        contentItem: Label {
            text: messageDialog.dialogText
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
                    onClicked: messageDialog.close()
                }
            }
        }
    }


}