import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: ruleEditorDialog
    modal: true
    standardButtons: Dialog.NoButton

    // ── 外部属性 ──
    property var ruleModel: null
    property int ruleIndex: -1
    property int ruleId: -1
    property var startTime: null
    property var endTime: null
    property int repeatMode: 1
    property int weekDays: 0
    property bool ruleEnabled: true

    // ── 临时变量 ──
    property var tempStartTime: startTime ? startTime : new Date(0, 0, 0, 9, 0)
    property var tempEndTime: endTime ? endTime : new Date(0, 0, 0, 10, 0)
    property int tempRepeatMode: repeatMode
    property int tempWeekDays: weekDays
    property bool tempRuleEnabled: ruleEnabled

    title: ruleIndex === -1 ? qsTr("Add Rule") : qsTr("Edit Rule")
    width: tempRepeatMode === 2 ? 610 : 340
    height: 360
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    footer: DialogButtonBox {
        contentItem: RowLayout {
            spacing: 12
            Button {
                text: qsTr("Cancel")
                onClicked: ruleEditorDialog.reject()
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("OK")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: ruleEditorDialog.accept()
            }
        }
    }

    onOpened: {
        if (startTime) tempStartTime = startTime
        else tempStartTime = new Date(0, 0, 0, 9, 0)
        if (endTime) tempEndTime = endTime
        else tempEndTime = new Date(0, 0, 0, 10, 0)
        tempRepeatMode = repeatMode
        tempWeekDays = weekDays
        tempRuleEnabled = ruleEnabled
    }

    onAccepted: {
        // 从 SpinBox 构造时间
        tempStartTime = new Date(0, 0, 0, startHour.value, startMinute.value)
        tempEndTime = new Date(0, 0, 0, endHour.value, endMinute.value)

        if (!ruleModel) return

        if (ruleIndex === -1) {
            ruleModel.addRule()
            let newIndex = ruleModel.rowCount() - 1
            ruleModel.updateRule(newIndex, {
                startTime: tempStartTime,
                endTime: tempEndTime,
                repeatMode: tempRepeatMode,
                weekDays: tempWeekDays,
                enabled: tempRuleEnabled
            })
        } else {
            ruleModel.updateRule(ruleIndex, {
                startTime: tempStartTime,
                endTime: tempEndTime,
                repeatMode: tempRepeatMode,
                weekDays: tempWeekDays,
                enabled: tempRuleEnabled
            })
        }
    }

    // ── 左右分区布局 ──
    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        anchors.leftMargin: 2
        anchors.rightMargin: 6
        anchors.topMargin: 8
        anchors.bottomMargin: 16
        spacing: 20

        // 左侧：基本设置
        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillHeight: true
            spacing: 12

            // 启用开关
            RowLayout {
                Label { text: qsTr("Enabled"); Layout.preferredWidth: 65 }
                Switch {
                    checked: tempRuleEnabled
                    onToggled: tempRuleEnabled = checked
                }
            }

            // 开始时间（小时+分钟）
            RowLayout {
                Label { text: qsTr("Start Time"); Layout.preferredWidth: 65 }
                SpinBox {
                    id: startHour
                    from: 0; to: 23
                    value: tempStartTime.getHours()
                    editable: true
                    Layout.preferredWidth: 100
                }
                Label { text: ":" }
                SpinBox {
                    id: startMinute
                    from: 0; to: 59
                    value: tempStartTime.getMinutes()
                    editable: true
                    Layout.preferredWidth: 100
                }
            }

            // 结束时间（小时+分钟）
            RowLayout {
                Label { text: qsTr("End Time"); Layout.preferredWidth: 65 }
                SpinBox {
                    id: endHour
                    from: 0; to: 23
                    value: tempEndTime.getHours()
                    editable: true
                    Layout.preferredWidth: 100
                }
                Label { text: ":" }
                SpinBox {
                    id: endMinute
                    from: 0; to: 59
                    value: tempEndTime.getMinutes()
                    editable: true
                    Layout.preferredWidth: 100
                }
            }

            // 重复模式
            RowLayout {
                Label {
                    text: qsTr("Repeat")
                    Layout.preferredWidth: 65
                }
                ComboBox {
                    id: repeatCombo
                    model: [qsTr("Once"), qsTr("Daily"), qsTr("Weekdays")]
                    currentIndex: tempRepeatMode
                    Layout.preferredWidth: 100
                    onActivated: function(index) { tempRepeatMode = index }
                }
            }
        }

        // 右侧：星期选择（仅 Weekdays 显示）
        ColumnLayout {
            visible: tempRepeatMode === 2
            Layout.alignment: Qt.AlignTop
            spacing: 8

            Label {
                text: qsTr("Days of Week")
                font.bold: true
            }

            GridLayout {
                columns: 3
                rowSpacing: 4
                columnSpacing: 8

                Repeater {
                    model: [
                        { text: qsTr("Mon"), value: 1 },
                        { text: qsTr("Tue"), value: 2 },
                        { text: qsTr("Wed"), value: 4 },
                        { text: qsTr("Thu"), value: 8 },
                        { text: qsTr("Fri"), value: 16 },
                        { text: qsTr("Sat"), value: 32 },
                        { text: qsTr("Sun"), value: 64 }
                    ]
                    delegate: CheckBox {
                        text: modelData.text
                        checked: (tempWeekDays & modelData.value) !== 0
                        onToggled: {
                            if (checked) tempWeekDays |= modelData.value
                            else tempWeekDays &= ~modelData.value
                        }
                    }
                }
            }
        }
    }
}