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
        server.runningChanged.connect(localServerRunningHandler);

        if (settings.startLocalServer)
            server.startLocalServer();
    }
    
    function newEventHandler(text) {
        logLabel.text = "> " + text + "\n" + logLabel.text;
    }
    
    function newCookieHandler() {
        urlLabel.text = server.getServerUrl();
    }
    
    function localServerRunningHandler() {
        serverToggle.checked = server.localServerRunning;
    }
    
    titleBar: TitleBar {
        title: APP_NAME
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
                        text: qsTr("Toggle below to enable or disable %1 local server.").arg(APP_NAME)
                        multiline: true
                    }
                    
                    ToggleComponent {
                        id: serverToggle
                        text: qsTr("Local server")
                        onCheckedChanged: {
                            if (checked && !server.localServerRunning) {
                                checked = server.startLocalServer();
                                return;
                            }
                            if (!checked && server.localServerRunning) {
                                server.stopLocalServer();
                                return;
                            }
                        }
                    }

                    Label {
                        visible: server.localServerRunning
                        textStyle.color: utils.secondaryText()
                        text: qsTr("To open Web client, go to the URL address below in your favorite web browser.")
                        multiline: true
                    }
                    
                    Label {
                        id: urlLabel
                        text: server.localServerRunning ? server.getLocalServerUrl() : ""
                        multiline: true
                        textStyle.color: utils.primary()
                        //visible: server.localServerRunning
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

}
