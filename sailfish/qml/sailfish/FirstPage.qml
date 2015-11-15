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
        if (settings.startProxy)
            server.connectProxy();
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
                text: qsTr("Mark below what connection mode you want to enable.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.secondaryColor
                text: qsTr("To be able to use Proxy mode, fill 'Proxy URL' in the settings.")
                wrapMode: Text.WordWrap
                visible: settings.proxyUrl == ""
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

            TextSwitch {
                text: qsTr("Proxy")
                automaticCheck: false
                checked: server.proxyOpen
                enabled: settings.proxyUrl != "";
                busy: server.proxyBusy;
                onClicked: {
                    if (!server.proxyBusy) {
                        if (server.proxyOpen)
                            server.disconnectProxy();
                        else
                            server.connectProxy();
                    }
                }
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.localServerRunning || server.proxyOpen
                color: Theme.secondaryColor
                text: qsTr("To open Web client, go to one of URL addresses below in your favorite web browser.")
                wrapMode: Text.WordWrap
            }

            /*Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.localServerRunning || server.proxyOpen
                color: Theme.secondaryColor
                text: qsTr("Tip #1: To open Web client, go to below URL address in your favorite web browser.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.localServerRunning || server.proxyOpen
                color: Theme.secondaryColor
                text: qsTr("Tip #2: To configure Firefox add-on, go to add-on's preferences and fill out the 'Server URL' with the URL displayed below.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.localServerRunning || server.proxyOpen
                color: Theme.secondaryColor
                text: qsTr("Tip #3: To send phone's clipboard data, after copying bring %1 to the foreground.").arg(APP_NAME)
                wrapMode: Text.WordWrap
            }*/

            ListItem {
                id: urlList
                enabled: server.localServerRunning || server.proxyOpen
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
                        text: server.getLocalServerUrl()
                        wrapMode: Text.WordWrap
                        visible: server.localServerRunning
                    }

                    Label {
                        id: proxyUrlLabel
                        anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                        color: Theme.highlightColor
                        text: server.getWebClientProxyUrl()
                        wrapMode: Text.WordWrap
                        visible: server.proxyOpen
                    }
                }

                Connections {
                    target: settings
                    onCookieChanged: {
                        localServerUrlLabel.text = server.getLocalServerUrl();
                        proxyUrlLabel.text = server.getWebClientProxyUrl();
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

            /*Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.proxyOpen
                color: Theme.highlightColor
                text: server.getWebClientProxyUrl()
                wrapMode: Text.WordWrap
            }*/

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
