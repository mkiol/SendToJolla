/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToJolla application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QDebug>
#include <QStringList>
#include <QDesktopServices>
#include <QUrl>
#include <QString>

#include <QHostAddress>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QProcess>
#include <QClipboard>
#include <QGuiApplication>
#include <QNetworkConfiguration>
#include <QDateTime>

#include "server.h"
#include "settings.h"

Server::Server(QObject *parent) :
    QObject(parent), isListening(false)
{
    qsrand(QDateTime::currentDateTime().toTime_t());
    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handle(QHttpRequest*, QHttpResponse*)));
}

Server::~Server()
{
    delete server;
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
        qWarning() << "Server failed to start because WLAN is not active.";
        emit newEvent("Server failed to start because WLAN is not active.");
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
        emit newEvent(QString("Server listening on %1 port").arg(port));
    } else {
        qWarning() << "Server failed to start on" << port << "port.";
        emit newEvent(QString("Server failed to start on %1 port").arg(port));
    }
}

void Server::handle(QHttpRequest *req, QHttpResponse *resp)
{
    //qDebug() << "handle" <<req->methodString() << req->remoteAddress() << req->remotePort() << req->url();

    QStringList path = req->url().path().split("/");

    bool ok = false;

    Settings* s = Settings::instance();
    if (path.length()>2 && path.at(1)==s->getCookie()) {

        if (path.length()>3 && path.at(2)=="open-url") {
            launchBrowser(path.at(3));
            ok = true;
        }

        if (path.length()>2 && path.at(2)=="set-clipboard" && req->method()==QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceived()));
            req->storeBody();
            //setClipboard(QString(req->body()));
            ok = true;
        }
    }

    resp->setHeader("Content-Length", "0");
    resp->setHeader("Connection", "close");

    if (ok) {
        emit newEvent(QString("%1 request from %2 received.").arg(path.at(2)).arg(req->remoteAddress()));
        resp->writeHead(204);
    } else {
        emit newEvent(QString("Unknown request from %1 received.").arg(req->remoteAddress()));
        resp->writeHead(400);
    }

    resp->end("");
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
             //url += QString("http://%1:%2/\n").arg(address.toString()).arg(port);
            return QString("http://%1:%2/%3/").arg(address.toString()).arg(port).arg(s->getCookie());
        }
    }

    return url;
}

void Server::launchBrowser(QString data)
{
    Settings* s = Settings::instance();
    QString browser = s->getBrowser();
    QUrl url(QUrl::fromPercentEncoding(data.toUtf8()));

    if (browser == "sailfish-browser") {
        QDesktopServices::openUrl(url);
    }
    if (browser == "firefox") {
        QProcess process;
        process.startDetached("apkd-launcher /data/app/org.mozilla.firefox-1.apk org.mozilla.firefox/org.mozilla.gecko.BrowserApp");
    }
}

void Server::setClipboard(QString data)
{
    QString text = QUrl::fromPercentEncoding(data.toUtf8());
    //qDebug() << text;
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}

bool Server::isRunning()
{
    return isListening;
}
