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
    
    topPadding: utils.du(0)
    bottomPadding: utils.du(3)
    
    property string title
    property string description
    
    Label {
        text: title
        textStyle.base: SystemDefaults.TextStyles.PrimaryText
    }
    
    Label {
        text: description
        textStyle.base: SystemDefaults.TextStyles.SubtitleText
    }
}
