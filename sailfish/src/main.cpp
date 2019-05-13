/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef BB10
#include <bb/cascades/AbstractPane>
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/device/DisplayInfo>
#include "utils.h"
#elif SAILFISH
#include <QGuiApplication>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include <sailfishapp.h>
#endif

#include "server.h"
#include "settings.h"

#ifdef BB10
using namespace bb::cascades;
#endif

static const char *APP_NAME = "Send to Phone";
static const char *AUTHOR = "Michal Kosciesza <michal@mkiol.net>";
static const char *PAGE = "https://github.com/mkiol/SendToJolla";
static const char *VERSION = "2.2 (beta)";

Q_DECL_EXPORT int main(int argc, char **argv)
{
#ifdef SAILFISH
#ifdef CONTACTS
    QCoreApplication::setSetuidAllowed(true);
#endif
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

    view->setSource(SailfishApp::pathTo("qml/main.qml"));

    view->show();

    return app->exec();
#elif BB10
    Application app(argc, argv);

    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(VERSION);

    QmlDocument* qml = QmlDocument::create("asset:///main.qml");

    qml->documentContext()->setContextProperty("APP_NAME", APP_NAME);
    qml->documentContext()->setContextProperty("VERSION", VERSION);
    qml->documentContext()->setContextProperty("AUTHOR", AUTHOR);
    qml->documentContext()->setContextProperty("PAGE", PAGE);

    bb::device::DisplayInfo display;
    qml->setContextProperty("display", &display);

    Settings* settings = Settings::instance();
    qml->documentContext()->setContextProperty("settings", settings);

    Utils utils;
    qml->documentContext()->setContextProperty("utils", &utils);

    Server* server = new Server();
    qml->documentContext()->setContextProperty("server", server);

    QObject::connect(qml->defaultDeclarativeEngine(), SIGNAL(quit()),
            QCoreApplication::instance(), SLOT(quit()));
    AbstractPane *root = qml->createRootObject<AbstractPane>();
    Application::instance()->setScene(root);

    return Application::exec();
#endif
}
