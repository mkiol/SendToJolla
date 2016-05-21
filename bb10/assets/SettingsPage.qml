/*
 * Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>
 * 
 * This file is part of SendToBerry application.
 * 
 * This Source Code Form is subject to the terms of
 * the Mozilla Public License, v.2.0. If a copy of
 * the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

import bb.cascades 1.2

Page {

    id: root
    
    property bool menuEnabled: false

    titleBar: TitleBar {
        title: qsTr("Settings")
    }

    function validatePort(port) {
        return parseInt(port).toString() == port && parseInt(port) > 1023 && parseInt(port) <= 65535;
    }

    function accept() {
        if (! validatePort(portField.text)) {
            notification.show(qsTr("Port number is invalid!"));
            return;
        }
        
        if (cryptSwitch.checked && cryptKeyField.text.trim() === "") {
            notification.show(qsTr("Password is invalid!"));
            return;
        }

        settings.port = portField.text;
        settings.cryptKey = cryptKeyField.text;

        nav.pop();
    }

    actions: ActionItem {
        title: qsTr("Save")
        imageSource: "asset:///ic_save.png"
        ActionBar.placement: ActionBarPlacement.OnBar

        onTriggered: {
            accept();
        }
    }

    Container {

        ScrollView {
            Container {
                
                Header {
                    title: qsTr("Server URL cookie")
                }
                
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    
                    Label {
                        text: settings.cookie
                        textStyle.color: utils.primary()
                        verticalAlignment: VerticalAlignment.Center
                    }

                    Button {
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Generate new cookie")
                        onClicked: {
                            settings.generateCookie();
                        }
                    }
                }

                Header {
                    title: qsTr("Local server")
                }

                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    ToggleComponent {
                        id: serverToggle
                        text: qsTr("Auto start on startup")
                        checked: settings.startLocalServer
                        enabled: validatePort(portField.text)
                        onCheckedChanged: {
                            onCheckedChanged: {
                                settings.startLocalServer = checked;
                            }
                        }
                    }

                    Label {
                        text: qsTr("Listening port number. Changes will take effect after app restart.")
                        multiline: true
                    }

                    Container {
                        topPadding: utils.du(1)
                    }

                    TextField {
                        id: portField
                        hintText: qsTr("Enter port number here!")
                        inputMode: TextFieldInputMode.NumbersAndPunctuation
                        preferredWidth: display.pixelSize.width / 2

                        validator: Validator {
                            mode: ValidationMode.FocusLost
                            errorMessage: qsTr("Port number is invalid! Allowed port range is 1024-65535.")
                            onValidate: {
                                if (validatePort(portField.text))
                                    state = ValidationState.Valid;
                                else
                                    state = ValidationState.Invalid;
                            }
                        }

                        onCreationCompleted: {
                            text = settings.port;
                        }
                    }
                }
                
                Header {
                    title: qsTr("Encryption")
                }
                
                Container {
                    leftPadding: utils.du(2)
                    rightPadding: utils.du(2)
                    topPadding: utils.du(2)
                    bottomPadding: utils.du(2)
                    
                    ToggleComponent {
                        id: cryptSwitch
                        text: qsTr("Encryption required")
                        checked: settings.crypt
                        onCheckedChanged: {
                            settings.crypt = checked;
                        }
                    }
                    
                    Container {
                        topPadding: utils.du(1)
                    }
                    
                    TextField {
                        id: cryptKeyField
                        hintText: qsTr("Enter password here!")
                        inputMode: TextFieldInputMode.Password
                        visible: cryptSwitch.checked
                        //preferredWidth: display.pixelSize.width / 2
                        
                        onCreationCompleted: {
                            text = settings.cryptKey;
                        }
                    }
                }
            }
        }
    }
}
