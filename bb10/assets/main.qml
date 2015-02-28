/*
 Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>
 
 This file is part of SendToBerry application.
 
 This Source Code Form is subject to the terms of
 the Mozilla Public License, v.2.0. If a copy of
 the MPL was not distributed with this file, You can
 obtain one at http://mozilla.org/MPL/2.0/.
 */

import bb.cascades 1.2

NavigationPane {
    id: nav
    
    onPopTransitionEnded: {
        if (nav.top.menuEnabled)
            Application.menuEnabled = true;
        else 
            Application.menuEnabled = false;
        page.destroy();
    }
    
    onPushTransitionEnded: {
        if (nav.top.menuEnabled)
            Application.menuEnabled = true;
        else 
            Application.menuEnabled = false;
    }
    
    Menu.definition: MenuDefinition {
        
        settingsAction: SettingsActionItem {
            onTriggered: {
                nav.push(settingsPage.createObject());
            }
        }
        
        helpAction: HelpActionItem {
            title: qsTr("About")
            onTriggered: {
                nav.push(aboutPage.createObject());
            }
        }
    }


    attachedObjects: [
        Notification {
            id: notification
        },
        ComponentDefinition {
            id: settingsPage
            source: "SettingsPage.qml"
        },
        ComponentDefinition {
            id: aboutPage
            source: "AboutPage.qml"
        },
        ComponentDefinition {
            id: changelogPage
            source: "ChangelogPage.qml"
        }
    ]

    FirstPage {
    }
}
