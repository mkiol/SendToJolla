/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QGuiApplication>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include <sailfishapp.h>

#include "server.h"
#include "settings.h"

static const char *APP_NAME = "Send to Jolla";
static const char *AUTHOR = "Michal Kosciesza <michal@mkiol.net>";
static const char *PAGE = "https://github.com/mkiol/SendToJolla";
static const char *VERSION = "1.0";

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    app->setApplicationDisplayName(APP_NAME);
    app->setApplicationVersion(VERSION);

    view->rootContext()->setContextProperty("APP_NAME", APP_NAME);
    view->rootContext()->setContextProperty("VERSION", VERSION);
    view->rootContext()->setContextProperty("AUTHOR", AUTHOR);
    view->rootContext()->setContextProperty("PAGE", PAGE);

    Settings* settings = Settings::instance();
    view->rootContext()->setContextProperty("settings", settings);

    Server* server = new Server();
    view->rootContext()->setContextProperty("server", server);

    QObject::connect(view->engine(), SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

    view->setSource(SailfishApp::pathTo("qml/sailfish/main.qml"));

    view->show();

    return app->exec();
}
