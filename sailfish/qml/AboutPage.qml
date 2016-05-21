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

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            anchors.left: parent.left; anchors.right: parent.right
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("About")
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "icon.png"
            }

            PaddedLabel {
                font.pixelSize: Theme.fontSizeHuge
                text: APP_NAME
            }

            PaddedLabel {
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.highlightColor
                text: qsTr("Version: %1").arg(VERSION);
            }

            PaddedLabel {
                text: qsTr("Web app that simplifies clipboard, bookmarks, notes & contacts transfer between the PC and your phone via WiFi");
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Website");
                onClicked: Qt.openUrlExternally(PAGE)
            }

            SectionHeader {
                text: qsTr("Copyright & license")
            }

            PaddedLabel {
                horizontalAlignment: Text.AlignLeft
                textFormat: Text.RichText
                text: "Copyright &copy; 2016 Michal Kosciesza"
            }

            PaddedLabel {
                horizontalAlignment: Text.AlignLeft
                textFormat: Text.RichText
                text: qsTr("This software is distributed under the terms of the Mozilla Public License v.2.0")
            }

            SectionHeader {
                text: qsTr("Third party components copyrights")
            }

            PaddedLabel {
                horizontalAlignment: Text.AlignLeft
                textFormat: Text.RichText
                text: "QHTTPServer - Copyright &copy; 2011-2013 Nikhil Marathe"
            }

            Spacer {}

            Button {
                text: qsTr("Changelog")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: pageStack.push(Qt.resolvedUrl("ChangelogPage.qml"))
            }

            Spacer {}
        }

        VerticalScrollDecorator {}
    }
}
