/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

class Settings: public QObject
{
    Q_OBJECT

    Q_PROPERTY (int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY (QString browser READ getBrowser WRITE setBrowser NOTIFY browserChanged)
    Q_PROPERTY (QByteArray cryptKey READ getCryptKey WRITE setCryptKey NOTIFY cryptKeyChanged)
    Q_PROPERTY (QString cookie READ getCookie WRITE setCookie NOTIFY cookieChanged)
    Q_PROPERTY (QString proxyUrl READ getProxyUrl WRITE setProxyUrl NOTIFY proxyUrlChanged)
    Q_PROPERTY (bool ignoreSSL READ isIgnoreSSL WRITE setIgnoreSSL NOTIFY ignoreSSLChanged)
    Q_PROPERTY (bool startLocalServer READ getStartLocalServer WRITE setStartLocalServer NOTIFY startLocalServerChanged)
    Q_PROPERTY (bool startProxy READ getStartProxy WRITE setStartProxy NOTIFY startProxyChanged)
    Q_PROPERTY (bool crypt READ getCrypt WRITE setCrypt NOTIFY cryptChanged)

public:
    static Settings* instance();

    Q_INVOKABLE void generateCookie();

    void setPort(int value);
    int getPort();

    void setBrowser(QString value);
    QString getBrowser();

    void setCookie(QString value);
    QString getCookie();

    void setProxyUrl(QString value);
    QString getProxyUrl();

    void setCryptKey(QByteArray value);
    QByteArray getCryptKey();

    void setIgnoreSSL(bool value);
    bool isIgnoreSSL();

    void setStartProxy(bool value);
    bool getStartProxy();

    void setStartLocalServer(bool value);
    bool getStartLocalServer();

    void setCrypt(bool value);
    bool getCrypt();

signals:
    void portChanged();
    void browserChanged();
    void cookieChanged();
    void proxyUrlChanged();
    void ignoreSSLChanged();
    void startLocalServerChanged();
    void startProxyChanged();
    void cryptKeyChanged();
    void cryptChanged();

private:
    QSettings settings;
    static Settings *inst;

    explicit Settings(QObject *parent = 0);
    QString getRandomString() const;
};

#endif // SETTINGS_H
