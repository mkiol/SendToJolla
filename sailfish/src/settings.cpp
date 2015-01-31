/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QVariant>
#include <QDebug>
#include <QChar>

#include "settings.h"

Settings* Settings::inst = 0;

Settings::Settings(QObject *parent) : QObject(parent), settings()
{
}

Settings* Settings::instance()
{
    if (Settings::inst == NULL) {
        Settings::inst = new Settings();
    }

    return Settings::inst;
}

void Settings::setPort(int value)
{
    if (getPort() != value) {
        settings.setValue("port", value);
        emit portChanged();
    }
}

int Settings::getPort()
{
    return settings.value("port", 9090).toInt();
}

void Settings::setBrowser(QString value)
{
    if (getBrowser() != value) {
        settings.setValue("browser", value);
        emit browserChanged();
    }
}

QString Settings::getBrowser()
{
    return settings.value("browser", "sailfish-browser").toString();
}

void Settings::setCookie(QString value)
{
    if (getCookie() != value) {
        settings.setValue("cookie", value);
        emit cookieChanged();
    }
}

QString Settings::getCookie()
{
    QString cookie = settings.value("cookie", "").toString();
    if (cookie == "") {
        cookie = getRandomString();
        settings.setValue("cookie", cookie);
    }
    return cookie;
}

void Settings::generateCookie()
{
    setCookie(getRandomString());
}

// Source: https://stackoverflow.com/questions/18862963/qt-c-random-string-generation
QString Settings::getRandomString() const
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 5;

   QString randomString;
   for(int i=0; i<randomStringLength; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   qDebug() << randomString;
   return randomString;
}

