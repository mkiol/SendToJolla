/*
  Copyright (C) 2016 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    anchors.leftMargin: Theme.paddingLarge
    anchors.rightMargin: Theme.paddingLarge
    anchors.left: parent.left
    anchors.right: parent.right
    wrapMode: Text.WordWrap
    horizontalAlignment: Text.AlignHCenter
    font.pixelSize: Theme.fontSizeSmall
    onLinkActivated: {
        Qt.openUrlExternally(link);
    }
}

