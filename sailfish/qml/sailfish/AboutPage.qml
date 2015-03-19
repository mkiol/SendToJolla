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

    SilicaListView {
        anchors { top: parent.top; left: parent.left; right: parent.right }

        anchors.leftMargin: Theme.paddingLarge
        anchors.rightMargin: Theme.paddingLarge
        spacing: Theme.paddingLarge

        height: app.height

        header: PageHeader {
            title: qsTr("About")
        }

        model: VisualItemModel {

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "icon.png"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: Theme.fontSizeHuge
                text: APP_NAME
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.highlightColor
                wrapMode: Text.WordWrap
                text: qsTr("Version: %1").arg(VERSION);
            }

            Label {
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left; anchors.right: parent.right
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("An app that allows to send urls and text from PC to your Jolla phone.");
            }

            Label {
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left; anchors.right: parent.right
                font.pixelSize: Theme.fontSizeExtraSmall
                text: PAGE

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Qt.openUrlExternally(PAGE);
                    }
                }
            }

            Label {
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left; anchors.right: parent.right
                font.pixelSize: Theme.fontSizeExtraSmall
                textFormat: Text.RichText
                text: "Copyright &copy; 2015 Michał Kościesza"
            }

            Item {
                height: Theme.paddingLarge
            }
        }

        VerticalScrollDecorator {}
    }
}
