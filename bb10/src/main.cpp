/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <bb/cascades/AbstractPane>
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/device/DisplayInfo>

#include "server.h"
#include "settings.h"
#include "utils.h"

using namespace bb::cascades;

static const char *APP_NAME = "Send to Phone";
static const char *AUTHOR = "Michal Kosciesza <michal@mkiol.net>";
static const char *PAGE = "https://github.com/mkiol/SendToJolla";
static const char *VERSION = "2.0";

Q_DECL_EXPORT int main(int argc, char **argv)
{
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
}
