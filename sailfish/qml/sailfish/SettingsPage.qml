/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: root

    canAccept: parseInt(portField.text).toString()==portField.text
               && parseInt(portField.text) > 0 && parseInt(portField.text) <= 65535

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            spacing: Theme.paddingLarge
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            DialogHeader {
                title: qsTr("Settings")
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: qsTr("Define a listening port number. Changes will take effect after app restart.")
                wrapMode: Text.WordWrap
            }

            TextField {
                id: portField
                anchors {left: parent.left;right: parent.right}

                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("Enter port number here!")
                label: qsTr("Port number")

                Component.onCompleted: {
                    text = settings.port
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Qt.inputMethod.hide();
                }
            }

        }
    }

    onAccepted: {
        settings.port = portField.text
    }
}
