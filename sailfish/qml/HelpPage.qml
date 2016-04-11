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

            PageHeader {
                title: qsTr("Help")
            }

            SectionHeader {
                text: settings.proxy ? qsTr("Connection modes") : qsTr("Connection")
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.highlightColor
                text: qsTr("Local server")
                wrapMode: Text.WordWrap
            }

            AnimatedImage {
                source: "local.gif"
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: settings.proxy ? qsTr("In this mode, app acts as a local web server.") : qsTr("The app acts as a local web server.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: settings.proxy ? qsTr("This mode is recommended for cases where your Jolla device and PC are in the same local network, so data can be transferred directly between these two devices.") : qsTr("The connection between Jolla device and PC is possible only when they are in the same local network, so data can be transferred directly between these two devices.")
                wrapMode: Text.WordWrap
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.highlightColor
                text: qsTr("Proxy")
                wrapMode: Text.WordWrap
                visible: settings.proxy
            }

            AnimatedImage {
                source: "proxy.gif"
                visible: settings.proxy
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: qsTr("In this mode, connection between Jolla device and PC is done via Proxy server.")
                wrapMode: Text.WordWrap
                visible: settings.proxy
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: qsTr("This mode is especially recommended for cases where your Jolla device and PC are not the same network and data can't be transferred directly. Typical scenario is when your phone is connected to Internet via cellular network.")
                wrapMode: Text.WordWrap
                visible: settings.proxy
            }

            Label {
                anchors {left: parent.left; right: parent.right; leftMargin: Theme.paddingLarge;rightMargin: Theme.paddingLarge}
                color: Theme.primaryColor
                text: qsTr("Required <i>Proxy server</i> is a single file PHP script that have to be deployed on private WWW server. Script can be downloaded from <a href='" + PAGE + "'> project website</a>.")
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                onLinkActivated: Qt.openUrlExternally(link)
                visible: settings.proxy
            }

            Item {
                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge
            }
        }
    }
}

