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

    property alias text: nameLabel.text
    property alias description: descriptionLabel.text
    property alias checked: toggle.checked
    property alias iconSource: image.imageSource

    topPadding: utils.du(1)
    bottomPadding: topPadding

    Container {

        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }
        
        Label {
            id: nameLabel
            
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
            
            verticalAlignment: VerticalAlignment.Center
        }
        
        ImageView {
            id: image
            visible: imageSource != ""
        }

        ToggleButton {
            id: toggle
            
            verticalAlignment: VerticalAlignment.Center
        }
    }
    
    Container {
        topMargin: utils.du(1)
        visible: root.description != ""
        
        Label {
            id: descriptionLabel
            multiline: true
            textStyle.base: SystemDefaults.TextStyles.SubtitleText
            textStyle.color: utils.secondaryText()
        }
        
    }
    
    
}