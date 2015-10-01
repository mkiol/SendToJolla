/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QDebug>
#include <QTextStream>

#ifdef BB10
#include <QtGui/QApplication>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/ThemeSupport>
#include <bb/cascades/Theme>
#include <bb/cascades/Application>
#include <bb/cascades/ColorTheme>
#include <bb/cascades/VisualStyle>
#include <bb/platform/PlatformInfo>
#include <bb/device/DisplayTechnology>
#include <bb/platform/PlatformInfo>
#include <QtCore/QString>
#include <QtCore/QList>
#endif

#include "utils.h"

Utils::Utils(QObject *parent) :
    QObject(parent)
{}

#ifdef BB10
// Source: http://hecgeek.blogspot.com/2014/10/blackberry-10-multiple-os-versions-from.html
bool Utils::checkOSVersion(int major, int minor, int patch, int build)
{
    bb::platform::PlatformInfo platformInfo;
    const QString osVersion = platformInfo.osVersion();
    const QStringList parts = osVersion.split('.');
    const int partCount = parts.size();

    if(partCount < 4) {
        // Invalid OS version format, assume check failed
        return false;
    }

    // Compare the base OS version using the same method as the macros
    // in bbndk.h, which are duplicated here for maximum compatibility.
    int platformEncoded = (((parts[0].toInt())*1000000)+((parts[1].toInt())*1000)+(parts[2].toInt()));
    int checkEncoded = (((major)*1000000)+((minor)*1000)+(patch));

    if(platformEncoded < checkEncoded) {
        return false;
    }
    else if(platformEncoded > checkEncoded) {
        return true;
    }
    else {
        // The platform and check OS versions are equal, so compare the build version
        int platformBuild = parts[3].toInt();
        return platformBuild >= build;
    }
}

int Utils::du(float value)
{
    int width = display.pixelSize().width();
    int height = display.pixelSize().height();

    if (width==720 && height==1280) {
        return value*8;
    }

    if (width==720 && height==720) {
        return value*8;
    }

    if (width==768 && height==1280) {
        return value*10;
    }

    if (width==1440 && height==1440) {
        return value*12;
    }

    return 8*value;
}

bb::cascades::Color Utils::background()
{
    bb::cascades::ColorTheme* colorTheme = bb::cascades::Application::instance()->themeSupport()->theme()->colorTheme();
    bool isOled = display.displayTechnology()==bb::device::DisplayTechnology::Oled;
    bool is103 = checkOSVersion(10,3);

    switch (colorTheme->style()) {
        case bb::cascades::VisualStyle::Bright:
            if (is103)
                return bb::cascades::Color::White;
            if (isOled)
                return bb::cascades::Color::fromARGB(0xfff2f2f2);
            return bb::cascades::Color::fromARGB(0xfff8f8f8);
            break;
        case bb::cascades::VisualStyle::Dark:
            if (is103)
                return bb::cascades::Color::Black;
            return bb::cascades::Color::fromARGB(0xff121212);
            break;
    }

    return bb::cascades::Color::Black;
}

bb::cascades::Color Utils::plain()
{
    bb::cascades::ColorTheme* colorTheme = bb::cascades::Application::instance()->themeSupport()->theme()->colorTheme();

    switch (colorTheme->style()) {
        case bb::cascades::VisualStyle::Bright:
            return bb::cascades::Color::fromARGB(0xfff0f0f0);
            break;
        case bb::cascades::VisualStyle::Dark:
            return bb::cascades::Color::fromARGB(0xff323232);
            break;
    }

    return bb::cascades::Color::fromARGB(0xff323232);
}

bb::cascades::Color Utils::plainBase()
{
    bb::cascades::ColorTheme* colorTheme = bb::cascades::Application::instance()->themeSupport()->theme()->colorTheme();

    switch (colorTheme->style()) {
        case bb::cascades::VisualStyle::Bright:
            return bb::cascades::Color::fromARGB(0xffe6e6e6);
            break;
        case bb::cascades::VisualStyle::Dark:
            return bb::cascades::Color::fromARGB(0xff282828);
            break;
    }

    return bb::cascades::Color::fromARGB(0xff282828);
}

bb::cascades::Color Utils::text()
{
    bb::cascades::ColorTheme* colorTheme = bb::cascades::Application::instance()->themeSupport()->theme()->colorTheme();

    switch (colorTheme->style()) {
        case bb::cascades::VisualStyle::Bright:
            return bb::cascades::Color::fromARGB(0xff323232);
            break;
        case bb::cascades::VisualStyle::Dark:
            return bb::cascades::Color::fromARGB(0xfff0f0f0);
            break;
    }

    return bb::cascades::Color::fromARGB(0xfff0f0f0);
}

bb::cascades::Color Utils::secondaryText()
{
    bb::cascades::ColorTheme* colorTheme = bb::cascades::Application::instance()->themeSupport()->theme()->colorTheme();

    switch (colorTheme->style()) {
        case bb::cascades::VisualStyle::Bright:
            return bb::cascades::Color::fromARGB(0xff646464);
            break;
        case bb::cascades::VisualStyle::Dark:
            return bb::cascades::Color::fromARGB(0xff969696);
            break;
    }

    return bb::cascades::Color::fromARGB(0xff969696);
}

bb::cascades::Color Utils::primary()
{
    return bb::cascades::Color::fromARGB(0xff0092cc);
}

bb::cascades::Color Utils::textOnPlain()
{
    return text();
}
#endif

