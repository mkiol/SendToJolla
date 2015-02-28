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
        title: qsTr("About")
        appearance: TitleBarAppearance.Plain
    }
    
    /*actions: [
        ActionItem {
            title: qsTr("Changelog")
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: "asset:///changelog.png"
            onTriggered: {
                nav.push(changelogPage.createObject());
            }
        }
    ]*/

    ScrollView {

        Container {

            preferredWidth: display.pixelSize.width
            verticalAlignment: VerticalAlignment.Center
            
            Container {
                topPadding: utils.du(4)
            }

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                imageSource: "asset:///icon.png"
            }

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.textAlign: TextAlign.Center
                textStyle.base: SystemDefaults.TextStyles.BigText
                text: APP_NAME
            }

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.textAlign: TextAlign.Center
                textStyle.color: utils.text()
                text: qsTr("Version: %1").arg(VERSION)
            }

            PaddingLabel {
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.textAlign: TextAlign.Center
                text: qsTr("An app that allows you to send URLs and text between PC and your BlackBerry phone via WiFi.")
            }

            PaddingLabel {
                horizontalAlignment: HorizontalAlignment.Center
                textFormat: TextFormat.Html
                text: "<a href='%1'>%2</a>".arg(PAGE).arg(PAGE)
            }
            
            Container {
                topPadding: utils.du(6)
            }
            
            Header {
                title: qsTr("Copyright statement")
            }
            
            PaddingLabel {
                textStyle.textAlign: TextAlign.Left
                textFormat: TextFormat.Html
                text: "Copyright &copy; 2015 Michal Kosciesza"
            }
            
            PaddingLabel {
                textStyle.textAlign: TextAlign.Left
                textFormat: TextFormat.Html
                text: qsTr("This software is distributed under "+
                "<a href='https://www.mozilla.org/MPL/2.0/'>Mozilla Public License Version 2.0</a>")
            }
            
            Container {
                topPadding: utils.du(2)
            }
            
            Header {
                title: qsTr("Third-party copyrights")
            }
            
            PaddingLabel {
                textStyle.textAlign: TextAlign.Left
                text: qsTr("Send to Berry utilizes third party software. Such third party software is copyrighted by their owners as indicated below.")
            }
            
            PaddingLabel {
                textFormat: TextFormat.Html
                text: "Copyright &copy; 2011-2013 Nikhil Marathe"
            }
            
            Container {
                topPadding: utils.du(5)
            }
        }
    }
}
