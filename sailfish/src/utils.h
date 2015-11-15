/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QString>

#ifdef BB10
#include <bb/device/DisplayInfo>
#include <bb/cascades/Color>
#endif

class Utils : public QObject
{
    Q_OBJECT

public:

#ifdef SAILFISH
    static void setPrivileged(bool privileged);
#endif

    explicit Utils(QObject *parent = 0);

#ifdef BB10
    Q_INVOKABLE bool checkOSVersion(int major, int minor, int patch = 0, int build = 0);
    Q_INVOKABLE int du(float value);
    Q_INVOKABLE bb::cascades::Color background();
    Q_INVOKABLE bb::cascades::Color plain();
    Q_INVOKABLE bb::cascades::Color plainBase();
    Q_INVOKABLE bb::cascades::Color textOnPlain();
    Q_INVOKABLE bb::cascades::Color text();
    Q_INVOKABLE bb::cascades::Color secondaryText();
    Q_INVOKABLE bb::cascades::Color primary();
#endif

private:
#ifdef BB10
    bb::device::DisplayInfo display;
#endif
};

#endif // UTILS_H
