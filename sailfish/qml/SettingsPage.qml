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

Dialog {
    id: root

    // Source: http://dzone.com/snippets/validate-url-regexp
    function isUrlValid() {
      var re = /(http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
      proxyUrlErrorLabel.visible = proxyUrlField.text !== "" && !re.test(proxyUrlField.text);
      return !proxyUrlErrorLabel.visible;
    }

    function isPortValid() {
      var result = parseInt(portField.text).toString()==portField.text && parseInt(portField.text) > 1023 && parseInt(portField.text) <= 65535;
      portErrorLabel.visible = !result;
      return !portErrorLabel.visible;
    }

    canAccept: settings.proxy ? isPortValid() && isUrlValid() : isPortValid()

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

            SectionHeader {
                text: qsTr("Local server")
            }

            TextSwitch {
                text: qsTr("Auto start on startup")
                checked: settings.startLocalServer
                enabled: isPortValid()
                onCheckedChanged: {
                    settings.startLocalServer = checked;
                }
            }

            TextField {
                id: portField
                anchors {left: parent.left;right: parent.right}

                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: qsTr("Enter port number here!")
                label: qsTr("Listening port number")

                Component.onCompleted: {
                    text = settings.port
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Qt.inputMethod.hide();
                }
            }

            Label {
                id: portErrorLabel
                visible: false
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: "Red"
                text: qsTr("Port number is invalid! Allowed port range is 1024-65535.")
                wrapMode: Text.WordWrap
            }

            SectionHeader {
                text: qsTr("Proxy")
                visible: settings.proxy
            }

            TextSwitch {
                text: qsTr("Auto connect on startup")
                checked: settings.startProxy
                enabled: isUrlValid() && proxyUrlField.text !== ""
                visible: settings.proxy
                onCheckedChanged: {
                    settings.startProxy = checked;
                }
            }

            TextSwitch {
                text: qsTr("Ignore SSL errors")
                checked: settings.ignoreSSL
                visible: settings.proxy
                onCheckedChanged: {
                    settings.ignoreSSL = checked;
                }
            }

            TextField {
                id: proxyUrlField
                anchors {left: parent.left;right: parent.right}
                visible: settings.proxy
                inputMethodHints: Qt.ImhUrlCharactersOnly
                placeholderText: qsTr("Enter proxy URL here!")
                label: qsTr("Proxy URL")

                Component.onCompleted: {
                    text = settings.proxyUrl
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Qt.inputMethod.hide();
                }
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.secondaryColor
                text: qsTr("URL should point to <i>proxy.php</i> deployed on WWW server. E.g. https://example.com/proxy.php")
                wrapMode: Text.WordWrap
                visible: settings.proxy
            }

            Label {
                id: proxyUrlErrorLabel
                visible: false
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: "Red"
                text: qsTr("URL is invalid!")
                wrapMode: Text.WordWrap
            }

            SectionHeader {
                text: qsTr("Encryption")
            }

            TextSwitch {
                id: cryptSwitch
                text: qsTr("Encryption required")
                checked: settings.crypt
                onCheckedChanged: {
                    settings.crypt = checked;
                }
            }

            TextField {
                id: cryptKeyField
                visible: settings.crypt
                anchors {left: parent.left;right: parent.right}

                inputMethodHints: Qt.ImhEmailCharactersOnly
                placeholderText: qsTr("Enter password here!")
                label: qsTr("Password for encryption")

                Component.onCompleted: {
                    text = settings.cryptKey
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    Qt.inputMethod.hide();
                }
            }

            Item {
                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge
            }

        }
    }

    onAccepted: {
        settings.port = portField.text
        if (settings.proxy)
            settings.proxyUrl = proxyUrlField.text
        settings.cryptKey = cryptKeyField.text

    }
}
