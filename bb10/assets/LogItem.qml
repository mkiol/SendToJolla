/*
 Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>
 
 This file is part of SendToBerry application.
 
 This Source Code Form is subject to the terms of
 the Mozilla Public License, v.2.0. If a copy of
 the MPL was not distributed with this file, You can
 obtain one at http://mozilla.org/MPL/2.0/.
 */

import bb.cascades 1.2

Container {
    id: root
    
    topPadding: 0
    bottomPadding: 0
    
    property string title
    property string description
    
    Label {
        text: title
        textStyle.base: SystemDefaults.TextStyles.BodyText
        multiline: true
        topMargin: 0; topPadding: 0; bottomMargin: 0; bottomPadding: 0;
    }
    
    Label {
        topMargin: utils.du(1); topPadding: 0;
        text: description
        textStyle.base: SystemDefaults.TextStyles.SubtitleText
        multiline: true
    }
    
    Container {
        preferredHeight: utils.du(2)
    }
}
