// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Save Effect")

    closePolicy: Popup.CloseOnEscape
    modal: true
    implicitWidth: 250
    implicitHeight: 160

    property string compositionName: ""

    onOpened: {
        nameText.text = compositionName //TODO: Generate unique name
        emptyText.text = ""
        nameText.forceActiveFocus()
    }

    contentItem: Item {
        Column {
            spacing: 2

            Row {
                id: row
                Text {
                    text: qsTr("Effect name: ")
                    anchors.verticalCenter: parent.verticalCenter
                    color: StudioTheme.Values.themeTextColor
                }

                StudioControls.TextField {
                    id: nameText

                    actionIndicator.visible: false
                    translationIndicator.visible: false

                    onTextChanged: {
                        let errMsg = ""
                        if (/[^A-Za-z0-9_]+/.test(text))
                            errMsg = qsTr("Name contains invalid characters.")
                        else if (!/^[A-Z]/.test(text))
                            errMsg = qsTr("Name must start with a capital letter")
                        else if (text.length < 3)
                            errMsg = qsTr("Name must have at least 3 characters")
                        else if (/\s/.test(text))
                            errMsg = qsTr("Name cannot contain white space")

                        emptyText.text = errMsg
                        btnSave.enabled = errMsg.length === 0
                    }
                    Keys.onEnterPressed: btnSave.onClicked()
                    Keys.onReturnPressed: btnSave.onClicked()
                    Keys.onEscapePressed: root.reject()
                }
            }

            Text {
                id: emptyText

                color: StudioTheme.Values.themeError
                anchors.right: row.right
            }
        }

        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 2

            HelperWidgets.Button {
                id: btnSave

                text: qsTr("Save")
                enabled: nameText.text !== ""
                onClicked: {
                    root.compositionName = nameText.text
                    root.accept() //TODO: Check if name is unique
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
