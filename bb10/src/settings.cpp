/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

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

void Settings::setCryptKey(QByteArray value)
{
    if (getCryptKey() != value) {
        settings.setValue("cryptkey", value);
        emit cryptKeyChanged();
    }
}

QByteArray Settings::getCryptKey()
{
    QByteArray key = settings.value("cryptkey", "").toByteArray();
    if (key == "") {
        key = getRandomString().toUtf8();
        settings.setValue("cryptkey", key);
    }
    return key;
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

void Settings::setProxyUrl(QString value)
{
    if (getProxyUrl() != value) {
        settings.setValue("proxyurl", value);
        emit proxyUrlChanged();
    }
}

QString Settings::getProxyUrl()
{
    return settings.value("proxyurl", "").toString();
}

void Settings::setIgnoreSSL(bool value)
{
    if (isIgnoreSSL() != value) {
        settings.setValue("ignoressl", value);
        emit ignoreSSLChanged();
    }
}

bool Settings::isIgnoreSSL()
{
    return settings.value("ignoressl", false).toBool();
}

void Settings::setStartProxy(bool value)
{
    if (getStartProxy() != value) {
        settings.setValue("startproxy", value);
        emit startProxyChanged();
    }
}

bool Settings::getStartProxy()
{
#ifdef PROXY
    return settings.value("startproxy", false).toBool();
#else
    return false;
#endif
}

void Settings::setStartLocalServer(bool value)
{
    if (getStartLocalServer() != value) {
        settings.setValue("startlocalserver", value);
        emit startLocalServerChanged();
    }
}

void Settings::setProxy(bool value)
{
}

bool Settings::getProxy()
{
#ifdef PROXY
    return true;
#else
    return false;
#endif
}

bool Settings::getStartLocalServer()
{
    return settings.value("startlocalserver", true).toBool();
}

void Settings::setCrypt(bool value)
{
    if (getCrypt() != value) {
        settings.setValue("crypt", value);
        emit cryptChanged();
    }
}

bool Settings::getCrypt()
{
    return settings.value("crypt", false).toBool();
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

