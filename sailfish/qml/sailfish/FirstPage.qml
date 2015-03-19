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

Page {
    id: root

    Component.onCompleted: {
        server.startServer();
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
                text: server.running ? qsTr("Server is running. Use below URL to configure client application.") :
                                       qsTr("Server is not running.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.running
                color: Theme.secondaryColor
                text: qsTr("Tip #1: To configure Firefox add-on, go to add-on's preferences and fill out the 'Server URL' with the URL displayed below.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                visible: server.running
                color: Theme.secondaryColor
                text: qsTr("Tip #2: To send phone's clipboard data, after copying bring %1 to the foreground.").arg(APP_NAME)
                wrapMode: Text.WordWrap
            }

            ListItem {
                enabled: true
                onClicked: showMenu()

                Label {
                    id: urlLabel
                    anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                    color: Theme.highlightColor
                    text: server.getServerUrl()
                    anchors.verticalCenter: parent.verticalCenter
                    visible: server.running
                }

                Connections {
                    target: settings
                    onCookieChanged: {
                        urlLabel.text = server.getServerUrl();
                    }
                }

                menu: ContextMenu {
                    enabled: server.running
                    MenuItem {
                        text: qsTr("Generate new URL")
                        onClicked: {
                            settings.generateCookie();
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

            /*ComboBox {
                label: qsTr("Browser")
                currentIndex: {
                    switch (settings.browser) {
                    case "sailfish-browser":
                        return 0;
                    case "firefox":
                        return 1;
                    }
                    return 0;
                }

                menu: ContextMenu {
                    MenuItem { text: qsTr("Sailfish browser") }
                    MenuItem { text: qsTr("Firefox") }
                }

                onCurrentIndexChanged: {
                    switch (currentIndex) {
                    case 0:
                        settings.browser = "sailfish-browser";
                        return;
                    case 1:
                        settings.browser = "firefox";
                        return;
                    }
                }
            }*/
        }
    }
}
