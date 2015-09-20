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
        anchors.fill: parent

        header: PageHeader {
            title: qsTr("Changelog")
        }

        model: VisualItemModel {

            SectionHeader {
                text: qsTr("Version %1").arg("2.0")
            }

            LogItem {
                title: 'Web Client'
                description: "Now, you can use Send to Phone directly through browser. To open Web Client, go to Server URL address in your favorite browser. This is alternative to Firefox add-on, especially for those who don't use Mozilla's browser.";
            }

            LogItem {
                title: 'Notes'
                description: "Few features were added enabling simple notes management tasks. There is possibility to save text as a new note or update existing note. Currently, note related actions are available only via Web Client interface.";
            }

            Item {
                height: Theme.paddingMedium
            }

        }

        VerticalScrollDecorator {}
    }

}
