/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

import QtQuick 2.2
import Sailfish.Silica 1.0

CoverBackground {

    CoverActionList {
        enabled: !server.proxyBusy && ((settings.startProxy && settings.proxyUrl != "") || settings.startLocalServer)
        CoverAction {
            iconSource: (server.localServerRunning || server.proxyOpen) ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: {
                if (server.localServerRunning || server.proxyOpen) {
                    if (server.proxyOpen)
                        server.disconnectProxy();
                    if (server.localServerRunning)
                        server.stopLocalServer();
                } else {
                    if (settings.startProxy && settings.proxyUrl != "")
                        server.connectProxy();
                    if (settings.startLocalServer)
                        server.startLocalServer();
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingMedium

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "icon-small.png"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: APP_NAME
        }
    }
}


