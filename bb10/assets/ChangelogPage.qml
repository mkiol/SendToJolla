/*
 Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>
 
 This file is part of SendToBerry application.
 
 This Source Code Form is subject to the terms of
 the Mozilla Public License, v.2.0. If a copy of
 the MPL was not distributed with this file, You can
 obtain one at http://mozilla.org/MPL/2.0/.
 */

import bb.cascades 1.2

Page {
    id: root
    
    property bool menuEnabled: false
    
    titleBar: TitleBar {
        title: qsTr("Changelog")
    }
    
    Container {
        ScrollView {
            Container {
                Header {
                    title: qsTr("Version %1").arg("1.1")
                }

                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)

                    LogItem {
                        title: 'Initial version for BlackBerry'
                        description: 'This is first version of Sent to Berry for BlackBerry 10.'
                    }
                }
            }
        }

    }
}
