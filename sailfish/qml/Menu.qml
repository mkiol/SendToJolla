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

PullDownMenu {
    id: root

    MenuItem {
        text: qsTr("About")

        onClicked: {
            pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
        }
    }

    MenuItem {
        text: qsTr("Settings")

        onClicked: {
            pageStack.push(Qt.resolvedUrl("SettingsPage.qml"));
        }
    }
}
