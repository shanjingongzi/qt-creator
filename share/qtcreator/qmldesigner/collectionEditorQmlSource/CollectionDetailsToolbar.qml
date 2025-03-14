// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as PlatformWidgets
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import CollectionEditorBackend

Item {
    id: root

    property real iconHeight: 2 * StudioTheme.Values.bigFont
    required property var model
    required property var backend
    property int selectedRow: -1

    implicitHeight: container.height

    function addNewColumn() {
        addColumnDialog.popUp(root.model.columnCount())
    }

    function addNewRow() {
        root.model.insertRow(root.model.rowCount())
    }

    RowLayout {
        id: container
        width: parent.width

        spacing: StudioTheme.Values.sectionRowSpacing

        RowLayout {
            id: leftSideToolbar

            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: StudioTheme.Values.sectionRowSpacing

            IconButton {
                icon: StudioTheme.Constants.addcolumnleft_medium
                tooltip: qsTr("Add property left %1").arg(leftSideToolbar.topPadding)
                enabled: root.model.selectedColumn > -1
                onClicked: addColumnDialog.popUp(root.model.selectedColumn - 1)
            }

            IconButton {
                icon: StudioTheme.Constants.addcolumnright_medium
                tooltip: qsTr("Add property right")
                enabled: root.model.selectedColumn > -1
                onClicked: addColumnDialog.popUp(root.model.selectedColumn + 1)
            }

            IconButton {
                icon: StudioTheme.Constants.deletecolumn_medium
                tooltip: qsTr("Delete selected property")
                enabled: root.model.selectedColumn > -1
                onClicked: root.model.removeColumn(root.model.selectedColumn)
            }

            Item { // spacer
                implicitWidth: StudioTheme.Values.toolbarSpacing
                implicitHeight: 1
            }

            IconButton {
                icon: StudioTheme.Constants.addrowbelow_medium
                tooltip: qsTr("Insert row below")
                enabled: root.model.selectedRow > -1
                onClicked: root.model.insertRow(root.model.selectedRow + 1)
            }

            IconButton {
                icon: StudioTheme.Constants.addrowabove_medium
                tooltip: qsTr("Insert row above")
                enabled: root.model.selectedRow > -1
                onClicked: root.model.insertRow(root.model.selectedRow)
            }

            IconButton {
                icon: StudioTheme.Constants.deleterow_medium
                tooltip: qsTr("Delete selected row")
                enabled: root.model.selectedRow > -1
                onClicked: root.model.removeRow(root.model.selectedRow)
            }
        }

        Item { // spacer
            Layout.minimumHeight: 1
            Layout.fillWidth: true
        }

        RowLayout {
            id: rightSideToolbar
            spacing: StudioTheme.Values.sectionRowSpacing
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

            IconButton {
                icon: StudioTheme.Constants.updateContent_medium
                tooltip: qsTr("Update existing file with changes")
                enabled: root.model.collectionName !== ""
                onClicked:
                {
                    if (root.backend.selectedSourceAddress().indexOf("json") !== -1)
                        root.model.exportCollection(root.backend.selectedSourceAddress(), root.model.collectionName, "JSON")
                    else
                        root.model.exportCollection(root.backend.selectedSourceAddress(), root.model.collectionName, "CSV")
                }
            }

            IconButton {
                icon: StudioTheme.Constants.export_medium
                tooltip: qsTr("Export the model to a new file")
                enabled: root.model.collectionName !== ""
                onClicked: exportMenu.popup()
            }

            StudioControls.Menu {
                id: exportMenu

                StudioControls.MenuItem {
                    text: qsTr("Export as JSON")
                    onTriggered:
                    {
                        fileDialog.defaultSuffix = "json"
                        fileDialog.open()
                    }
                }

                StudioControls.MenuItem {
                    text: qsTr("Export as CSV")
                    onTriggered:
                    {
                        fileDialog.defaultSuffix = "csv"
                        fileDialog.open()
                    }
                }
            }
        }
    }


    PlatformWidgets.FileDialog {
        id: fileDialog
        fileMode: PlatformWidgets.FileDialog.SaveFile
        onAccepted:
        {
            var fileAddress = file.toString()

            if (fileAddress.indexOf("json") !== -1)
                root.model.exportCollection(fileAddress, root.model.collectionName, "JSON")
            else if (fileAddress.indexOf("csv") !== -1)
                root.model.exportCollection(fileAddress, root.model.collectionName, "CSV")

            fileDialog.reject()
        }
    }

    component IconButton: HelperWidgets.IconButton {
        Layout.preferredHeight: root.iconHeight
        Layout.preferredWidth: root.iconHeight
        radius: StudioTheme.Values.smallRadius
        iconSize: StudioTheme.Values.bigFont
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
    }

    RegularExpressionValidator {
        id: nameValidator
        regularExpression: /^\w+$/
    }

    StudioControls.Dialog {
        id: addColumnDialog

        property int clickedIndex: -1
        property bool nameIsValid

        title: qsTr("Add Column")

        function popUp(index)
        {
            addColumnDialog.clickedIndex = index
            columnName.text = ""
            addedPropertyType.currentIndex = addedPropertyType.find("String")

            addColumnDialog.open()
        }

        function addColumnName() {
            if (addColumnDialog.nameIsValid) {
                root.model.addColumn(addColumnDialog.clickedIndex, columnName.text, addedPropertyType.currentText)
                addColumnDialog.accept()
            } else {
                addColumnDialog.reject()
            }
        }

        contentItem: ColumnLayout {
            spacing: 2

            Text {
                text: qsTr("Column name:")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: columnName

                Layout.fillWidth: true

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: nameValidator

                Keys.onEnterPressed: addColumnDialog.addColumnName()
                Keys.onReturnPressed: addColumnDialog.addColumnName()
                Keys.onEscapePressed: addColumnDialog.reject()

                onTextChanged: {
                    addColumnDialog.nameIsValid = (columnName.text !== ""
                                                   && !root.model.isPropertyAvailable(columnName.text))
                }
            }

            Spacer { implicitHeight: StudioTheme.Values.controlLabelGap }

            Label {
                Layout.fillWidth: true

                text: qsTr("The model already contains \"%1\"!").arg(columnName.text)
                visible: columnName.text !== "" && !addColumnDialog.nameIsValid

                color: StudioTheme.Values.themeTextColor
                wrapMode: Label.WordWrap
                padding: 5

                background: Rectangle {
                    color: "transparent"
                    border.width: StudioTheme.Values.border
                    border.color: StudioTheme.Values.themeWarning
                }
            }

            Spacer {}

            Text {
                text: qsTr("Type:")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.ComboBox {
                id: addedPropertyType

                Layout.fillWidth: true

                model: root.model.typesList()
                actionIndicatorVisible: false
            }

            Spacer {}

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

                HelperWidgets.Button {
                    enabled: addColumnDialog.nameIsValid
                    text: qsTr("Add")
                    onClicked: addColumnDialog.addColumnName()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: addColumnDialog.reject()
                }
            }
        }
    }
}
