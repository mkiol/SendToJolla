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
                    title: qsTr("Version %1").arg("2.1")
                }
                
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    LogItem {
                        title: "Contacts"
                        description: "Provides contacts management functions. It allows to add new, delete or edit contacts."
                    }
                    
                    LogItem {
                        title: "End-to-end encryption"
                        description: "Data transfer between server and client app (e.g. web browser) can be encrypted."
                    }
                    
                    LogItem {
                        title: "Connection on WiFi tethering"
                        description: "Enables connection when the client app (e.g. web browser on the laptop) is connected to WiFi Hotspot that is provided by your phone."
                    }
                }
                
                Header {
                    title: qsTr("Version %1").arg("2.0")
                }
                
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    LogItem {
                        title: 'Web-based client'
                        description: "This is an alternative way to use Send to Phone, especially for those who don't use Firefox. To open Web client, go to 'Server URL' address in your favorite browser."
                    }
                    
                    LogItem {
                        title: 'Notes'
                        description: "Provides simple notes management functions. It allows to save text as a new note, delete or edit existing note."
                    }
                }
                
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
                        description: 'This is first version of Sent to Phone for BlackBerry.'
                    }
                }
            }
        }

    }
}
