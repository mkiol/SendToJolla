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
    
    property bool menuEnabled: true
    
    onCreationCompleted: {
        server.newEvent.connect(newEventHandler);
        settings.cookieChanged.connect(newCookieHandler);
        
        server.startServer();
    }
    
    function newEventHandler(text) {
        logLabel.text = "> " + text + "\n" + logLabel.text;
    }
    
    function newCookieHandler() {
        urlLabel.text = server.getServerUrl();
    }
    
    titleBar: TitleBar {
        title: qsTr("Send to Berry") + Retranslate.onLocaleOrLanguageChanged
    }

    Container {

        ScrollView {
            Container {
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    Label {
                        text: server.running ? 
                        qsTr("Server is running. Use below URL to configure client application.") :
                        qsTr("Server is not running.")
                        multiline: true
                    }
                    
                    Label {
                        id: urlLabel
                        text: server.getServerUrl()
                        multiline: true
                        textStyle.color: utils.primary()
                    }
                }
                
                Container {
                    topPadding: utils.du(5)
                }
                
                Header {
                    title: qsTr("log")
                }
                
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    Label {
                        id: logLabel
                        multiline: true
                    }
                }

            }
        }
    }

    /*actions: ActionItem {
        title: qsTr("Second page") + Retranslate.onLocaleOrLanguageChanged
        ActionBar.placement: ActionBarPlacement.OnBar
        
        onTriggered: {
            // A second Page is created and pushed when this action is triggered.
            navigationPane.push(secondPageDefinition.createObject());
        }
    }*/
}
