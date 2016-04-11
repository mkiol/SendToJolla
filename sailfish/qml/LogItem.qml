/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0

Row {
    id: root

    property alias title: label1.text
    property alias description: label2.text

    anchors.left: parent.left; anchors.right: parent.right
    anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
    spacing: 1.0*Theme.paddingLarge

    Column {
        spacing: Theme.paddingSmall
        anchors.top: parent.top
        width: root.visible ? parent.width-1*Theme.paddingLarge : 0

        Label {
            id: label1
            width: parent.width
            wrapMode: Text.WordWrap
            color: Theme.primaryColor
            font.bold: true

        }

        Label {
            id: label2
            width: parent.width
            wrapMode: Text.WordWrap
            color: Theme.primaryColor
        }

        Item {
            height: Theme.paddingLarge
            width: parent.width
        }
    }
}
