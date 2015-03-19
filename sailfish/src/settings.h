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
    Q_PROPERTY (QString cookie READ getCookie WRITE setCookie NOTIFY cookieChanged)

public:
    static Settings* instance();

    Q_INVOKABLE void generateCookie();

    void setPort(int value);
    int getPort();

    void setBrowser(QString value);
    QString getBrowser();

    void setCookie(QString value);
    QString getCookie();

signals:
    void portChanged();
    void browserChanged();
    void cookieChanged();

private:
    QSettings settings;
    static Settings *inst;

    explicit Settings(QObject *parent = 0);
    QString getRandomString() const;
};

#endif // SETTINGS_H
