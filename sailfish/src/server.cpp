/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QDebug>
#include <QStringList>
#include <QUrl>
#include <QString>

#ifdef BB10
#include <bps/navigator.h>
#include <bb/system/Clipboard>
#include <QByteArray>
#endif

#ifdef SAILFISH
#include <QDesktopServices>
#include <QGuiApplication>
#endif

#include <QHostAddress>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QProcess>
#include <QNetworkConfiguration>
#include <QDateTime>

#include "server.h"
#include "settings.h"

Server::Server(QObject *parent) :
    QObject(parent), isListening(false)
{
    qsrand(QDateTime::currentDateTime().toTime_t());

    connect(&ncm, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));

#ifdef SAILFISH
    clipboard = QGuiApplication::clipboard();
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));
#endif

    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handle(QHttpRequest*, QHttpResponse*)));
}

Server::~Server()
{
    delete server;
}

void Server::onlineStateChanged(bool state)
{
    if (state && !isListening) {
        //qDebug() << "New network conectivity was detected.";
        emit newEvent(tr("New network conectivity was detected."));
        startServer();
        return;
    }

    if (!state && isListening) {
        //qDebug() << "Network was disconnected.";
        emit newEvent(tr("Network was disconnected."));
        stopServer();
        return;
    }
}

#ifdef SAILFISH
void Server::clipboardChanged(QClipboard::Mode mode)
{
    Q_UNUSED(mode)

    QString data = getClipboard();

    if (clipboardData!=data) {
        clipboardData = data;
        emit newEvent(tr("Clipboard data change was detected."));
    }
}
#endif

void Server::stopServer()
{
    server->close();
    isListening = false;
    emit runningChanged();
}


void Server::startServer()
{
    bool bearerOk = false;
    QList<QNetworkConfiguration> activeConfigs = ncm.allConfigurations(QNetworkConfiguration::Active);
    QList<QNetworkConfiguration>::iterator i = activeConfigs.begin();
    while (i != activeConfigs.end()) {
        QNetworkConfiguration c = (*i);
        //qDebug() << c.bearerTypeName() << c.identifier() << c.name();
        if (c.bearerType()==QNetworkConfiguration::BearerWLAN ||
            c.bearerType()==QNetworkConfiguration::BearerEthernet) {
            bearerOk = true;
            break;
        }
        ++i;
    }

    if (!bearerOk) {
        //qWarning() << "Server is failed to start because WLAN is not active.";
        emit newEvent(tr("Server is failed to start because WLAN is not active."));
        return;
    }

    Settings* s = Settings::instance();
    int port = s->getPort();

    if (isListening != server->listen(port)) {
        isListening = !isListening;
        emit runningChanged();
    }

    if (isListening) {
        //qDebug() << "Server listening on" << port << "port.";
        emit newEvent(tr("Server is listening on %1 port.").arg(port));
    } else {
        qWarning() << "Server is failed to start on" << port << "port.";
        emit newEvent(tr("Server is failed to start on %1 port.").arg(port));
    }
}

void Server::handle(QHttpRequest *req, QHttpResponse *resp)
{
    //qDebug() << "handle" <<req->methodString() << req->remoteAddress() << req->remotePort() << req->url();

    QStringList path = req->url().path().split("/");

    bool ok = false;

    Settings* s = Settings::instance();
    if (path.length()>2 && path.at(1)==s->getCookie()) {

        if (path.length()>2 && path.at(2)=="get-clipboard") {
            QString data = getClipboard();
            resp->setHeader("Content-Length", QString::number(data.size()));
            resp->setHeader("Content-Type", "text/plain; charset=utf-8");
            resp->setHeader("Connection", "close");
            resp->writeHead(200);
            resp->end(data.toUtf8());
            ok = true;
        }

        if (path.length()>3 && path.at(2)=="open-url") {
            launchBrowser(path.at(3));
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(204);
            resp->end("");
            ok = true;
        }

        if (path.length()>2 && path.at(2)=="set-clipboard" && req->method()==QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceived()));
            req->storeBody();
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(204);
            resp->end("");
            ok = true;
        }
    }

    if (ok) {
        emit newEvent(QString("%1 request from %2 received.").arg(path.at(2)).arg(req->remoteAddress()));
    } else {
        resp->setHeader("Content-Length", "0");
        resp->setHeader("Connection", "close");
        resp->writeHead(400);
        resp->end("");
        emit newEvent(tr("Unknown request from %1 received.").arg(req->remoteAddress()));
    }
}

void Server::bodyReceived() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());
    //qDebug() << "body" <<req->body();
    setClipboard(QString(req->body()));
}

QString Server::getServerUrl()
{
    QString url = "";
    Settings* s = Settings::instance();
    int port = s->getPort();

    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            return QString("http://%1:%2/%3").arg(address.toString()).arg(port).arg(s->getCookie());
        }
    }

    return url;
}

void Server::launchBrowser(QString data)
{
    Settings* s = Settings::instance();
    QString browser = s->getBrowser();
    QUrl url(QUrl::fromPercentEncoding(data.toUtf8()));

#ifdef SAILFISH
    if (browser == "sailfish-browser") {
        QDesktopServices::openUrl(url);
    }
    if (browser == "firefox") {
        QProcess process;
        process.startDetached("apkd-launcher /data/app/org.mozilla.firefox-1.apk org.mozilla.firefox/org.mozilla.gecko.BrowserApp");
    }
#endif
#ifdef BB10
    navigator_invoke(url.toString().toStdString().c_str(),0);
#endif
}

void Server::setClipboard(QString data)
{
#ifdef SAILFISH
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
#endif
#ifdef BB10
    bb::system::Clipboard clipboard;
    clipboard.clear();
    clipboard.insert("text/plain", data.toUtf8());
#endif
}

QString Server::getClipboard()
{
#ifdef SAILFISH
    QClipboard *clipboard = QGuiApplication::clipboard();
    return clipboard->text();
#endif
#ifdef BB10
    bb::system::Clipboard clipboard;
    return QString(clipboard.value("text/plain"));
#endif
}

bool Server::isRunning()
{
    return isListening;
}
