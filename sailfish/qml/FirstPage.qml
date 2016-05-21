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

Page {
    id: root

    Component.onCompleted: {
        if (settings.startLocalServer)
            server.startLocalServer();
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Menu {}

        Column {
            id: column
            spacing: Theme.paddingLarge
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            PageHeader {
                title: APP_NAME
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: qsTr("Toggle below to enable or disable %1 local server.").arg(APP_NAME)
                wrapMode: Text.WordWrap
            }

            TextSwitch {
                text: qsTr("Local server")
                automaticCheck: false
                checked: server.localServerRunning
                onClicked: {
                    if (server.localServerRunning)
                        server.stopLocalServer();
                    else
                        server.startLocalServer();
                }
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.localServerRunning
                color: Theme.secondaryColor
                text: qsTr("To open Web client, go to the URL address below in your favorite web browser.")
                wrapMode: Text.WordWrap
            }

            ListItem {
                id: urlList
                enabled: server.localServerRunning
                visible: enabled
                onClicked: showMenu()
                contentHeight: col.height + 2 * Theme.paddingLarge

                Column {
                    id: col
                    anchors {left: parent.left; right: parent.right; topMargin: Theme.paddingLarge; bottomMargin:Theme.paddingLarge;}
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingMedium

                    Label {
                        id: localServerUrlLabel
                        anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                        color: Theme.highlightColor
                        text: server.localServerRunning ? server.getLocalServerUrl() : ""
                        wrapMode: Text.WordWrap
                        visible: server.localServerRunning
                    }
                }

                Connections {
                    target: settings
                    onCookieChanged: {
                        localServerUrlLabel.text = server.getLocalServerUrl();
                    }
                }

                menu: ContextMenu {
                    enabled: server.localServerRunning
                    MenuItem {
                        text: qsTr("Generate new URL")
                        onClicked: {
                            urlList.remorseAction("Generating new URL", function() {
                                settings.generateCookie();
                            });
                        }
                    }
                }
            }

            Connections {
                target: server
                onNewEvent: {
                    logLabel.text = "> " + text + "\n" + logLabel.text;
                }
            }

            SectionHeader {
                text: qsTr("Log")
            }

            Label {
                id: logLabel
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                wrapMode: Text.WordWrap
            }

        }
    }
}
