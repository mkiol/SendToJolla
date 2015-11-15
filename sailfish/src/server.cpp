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
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QSignalMapper>
#include <QCryptographicHash>
#include <QtMath>

#ifdef BB10
#include <bps/navigator.h>
#include <bb/system/Clipboard>
#include <bb/pim/notebook/Notebook>
#include <bb/pim/account/Account>
#include <bb/pim/account/Service>
#include <bb/pim/account/AccountService>
#include <bb/pim/notebook/NotebookService>
#include <QByteArray>
#include <QtGui/QDesktopServices>

#include "QJson/serializer.h"
#include "QJson/parser.h"

using namespace bb::pim::account;
using namespace bb::pim::notebook;
#endif

#ifdef SAILFISH
#include <QDesktopServices>
#include <QGuiApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QTextStream>
#include <QVariantMap>
#include <QVariantList>
#include <QUrlQuery>

#ifdef CONTACTS
#include <QtContacts/QContact>
#include <QtContacts/QContactDetail>
#include <QtContacts/QContactName>
#include <QtContacts/QContactEmailAddress>
#include <QtContacts/QContactDisplayLabel>
#include <QtContacts/QContactAddress>
#include <QtContacts/QContactBirthday>
#include <QtContacts/QContactNote>
#include <QtContacts/QContactOrganization>
#include <QtContacts/QContactPhoneNumber>
#include <QtContacts/QContactId>
#include <QtContacts/QContactSortOrder>
#include <QtContacts/QContactFilter>
#include <QtContacts/QContactActionDescriptor>
#include <QtContacts/QContactAvatar>
#endif

#include "sailfishapp.h"
#endif

#include <QHostAddress>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QProcess>
#include <QNetworkConfiguration>
#include <QDateTime>

#include "server.h"
#include "settings.h"
#include "utils.h"

#ifdef CONTACTS
using namespace QtContacts;
#endif

Server::Server(QObject *parent) :
    QObject(parent), isListening(false)
{
    qsrand(QDateTime::currentDateTime().toTime_t());

    connect(&ncm, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));

#ifdef SAILFISH
    clipboard = QGuiApplication::clipboard();
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));
    openNotesDB();
#endif

    // Proxy
    connect(&proxy, SIGNAL(dataReceived(QByteArray)), this, SLOT(proxyDataReceived(QByteArray)));
    connect(&proxy, SIGNAL(openChanged()), this, SLOT(proxyOpenHandler()));
    connect(&proxy, SIGNAL(error(int)), this, SLOT(proxyErrorHandler(int)));
    connect(&proxy, SIGNAL(closingChanged()), this, SLOT(proxyBusyHandler()));
    connect(&proxy, SIGNAL(openingChanged()), this, SLOT(proxyBusyHandler()));

    // Local server
    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handleLocalServer(QHttpRequest*, QHttpResponse*)));

    // New cookie
    Settings* s = Settings::instance();
    connect(s, SIGNAL(cookieChanged()), this, SLOT(cookieChangedHandler()));

#ifdef CONTACTS
    // Contacts
    Utils::setPrivileged(true);
    cm = new QContactManager();
    Utils::setPrivileged(false);
#endif
}

Server::~Server()
{
#ifdef SAILFISH
    closeNotesDB();
#ifdef CONTACTS
    delete cm;
#endif
#endif
    delete server;
}

void Server::onlineStateChanged(bool state)
{
    Settings* s = Settings::instance();

    if (state && !isListening && s->getStartLocalServer()) {
        //qDebug() << "New network conectivity was detected.";
        //emit newEvent(tr("New network conectivity was detected"));
        startLocalServer();
        return;
    }

    if (!state && isListening) {
        //qDebug() << "Network was disconnected.";
        emit newEvent(tr("Network was disconnected"));
        stopLocalServer();
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
        emit newEvent(tr("Clipboard data change was detected"));
    }
}
#endif

void Server::stopLocalServer()
{
    server->close();
    isListening = false;
    emit runningChanged();
    emit newEvent(tr("Local server is stopped"));
}


void Server::startLocalServer()
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
        //qWarning() << "Local server is failed to start because WLAN is not active.";
        emit newEvent(tr("Local server is failed to start because WLAN is not active"));
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
        emit newEvent(tr("Local server is started"));
    } else {
        qWarning() << "Local server is failed to start on" << port << "port";
        emit newEvent(tr("Local server is failed to start on %1 port").arg(port));
    }
}

void Server::handleProxy(const QJsonObject &obj)
{
    Settings* s = Settings::instance();

    if (!obj["uid"].isString() || obj["uid"].toString().isEmpty()) {
        qWarning() << "Uid param is missing!";
        sendProxyResponse(0,400);
        return;
    }

    QString uid = obj["uid"].toString();

    if (!obj["url"].isString() || obj["url"].toString().isEmpty()) {
        qWarning() << "Url param is missing!";
        sendProxyResponse(uid,400);
        return;
    }

    QUrl url(obj["url"].toString());
    if (!url.isValid()) {
        qWarning() << "Url is invalid!";
        sendProxyResponse(uid,400);
        return;
    }

    if (!obj["source"].isString() || obj["source"].toString().isEmpty()) {
        qWarning() << "Source param is missing!";
        sendProxyResponse(uid,400);
        return;
    }

    QString source = obj["source"].toString();

    QUrlQuery query(url);

    if (query.hasQueryItem("api")) {

        QString api = query.queryItemValue("api");
        QString data = query.queryItemValue("data");

        QByteArray body = "";
        if (obj["body"].isString() && !obj["body"].toString().isEmpty()) {
            body = QByteArray::fromBase64(obj["body"].toString().toUtf8());
        }

        if (api == "open-url") {

            if (data.isEmpty()) {
                sendProxyResponse(uid,400);
                return;
            }

            launchBrowser(data);
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid);
            return;
        }

        if (api == "options") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json", getOptions());
            return;
        }

        if (api == "web-client") {

            QByteArray body;

            data = data.isEmpty() ? "index.html" : data;
            if (getWebContent(data, body)) {
                QString contentType = getContentType(data);
                if (contentType.isEmpty()) {
                    qWarning() << "Unknown content type!";
                    sendProxyResponse(uid, 404);
                } else {
                    sendProxyResponse(uid, 200, contentType, body);
                }
                return;
            }

            sendProxyResponse(uid, 404);
            return;
        }

        if (api == "get-bookmarks") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json; charset=utf-8", getBookmarks(), s->getCrypt());
            return;
        }

        if (api == "get-notes-list") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json; charset=utf-8", getNotes(), s->getCrypt());
            return;
        }

        if (api == "get-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json; charset=utf-8", getNote(data.toInt()), s->getCrypt());
            return;
        }

        if (api == "get-clipboard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "text/plain; charset=utf-8", getClipboard().toUtf8(), s->getCrypt());
            return;
        }

        if (api == "delete-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            if (deleteNote(data.toInt()))
                sendProxyResponse(uid);
            else
                sendProxyResponse(uid, 404);
            return;
        }

        if (api == "delete-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            if (deleteBookmark(data.toInt()))
                sendProxyResponse(uid);
            else
                sendProxyResponse(uid, 404);
            return;
        }

        if (api == "update-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!updateNote(data.toInt(), body)) {
                qWarning() << "Error updatig note with id:" << data.toInt();
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
            return;
        }

        if (api == "update-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!updateBookmark(data.toInt(), body)) {
                qWarning() << "Error updatig bookmark with id:" << data.toInt();
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
            return;
        }

        if (api == "create-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!createNote(body)) {
                qWarning() << "Error creating note!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
            return;
        }

        if (api == "create-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!createBookmark(body)) {
                qWarning() << "Error creating bookmark!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
            return;
        }

        if (api == "set-clipboard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
            }

            setClipboard(QString(data));

            sendProxyResponse(uid);
            return;
        }

        if (api == "get-contacts") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
#ifdef CONTACTS
            sendProxyResponse(uid, 200, "application/json", getContacts(data), s->getCrypt());
#else
            sendProxyResponse(uid, 404);
#endif
            return;
        }

        if (api == "get-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendProxyResponse(uid,400);
                return;
            }

#ifdef CONTACTS
            QByteArray json = getContact(data.toInt());
            if (json.isEmpty())
                sendProxyResponse(uid, 404);
            else
                sendProxyResponse(uid, 200, "application/json", json, s->getCrypt());
#else
            sendProxyResponse(uid, 404);
#endif
            return;
        }

        if (api == "create-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

#ifdef CONTACTS
            if (!createContact(body)) {
                qWarning() << "Error creating contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
#else
            sendProxyResponse(uid, 404);
#endif
            return;
        }

        if (api == "update-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendProxyResponse(uid, 400);
                return;
            }

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

#ifdef CONTACTS
            if (!updateContact(data.toInt(), body)) {
                qWarning() << "Error updating contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
#else
            sendProxyResponse(uid, 404);
#endif
            return;
        }

        if (api == "delete-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendProxyResponse(uid,400);
                return;
            }

#ifdef CONTACTS
            if (!deleteContact(data.toInt())) {
                qWarning() << "Error deleting contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);
#else
            sendProxyResponse(uid, 404);
#endif
            return;
        }

    }

    qWarning() << "Unknown request!";
    sendProxyResponse(uid,404);
}

void Server::bodyReceived()
{
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());
    QHttpResponse* resp = respMap.take(req);

    //qDebug() << "bodyReceived, url: " << req->url().toString();

    QUrlQuery query(req->url());
    if (query.hasQueryItem("app")) {
        handleLocalServerNewApi(req, resp);
    } else {
        handleLocalServerOldApi(req, resp);
    }

}

QString Server::getContentType(const QString & file)
{
    QStringList l = file.split(".");

    QString ext = "";
    if (l.length()>0)
        ext = l[l.length()-1];

    if (ext.isEmpty())
        return "";

    return ext == "css" ? "text/css; charset=utf-8" :
                          ext == "gif" ? "image/gif" :
                          ext == "png" ? "image/png" :
                          ext == "js" ? "application/x-javascript; charset=utf-8" :
                                        "text/html; charset=utf-8";
}

void Server::handleLocalServerNewApi(QHttpRequest *req, QHttpResponse *resp)
{
    Settings* s = Settings::instance();

    QString source = req->remoteAddress();
    QUrlQuery query(req->url());

    if (query.hasQueryItem("api")) {

        QString api = query.queryItemValue("api");
        QString data = query.queryItemValue("data");
        QByteArray body = req->body();

        if (api == "open-url") {

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

            launchBrowser(data);
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp);
            return;
        }

        if (api == "options") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getOptions());
            return;
        }

        if (api == "web-client") {

            QByteArray body;

            data = data.isEmpty() ? "index.html" : data;
            if (getWebContent(data, body)) {
                QString contentType = getContentType(data);
                if (contentType.isEmpty()) {
                    qWarning() << "Unknown content type!";
                    sendResponse(req, resp, 404);
                } else {
                    sendResponse(req, resp, 200, contentType, body);
                }
                return;
            }

            sendResponse(req, resp, 404);
            return;
        }

        if (api == "get-bookmarks") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getBookmarks(), s->getCrypt());
            return;
        }

        if (api == "get-notes-list") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getNotes(), s->getCrypt());
            return;
        }

        if (api == "get-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getNote(data.toInt()), s->getCrypt());
            return;
        }

        if (api == "get-clipboard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "text/plain; charset=utf-8", getClipboard().toUtf8(), s->getCrypt());
            return;
        }

        if (api == "delete-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            if (deleteNote(data.toInt()))
                sendResponse(req, resp);
            else
                sendResponse(req, resp, 404);
            return;
        }

        if (api == "delete-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            if (deleteBookmark(data.toInt()))
                sendResponse(req, resp);
            else
                sendResponse(req, resp, 404);
            return;
        }

        if (api == "update-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!updateNote(data.toInt(), body)) {
                qWarning() << "Error updatig note with id:" << data.toInt();
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
            return;
        }

        if (api == "update-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!updateBookmark(data.toInt(), body)) {
                qWarning() << "Error updatig bookmark with id:" << data.toInt();
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
            return;
        }

        if (api == "create-note") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!createNote(body)) {
                qWarning() << "Error creating note!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
            return;
        }

        if (api == "create-bookmark") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!createBookmark(body)) {
                qWarning() << "Error deleting bookmark!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
            return;
        }

        if (api == "set-clipboard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
            }

            setClipboard(QString(body));
            sendResponse(req, resp);
            return;
        }

        if (api == "get-contacts") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
#ifdef CONTACTS
            sendResponse(req, resp, 200, "application/json", getContacts(data), s->getCrypt());
#else
            sendResponse(req, resp, 404);
#endif
            return;
        }

        if (api == "get-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

#ifdef CONTACTS
            QByteArray json = getContact(data.toInt());
            if (json.isEmpty())
                sendResponse(req, resp, 404);
            else
                sendResponse(req, resp, 200, "application/json", json, s->getCrypt());
#else
            sendResponse(req, resp, 404);
#endif
            return;
        }

        if (api == "create-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

#ifdef CONTACTS
            if (!createContact(body)) {
                qWarning() << "Error creating contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
#else
            sendResponse(req, resp, 404);
#endif
            return;
        }

        if (api == "update-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

#ifdef CONTACTS
            if (!updateContact(data.toInt(), body)) {
                qWarning() << "Error updating contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
#else
            sendResponse(req, resp, 404);
#endif
            return;
        }

        if (api == "delete-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

#ifdef CONTACTS
            if (!deleteContact(data.toInt())) {
                qWarning() << "Error deleting contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);
#else
            sendResponse(req, resp, 404);
#endif
            return;
        }

    }

    qWarning() << "Unknown request!";
    sendResponse(req, resp, 404);
}

void Server::handleLocalServerOldApi(QHttpRequest *req, QHttpResponse *resp)
{
    QStringList path = req->url().path().split("/");

    Settings* s = Settings::instance();

    //qDebug() << "handleLocalServerOldApi, path:" << req->url().path();

    if (path.length() > 1 && path.at(1) == s->getCookie()) {

        // Redirection server_url/cookie -> server_url/cookie/
        if (path.length() == 2) {
            resp->setHeader("Location", s->getCookie() + "/");
            sendResponse(req, resp, 302);
            return;
        }

        if (path.length() > 2 && path.at(2) == "get-platform") {
#ifdef SAILFISH
            sendResponse(req, resp, 200, "text/plain; charset=utf-8", "sailfish");
#elif BB10
            sendResponse(req, resp, 200, "text/plain; charset=utf-8", "blackberry");
#else
            sendResponse(req, resp, 200, "text/plain; charset=utf-8", "unknown");
#endif
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 2 && path.at(2) == "get-clipboard") {
            sendResponse(req, resp, 200, "text/plain; charset=utf-8", getClipboard().toUtf8());
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 2 && path.at(2) == "get-notes-list") {
            sendResponse(req, resp, 200, "application/json; charset=utf-8", getNotes());
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "get-note") {
            sendResponse(req, resp, 200, "application/json; charset=utf-8", getNote(path.at(3).toInt()));
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "delete-note") {
            if (deleteNote(path.at(3).toInt()))
                sendResponse(req, resp);
            else
                sendResponse(req, resp, 404);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "update-note" && req->method() == QHttpRequest::HTTP_POST) {
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            if (!updateNote(path.at(3).toInt(),req->body())) {
                qWarning() << "Error updatig note with id:" << path.at(3).toInt();
                sendResponse(req, resp, 400);
                return;
            }
            sendResponse(req, resp);
            return;
        }

        if (path.length() > 2 && path.at(2) == "create-note" && req->method() == QHttpRequest::HTTP_POST) {
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            if (!createNote(req->body())) {
                qWarning() << "Error creating new note";
                sendResponse(req, resp, 400);
                return;
            }
            sendResponse(req, resp);
            return;
        }

        if (path.length() > 2 && path.at(2)=="create-bookmark" && req->method()==QHttpRequest::HTTP_POST) {
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            if (!createBookmark(req->body())) {
                qWarning() << "Error creating new bookmark";
                sendResponse(req, resp, 400);
                return;
            }
            sendResponse(req, resp);
            return;
        }

        if (path.length() > 2 && path.at(2) == "get-bookmarks") {
            sendResponse(req, resp, 200, "application/json; charset=utf-8", getBookmarks());
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "delete-bookmark") {
            if (deleteBookmark(path.at(3).toInt()))
                sendResponse(req, resp);
            else
                sendResponse(req, resp, 404);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "update-bookmark" && req->method()==QHttpRequest::HTTP_POST) {
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            if (!updateBookmark(path.at(3).toInt(), req->body())) {
                qWarning() << "Error updating bookmark";
                sendResponse(req, resp, 400);
                return;
            }
            sendResponse(req, resp);
            return;
        }

        if (path.length() > 3 && path.at(2) == "open-url") {
            launchBrowser(path.at(3));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 2 && path.at(2) == "set-clipboard" && req->method() == QHttpRequest::HTTP_POST) {
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            setClipboard(QString(req->body()));
            sendResponse(req, resp);
            return;
        }

        // Web client

        if (path.length() == 3) {

            // Redirection to new API
            //qDebug() << "Location:" << QString("/?app=%1&api=web-client").arg(s->getCookie());
            resp->setHeader("Location", QString("/?app=%1&api=web-client").arg(s->getCookie()));
            sendResponse(req, resp, 302);
            return;

        }

        sendResponse(req, resp, 400);
        emit newEvent(tr("Unknown request from %1").arg(req->remoteAddress()));
        return;
    }

    sendResponse(req, resp, 400);
    emit newEvent(tr("Unknown request from %1").arg(req->remoteAddress()));
}

void Server::handleLocalServer(QHttpRequest *req, QHttpResponse *resp)
{
    //qDebug() << "handleLocalServer, url:" << req->url().toString();
    respMap.insert(req, resp);
    connect(req, SIGNAL(end()), this, SLOT(bodyReceived()));
    req->storeBody();
}

void Server::sendProxyResponse(const QString &uid, int status, const QString &contentType, const QByteArray &body, bool encrypt)
{
    QJsonObject newObj;
    newObj.insert("type", QString("http_response"));
    newObj.insert("uid", uid);
    newObj.insert("status", status);

    if (status != 204) {
        if (encrypt) {
            newObj.insert("content_type", QString("application/json"));
            newObj.insert("body", QLatin1String(this->encrypt(body).toBase64()));
        } else {
            newObj.insert("body", QLatin1String(body.toBase64()));
            newObj.insert("content_type", contentType);
        }
    }

    QJsonDocument doc(newObj);
    if (!proxy.sendData(uid, doc.toJson())) {
        qWarning() << "Sending response via proxy failed!";
    }

    //qDebug() << "Sending HTTP response:" << doc.toJson();
}

void Server::sendResponse(QHttpRequest *req, QHttpResponse *resp, int status, const QString &contentType, const QByteArray &body, bool encrypt)
{
    Q_UNUSED(req)

    QByteArray nbody = body;

    if (status == 204) {
        resp->setHeader("Content-Length", "0");
    } else {
        if (encrypt) {
            nbody = this->encrypt(body);
            resp->setHeader("Content-Type", "application/json");
        } else {
            resp->setHeader("Content-Type", contentType);
        }
        resp->setHeader("Content-Length", QString::number(nbody.size()));
    }
    resp->setHeader("Connection", "close");
    resp->writeHead(status);
    resp->end(nbody);
}

QString Server::getLocalServerUrl()
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

bool Server::getWebContent(const QString &filename, QByteArray &data)
{
#ifdef SAILFISH
    QFile file(SailfishApp::pathTo("webclient/" + (filename == "" ? "index.html" : filename)).toLocalFile());
#elif BB10
    QFile file(QDir::currentPath()+"/app/native/webclient/" + (filename == "" ? "index.html" : filename));
#else
    return false;
#endif

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << filename << "for reading: " << file.errorString();
        file.close();
        return false;
    }

    data = file.readAll();
    file.close();

    return true;
}

QByteArray Server::getNotes()
{
#ifdef SAILFISH
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return "";
    }

    QJsonArray array;

    QSqlQuery query(notesDB);
    if(query.exec("SELECT pagenr, color, body FROM notes ORDER BY pagenr")) {
        while(query.next()) {

            QString text = query.value(2).toString();
            text = text.length() <= 50 ? text : text.left(50);

            QJsonObject obj;
            obj.insert("id",query.value(0).toInt());
            obj.insert("color",query.value(1).toString());
            obj.insert("title",text);
            array.append(obj);

        }
    } else {
        qWarning() << "SQL execution error:" << query.lastError().text();
        return "";
    }

    return QJsonDocument(array).toJson(QJsonDocument::Compact);
#elif BB10
    NotebookService service;
    Notebook notebook = service.defaultNotesNotebook();
    NotebookEntryFilter filter;
    filter.setParentNotebookId(notebook.id());

    QList<NotebookEntry> list = service.notebookEntries(filter);

    if (list.isEmpty()) {
        return "";
    }

    QVariantList jsonList;
    QList<NotebookEntry>::iterator it = list.begin();
    int i = 1;
    while(it != list.end()) {
        QVariantMap map;
        map.insert("id", i);
        QString title = (*it).title();
        title = title.length() <= 50 ? title : title.left(50);
        map.insert("title", title);
        jsonList.append(map);
        ++it; ++i;
    }

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(jsonList, &ok);

    if (!ok) {
      qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
      return "";
    }

    return json;
#else
    return "";
#endif
}

QByteArray Server::getNote(int id)
{
#ifdef SAILFISH
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return "";
    }

    QSqlQuery query(notesDB);
    if(query.exec(QString("SELECT color, body FROM notes WHERE pagenr = %1").arg(id))) {
        while(query.next()) {

            QJsonObject obj;
            obj.insert("id",QJsonValue(id));
            obj.insert("color",query.value(0).toString());
            obj.insert("body",query.value(1).toString());
            return QJsonDocument(obj).toJson(QJsonDocument::Compact);

        }
    } else {
        qWarning() << "SQL execution error:" << query.lastError().text();
    }
#elif BB10
    NotebookService service;
    Notebook notebook = service.defaultNotesNotebook();
    NotebookEntryFilter filter;
    filter.setParentNotebookId(notebook.id());

    QList<NotebookEntry> list = service.notebookEntries(filter);

    if (list.isEmpty()) {
        qWarning() << "List of notes is empty!";
        return "";
    }

    if (list.length() < id || id < 1) {
        qWarning() << "Note ID is invalid!";
        return "";
    }

    NotebookEntry entry = list[id-1];
    QVariantMap map;
    map.insert("id", id);
    map.insert("title", entry.title());
    map.insert("body", entry.description().plainText());

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(map, &ok);

    if (!ok) {
        qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
        return "";
    }

    return json;
#else
    return "";
#endif
    return "";
}

bool Server::createBookmark(const QByteArray &json)
{
#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse bookmark data!" << parseError.errorString();
        qDebug() << json;
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Bookmark data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    if (!obj["url"].isString() || obj["url"].toString().isEmpty() ||
        !obj["title"].isString() || obj["title"].toString().isEmpty()) {
        qWarning() << "Bookmark data is not valid!";
        return false;
    }

    QJsonObject newObj;
    QString icon = obj["icon"].isString() && !obj["icon"].toString().isEmpty() ? obj["icon"].toString() : "icon-launcher-bookmark";
    newObj.insert("favicon", icon);
    newObj.insert("url", obj["url"]);
    newObj.insert("title", obj["title"]);
    newObj.insert("hasTouchIcon", QJsonValue::fromVariant(false));

    QJsonArray newArray = readBookmarks();
    newArray.append(newObj);

    if (!writeBookmarks(newArray)) {
        qWarning() << "Can not create new bookmark!";
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool Server::updateBookmark(int id, const QByteArray &json)
{
#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse bookmark data!" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Bookmark data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    QJsonArray newArray = readBookmarks();

    if (id < 1 || newArray.size() < id) {
        qWarning() << "Unknown bookmark ID!";
        return false;
    }

    QJsonObject oldObj = newArray.at(id-1).toObject();
    if (obj["icon"].isString() && !obj["icon"].toString().isEmpty())
        oldObj["favicon"] = obj["icon"];
    if (obj["title"].isString() && !obj["title"].toString().isEmpty())
        oldObj["title"] = obj["title"];
    if (obj["url"].isString() && !obj["url"].toString().isEmpty())
        oldObj["url"] = obj["url"];
    newArray.replace(id-1, oldObj);

    if (!writeBookmarks(newArray)) {
        qWarning() << "Can not create new bookmark!";
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool Server::deleteBookmark(int id)
{
#ifdef SAILFISH
    QJsonArray newArray = readBookmarks();

    if (id < 1 || newArray.size() < id) {
        qWarning() << "Unknown bookmark ID!";
        return false;
    }

    newArray.removeAt(id - 1);

    if (!writeBookmarks(newArray)) {
        qWarning() << "Can not create new bookmark!";
        return false;
    }

    return true;
#else
    return false;
#endif
}

QByteArray Server::getBookmarks()
{
#ifdef SAILFISH
    QJsonArray newArr;
    QJsonArray arr = readBookmarks();
    QJsonArray::iterator it = arr.begin();
    int i = 1;
    while (it != arr.end()) {
        if ((*it).isObject()) {
            QJsonObject obj = (*it).toObject();
            QJsonObject newObj;
            newObj.insert("id",QJsonValue(i));
            newObj.insert("title",obj["title"]);
            newObj.insert("url",obj["url"]);

            QString icon = obj["favicon"].toString();
            if (icon.startsWith("http://",Qt::CaseInsensitive) ||
                icon.startsWith("https://",Qt::CaseInsensitive))
                newObj.insert("icon",obj["favicon"]);
            /*else
                newObj.insert("icon",QJsonValue(QJsonValue::String));*/

            newArr.append(newObj);
            ++i;
            ++it;
        }
    }

    if (newArr.isEmpty()) {
        qWarning() << "No bookmarks!";
        return "";
    }

    return QJsonDocument(newArr).toJson(QJsonDocument::Compact);
#else
    return "";
#endif
}

#ifdef SAILFISH
bool Server::writeBookmarks(const QJsonArray &array)
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
            "/org.sailfishos/sailfish-browser/bookmarks.json");

    if (!file.exists()) {
        qWarning() << "Bookmarks file doesn't exist!" << file.fileName();
        return false;
    }

    /*if (fileBackup.exists()) {
        qWarning() << "Bookmarks backup exist!";
        if (!fileBackup.remove()) {
            qWarning() << "Bookmarks backup can not be removed!";
        }
    }

    // Backup
    if (!file.copy(fileBackup.fileName())) {
        qWarning() << "Can not create bookmarks backup!";
        return false;
    }*/

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can not open bookmarks file!" << file.errorString();
        return false;
    }

    QTextStream outStream(&file);
    QJsonDocument doc(array);
    outStream.setCodec("UTF-8");
    outStream.setGenerateByteOrderMark(true);
    outStream << doc.toJson();

    file.close();

    return true;
}

QJsonArray Server::readBookmarks()
{
    QJsonArray empty;
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
            "/org.sailfishos/sailfish-browser/bookmarks.json");

    if (!file.exists()) {
        qWarning() << "Bookmarks file doesn't exist!" << file.fileName();
        return empty;
    }

    if(!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Can not open bookmarks file!" << file.errorString();
        return empty;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(),&parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse bookmarks file!" << parseError.errorString();
        return empty;
    }

    if (!doc.isArray()) {
        qWarning() << "Bookmarks file is not JSON array!";
        return empty;
    }

    return doc.array();
}

QString Server::getNotesDBfile()
{
    QString dirName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
            "/jolla-notes/QML/OfflineStorage/Databases";

    QDir dir(dirName);
    if (dir.exists()) {
        QFileInfoList infoList = dir.entryInfoList(QStringList("*.ini"), QDir::Files | QDir::Readable);
        Q_FOREACH(QFileInfo info, infoList) {
            QSettings ini(info.filePath(), QSettings::IniFormat);
            if (ini.value("Description").toString() == "Notes") {
                QString file = info.filePath(); file.chop(3); file += "sqlite";
                return file;
            }
        }
    }

    return "";
}

bool Server::openNotesDB()
{
    QString dbFile = getNotesDBfile();

    if (dbFile == "") {
        qWarning() << "Can not find notes DB file!";
        return false;
    }

    notesDB = QSqlDatabase::addDatabase("QSQLITE","qt_sql_sendtojolla_connection");
    notesDB.setDatabaseName(dbFile);

    if (!notesDB.open()) {
        qWarning() << "Can not open notes DB file:" << dbFile;
        QSqlDatabase::removeDatabase("qt_sql_sendtojolla_connection");
        return false;
    }

    return true;
}

void Server::closeNotesDB()
{
    notesDB.close();
    QSqlDatabase::removeDatabase("qt_sql_sendtojolla_connection");
}
#endif

bool Server::createNote(const QByteArray &json)
{
#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse note data!" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Note data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    if (!obj["color"].isString() || obj["color"].toString().isEmpty() ||
        !obj["body"].isString() || obj["body"].toString().isEmpty()) {
        qWarning() << "Note data is not valid!";
        return false;
    }

    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return false;
    }

    if (!notesDB.transaction()) {
        qWarning() << "DB transaction failed!" << notesDB.lastError().text();
        return false;
    }

    QSqlQuery query1(notesDB);
    query1.prepare("UPDATE notes SET pagenr = pagenr + 1 WHERE pagenr >= 1");
    QSqlQuery query2(notesDB);
    query2.prepare("INSERT INTO notes (pagenr, color, body) VALUES (1, ?, ?)");
    query2.addBindValue(obj["color"].toString());
    query2.addBindValue(obj["body"].toString());

    if(!query1.exec()) {
        qWarning() << "SQL execution error:" << query1.lastError().text();
        notesDB.rollback();
        return false;
    }

    if(!query2.exec()) {
        qWarning() << "SQL execution error:" << query2.lastError().text();
        notesDB.rollback();
        return false;
    }

    if (!notesDB.commit()) {
        qWarning() << "DB transaction failed to commit!" << notesDB.lastError().text();
        notesDB.rollback();
        return false;
    }

    return true;
#elif BB10
    QJson::Parser parser;
    bool ok;
    QVariant result = parser.parse(json, &ok);

    if (!ok) {
        qWarning() << "An error occurred during parsing JSON!";
        return false;
    }

    if (result.type() != QVariant::Map) {
        qWarning() << "JSON is not an object!";
        return false;
    }

    QVariantMap obj = result.toMap();

    if (!obj["title"].type() == QVariant::String ||
         obj["title"].toString().isEmpty()) {
            qWarning() << "Note data is not valid!";
            return false;
    }

    NotebookEntry entry;
    entry.setTitle(obj["title"].toString());
    NotebookEntryDescription desc;
    desc.setText(obj["body"].toString());
    entry.setDescription(desc);

    NotebookService service;
    Notebook notebook = service.defaultNotesNotebook();
    service.addNotebookEntry(&entry, notebook.id());

    return true;
#else
    return false;
#endif
}

bool Server::updateNote(int id, const QByteArray &json)
{
#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse note data!" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Note data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    if (!obj["color"].isString() || obj["color"].toString().isEmpty() ||
        !obj["body"].isString() || obj["body"].toString().isEmpty()) {
        qWarning() << "Note data is not valid!";
        return false;
    }

    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return false;
    }

    QSqlQuery query(notesDB);
    query.prepare("UPDATE notes SET body = ?, color = ? WHERE pagenr = ?");
    query.addBindValue(obj["body"].toString());
    query.addBindValue(obj["color"].toString());
    query.addBindValue(id);

    if(!query.exec()) {
        qWarning() << "SQL execution error:" << query.lastError().text();
        return false;
    }

    return true;
#elif BB10
    NotebookService service;
    Notebook notebook = service.defaultNotesNotebook();
    NotebookEntryFilter filter;
    filter.setParentNotebookId(notebook.id());

    QList<NotebookEntry> list = service.notebookEntries(filter);

    if (list.isEmpty()) {
        qWarning() << "List of notes is empty!";
        return false;
    }

    if (list.length() < id || id < 1) {
        qWarning() << "Note ID is invalid!";
        return false;
    }

    QJson::Parser parser;
    bool ok;
    QVariant result = parser.parse(json, &ok);

    if (!ok) {
        qWarning() << "An error occurred during parsing JSON!";
        return false;
    }

    if (result.type() != QVariant::Map) {
        qWarning() << "JSON is not an object!";
        return false;
    }

    QVariantMap obj = result.toMap();

    if (!obj["title"].type() == QVariant::String ||
         obj["title"].toString().isEmpty()) {
        qWarning() << "Note data is not valid!";
        return false;
    }

    NotebookEntry entry = list[id-1];
    entry.setTitle(obj["title"].toString());
    NotebookEntryDescription desc;
    desc.setText(obj["body"].toString());
    entry.setDescription(desc);

    if (service.updateNotebookEntry(entry) != NotebookServiceResult::Success) {
        qWarning() << "Note updating error!";
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool Server::deleteNote(int id)
{
#ifdef SAILFISH
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return false;
    }

    if (!notesDB.transaction()) {
        qWarning() << "DB transaction failed!" << notesDB.lastError().text();
        return false;
    }

    QSqlQuery query1(notesDB);
    query1.prepare("DELETE FROM notes WHERE pagenr = ?");
    query1.addBindValue(id);
    QSqlQuery query2(notesDB);
    query2.prepare("UPDATE notes SET pagenr = pagenr - 1 WHERE pagenr > ?");
    query2.addBindValue(id);

    if(!query1.exec()) {
        qWarning() << "SQL execution error:" << query1.lastError().text();
        notesDB.rollback();
        return false;
    }

    if(!query2.exec()) {
        qWarning() << "SQL execution error:" << query2.lastError().text();
        notesDB.rollback();
        return false;
    }

    if (!notesDB.commit()) {
        qWarning() << "DB transaction failed to commit!" << notesDB.lastError().text();
        notesDB.rollback();
        return false;
    }

    return true;
#elif BB10
    NotebookService service;
    Notebook notebook = service.defaultNotesNotebook();
    NotebookEntryFilter filter;
    filter.setParentNotebookId(notebook.id());

    QList<NotebookEntry> list = service.notebookEntries(filter);

    if (list.isEmpty()) {
        qWarning() << "List of notes is empty!";
        return false;
    }

    if (list.length() < id || id < 1) {
        qWarning() << "Note ID is invalid!";
        return false;
    }

    NotebookEntryId entryId = list[id-1].id();
    if (service.deleteNotebookEntry(entryId) != NotebookServiceResult::Success) {
        qWarning() << "Note deletion error!";
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool Server::isRunning()
{
    return isListening;
}

bool Server::isProxyOpen()
{
    return proxy.isOpen();
}

void Server::proxyDataReceived(QByteArray data)
{
    //qDebug() << "Data:" << data;

#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse data!" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "Data is not JSON object!";
        return;
    }

    QJsonObject obj = doc.object();

    if (!obj["type"].isString() || obj["type"].toString().isEmpty()) {
        qWarning() << "Type param is missing!";
        return;
    }

    if (obj["type"].toString() == "http_request") {
        handleProxy(obj);
        return;
    }
#endif
}

void Server::proxyOpenHandler()
{
    qDebug() << "Proxy open status:" << proxy.isOpen();

    emit proxyOpenChanged();

    if (proxy.isOpen()) {
        emit newEvent(tr("Connected to proxy"));
    } else {
        emit newEvent(tr("Disconnected from proxy"));
        if (autoStartProxy)
            connectProxy();
    }

    autoStartProxy = false;
}

void Server::connectProxy()
{
    Settings* s = Settings::instance();
    proxy.open(s->getCookie(), QUrl(s->getProxyUrl()+"?int=in"));
}

void Server::disconnectProxy()
{
    proxy.close();
}

void Server::sendDataToProxy(const QString &data)
{
    proxy.sendData("", data.toUtf8());
}

void Server::proxyErrorHandler(int code)
{
    qDebug() << "Proxy error, code:" << code;

    switch(code) {
    case 6:
        emit newEvent(tr("Can't connect to proxy due to SSL error. If your proxy server has a self-signed certificate, consider enabling 'Ignore SSL errors' option"));
        break;
    default:
        emit newEvent(tr("Can't connect to proxy. Check if 'Proxy URL' is valid"));
    }
}

void Server::proxyBusyHandler()
{
    emit proxyBusyChanged();
}

bool Server::isProxyBusy()
{
    return proxy.isClosing() || proxy.isOpening();
}

QString Server::getProxyUrl()
{
    Settings* s = Settings::instance();
    return s->getProxyUrl();
}

QString Server::getWebClientProxyUrl()
{
    Settings* s = Settings::instance();
    return s->getProxyUrl() + "?int=out&app=" + s->getCookie() + "&api=web-client";
}

void Server::cookieChangedHandler()
{
    if (proxy.isOpen()) {
        autoStartProxy = true;
        proxy.close();
    }
}

QByteArray Server::getContacts(const QString &filter)
{
#ifdef SAILFISH
#ifdef CONTACTS
    QJsonArray array;

    QContactSortOrder sort; sort.setDirection(Qt::DescendingOrder);
    QList<QContactSortOrder> sortList; sortList.push_front(sort);
    QList<QContact> results = filter.isEmpty() ?
                cm->contacts(sortList) :
                cm->contacts(QContactDisplayLabel::match(filter), sortList);

    //qDebug() << "Contacts list size:" << results.size();

    for (QList<QContact>::iterator it = results.begin(); it != results.end() && array.size() < 500; ++it) {
        int id = (*it).id().toString().split("-").last().toInt();

        QContactDisplayLabel label = (*it).detail<QContactDisplayLabel>();
        QContactName name = (*it).detail<QContactName>();

        QJsonObject obj;
        obj.insert("id",id);
        if (!name.firstName().isEmpty() || !name.lastName().isEmpty()) {
            obj.insert("label", label.label());
            array.append(obj);
        }
    }

    return QJsonDocument(array).toJson(QJsonDocument::Compact);
#else
    return "";
#endif
#else
    return "";
#endif
}

QByteArray Server::getContact(int id)
{
#ifdef SAILFISH
#ifdef CONTACTS

    QContact contact = cm->contact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)));

    if (contact.isEmpty()) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return "";
    }

    QContactDisplayLabel label = contact.detail<QContactDisplayLabel>();
    QContactName name = contact.detail<QContactName>();
    QContactAvatar avatar = contact.detail<QContactAvatar>();

    /*QContactAddress address = contact.detail<QContactAddress>();
    QContactOrganization org = contact.detail<QContactOrganization>();
    QContactNote note = contact.detail<QContactNote>();

    qDebug() << "contact.type" << contact.type();
    QStringList list = contact.tags();
    for (QStringList::iterator it = list.begin(); it != list.end(); ++it) {
        qDebug() << "contact.tag" << *it;
    }
    QMap<QString, QContactDetail> map = contact.preferredDetails();
    for (QMap<QString, QContactDetail>::iterator it = map.begin(); it != map.end(); ++it) {
        qDebug() << "contact.preferredDetails" << it.key();
    }
    QList<QContactActionDescriptor> al = contact.availableActions();
    for (QList<QContactActionDescriptor>::iterator it = al.begin(); it != al.end(); ++it) {
        qDebug() << "contact.availableActions" << (*it).actionName();
    }*/

    QJsonObject obj;
    obj.insert("id",id);
    if (!label.label().isEmpty())
        obj.insert("label", label.label());
    if (!name.firstName().isEmpty())
        obj.insert("first_name", name.firstName());
    if (!name.lastName().isEmpty())
        obj.insert("last_name", name.lastName());
    if (!avatar.imageUrl().isEmpty()) {
        QFile file(avatar.imageUrl().toLocalFile());
        if (file.exists()) {
            if(file.open(QIODevice::ReadOnly)) {
                QByteArray avatarData = "data:image/jpg;base64," + file.readAll().toBase64();
                obj.insert("avatar", QString(avatarData));
            } else {
                qWarning() << "Can not open avatar file!" << file.errorString();
            }
        } else {
            qWarning() << "Avatar file doesn't exist!" << file.fileName();
        }
    }
    /*if (!address.country().isEmpty())
        obj.insert("address_country", address.country());
    if (!address.locality().isEmpty())
        obj.insert("address_locality", address.locality());
    if (!address.street().isEmpty())
        obj.insert("address_street", address.street());
    if (!org.name().isEmpty())
        obj.insert("organization_name", org.name());
    if (!org.title().isEmpty())
        obj.insert("organization_title", org.title());
    if (!note.note().isEmpty())
        obj.insert("note", note.note());*/

    QList<QContactPhoneNumber> numbers = contact.details<QContactPhoneNumber>();
    if (!numbers.isEmpty()) {
        QJsonArray array;
        for (QList<QContactPhoneNumber>::iterator it = numbers.begin(); it != numbers.end(); ++it) {
            QJsonObject obj; obj.insert("number", (*it).number());
            QList<int> types = (*it).subTypes();
            for (QList<int>::iterator it = types.begin(); it != types.end(); ++it) {
                if ((*it) == QContactPhoneNumber::SubTypeMobile) {
                    obj.insert("type", QString("mobile"));
                    break;
                }
                if ((*it) == QContactPhoneNumber::SubTypeAssistant) {
                    obj.insert("type", QString("assistant"));
                    break;
                }
                if ((*it) == QContactPhoneNumber::SubTypeFax) {
                    obj.insert("type", QString("fax"));
                    break;
                }
                if ((*it) == QContactPhoneNumber::SubTypeVideo) {
                    obj.insert("type", QString("video"));
                    break;
                }
                if ((*it) == QContactPhoneNumber::SubTypePager) {
                    obj.insert("type", QString("pager"));
                    break;
                }
            }
            QList<int> contexts = (*it).contexts();
            for (QList<int>::iterator it = contexts.begin(); it != contexts.end(); ++it) {
                switch (*it) {
                case QContactDetail::ContextHome:
                    obj.insert("context", QString("home"));
                    break;
                case QContactDetail::ContextWork:
                    obj.insert("context", QString("work"));
                    break;
                case QContactDetail::ContextOther:
                    obj.insert("context", QString("other"));
                    break;
                }
            }
            array.append(obj);
        }
        obj.insert("phones", array);
    }

    QList<QContactEmailAddress> emails = contact.details<QContactEmailAddress>();
    if (!emails.isEmpty()) {
        QJsonArray array;
        for (QList<QContactEmailAddress>::iterator it = emails.begin(); it != emails.end(); ++it) {
            QJsonObject obj; obj.insert("address", (*it).emailAddress());
            QList<int> contexts = (*it).contexts();
            for (QList<int>::iterator it = contexts.begin(); it != contexts.end(); ++it) {
                switch (*it) {
                case QContactDetail::ContextHome:
                    obj.insert("context", QString("home"));
                    break;
                case QContactDetail::ContextWork:
                    obj.insert("context", QString("work"));
                    break;
                case QContactDetail::ContextOther:
                    obj.insert("context", QString("other"));
                    break;
                }
            }
            array.append(obj);
        }
        obj.insert("emails", array);
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);

    /*for (QList<QContact>::iterator it = results.begin(); it != results.end() && array.size() < 500; ++it) {
        int id = (*it).id().toString().split("-").last().toInt();
        //QContactName name = (*it).detail<QContactName>();
        QContactDisplayLabel label = (*it).detail<QContactDisplayLabel>();
        //QContactAddress address = (*it).detail<QContactAddress>();
        //QContactBirthday birthday = (*it).detail<QContactBirthday>();
        //QContactOrganization org = (*it).detail<QContactOrganization>();
        //QContactEmailAddress email = (*it).detail<QContactEmailAddress>();
        //QContactNote note = (*it).detail<QContactNote>();

        qDebug() << "*** contact ***";
        qDebug() << "Id:" << id;
        qDebug() << "Name:" << name.firstName() << name.lastName();
        qDebug() << "Label:" << label.label();
        qDebug() << "Address:" << address.country() << address.locality() << address.postcode() << address.street();
        qDebug() << "Birthday:" << birthday.date().toString();
        qDebug() << "Organization:" << org.name() << org.title();
        qDebug() << "Number:" << number.number();
        qDebug() << "Email:" << email.emailAddress();
        qDebug() << "Note:" << note.note();

        QJsonObject obj;
        obj.insert("id",id);
        if (!label.label().isEmpty()) {
            obj.insert("label", label.label());
            array.append(obj);
        }

        if (!name.firstName().isEmpty())
            obj.insert("first_name", name.firstName());
        if (!name.lastName().isEmpty())
            obj.insert("last_name", name.lastName());
        if (!address.country().isEmpty())
            obj.insert("address_country", address.country());
        if (!address.locality().isEmpty())
            obj.insert("address_locality", address.locality());
        if (!address.street().isEmpty())
            obj.insert("address_street", address.street());
        if (!org.name().isEmpty())
            obj.insert("organization_name", org.name());
        if (!note.note().isEmpty())
            obj.insert("organization_name", note.note());

        QList<QContactPhoneNumber> numbers = (*it).details<QContactPhoneNumber>();
        for (QList<QContactPhoneNumber>::iterator it = numbers.begin(); it != numbers.end(); ++it) {
            obj.insert("number", (*it).number());
        }

        QList<QContactEmailAddress> emails = (*it).details<QContactEmailAddress>();
        for (QList<QContactEmailAddress>::iterator it = emails.begin(); it != emails.end(); ++it) {
            obj.insert("email", (*it).emailAddress());
        }
    }*/
#else
    return "";
#endif
#else
    return "";
#endif
}

bool Server::createContact(const QByteArray &json)
{
#ifdef SAILFISH
#ifdef CONTACTS

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse cantact data!" << parseError.errorString();
        qDebug() << json;
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Contact data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    if ((!obj["first_name"].isString() || obj["first_name"].toString().isEmpty()) &&
        (!obj["last_name"].isString() || obj["last_name"].toString().isEmpty())) {
        qWarning() << "Contact data is not valid!";
        return false;
    }

    QString firstName = obj["first_name"].toString();
    QString lastName = obj["last_name"].toString();

    QContactDisplayLabel label;
    label.setLabel(firstName.isEmpty() ? lastName : lastName.isEmpty() ? firstName : firstName + " " + lastName);

    QContactName name;
    name.setFirstName(firstName);
    name.setLastName(lastName);

    QContact contact;
    contact.saveDetail(&label);
    contact.saveDetail(&name);

    if (obj.contains("note") && obj["note"].isString() &&
            !obj["note"].toString().isEmpty()) {
        QContactNote note;
        note.setNote(obj["note"].toString());
        contact.saveDetail(&note);
    }

    if (obj["phones"].isArray()) {
        QJsonArray arr = obj["phones"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("number") && obj["number"].isString() &&
                       !obj["number"].toString().isEmpty()) {

                   QContactPhoneNumber number;
                   number.setNumber(obj["number"].toString());

                   if (obj.contains("context") && obj["context"].isString() &&
                           !obj["context"].toString().isEmpty()) {
                       QString context = obj["context"].toString();
                       if (context == "work")
                           number.setContexts(QContactDetail::ContextWork);
                       if (context == "home")
                           number.setContexts(QContactDetail::ContextHome);
                       if (context == "other")
                           number.setContexts(QContactDetail::ContextOther);
                   }

                   if (obj.contains("type") && obj["type"].isString() &&
                           !obj["type"].toString().isEmpty()) {
                       QString type = obj["type"].toString();
                       if (type == "mobile") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeMobile);
                           number.setSubTypes(types);
                       } else if (type == "assistant") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeAssistant);
                           number.setSubTypes(types);
                       } else if (type == "pager") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypePager);
                           number.setSubTypes(types);
                       } else if (type == "video") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeVideo);
                           number.setSubTypes(types);
                       } else if (type == "fax") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeFax);
                           number.setSubTypes(types);
                       }
                   }

                   contact.saveDetail(&number);
               } else {
                   qWarning() << "Phone without number!";
               }
           }
        }
    }

    if (obj["emails"].isArray()) {
        QJsonArray arr = obj["emails"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("address") && obj["address"].isString() &&
                       !obj["address"].toString().isEmpty()) {

                   QContactEmailAddress email;
                   email.setEmailAddress(obj["address"].toString());

                   if (obj.contains("context") && obj["context"].isString() &&
                           !obj["context"].toString().isEmpty()) {
                       QString context = obj["context"].toString();
                       if (context == "work")
                           email.setContexts(QContactDetail::ContextWork);
                       if (context == "home")
                           email.setContexts(QContactDetail::ContextHome);
                       if (context == "other")
                           email.setContexts(QContactDetail::ContextOther);
                   }

                   contact.saveDetail(&email);
               } else {
                   qWarning() << "Email without address!";
               }
           }
        }
    }

    if (!cm->saveContact(&contact)) {
        qWarning() << "Error while adding contact, error id:" << cm->error();
        return false;
    }

    return true;

#else
    return false;
#endif
#else
    return false;
#endif
}

bool Server::updateContact(int id, const QByteArray &json)
{
#ifdef SAILFISH
#ifdef CONTACTS

    QContact contact = cm->contact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)));

    if (contact.isEmpty()) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse cantact data!" << parseError.errorString();
        qDebug() << json;
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Contact data is not JSON object!";
        return false;
    }

    QJsonObject obj = doc.object();

    QContactDisplayLabel label = contact.detail<QContactDisplayLabel>();
    if (obj.contains("label")) {
        if (label.label() != obj["label"].toString()) {
            label.setLabel(obj["label"].toString());
            contact.saveDetail(&label);
        }
    }

    QContactName name = contact.detail<QContactName>();
    if (obj.contains("first_name")) {
        if (name.firstName() != obj["first_name"].toString())
            name.setFirstName(obj["first_name"].toString());
    }
    if (obj.contains("last_name")) {
        if (name.lastName() != obj["last_name"].toString())
            name.setLastName(obj["last_name"].toString());
    }
    contact.saveDetail(&name);

    /*if (obj.contains("note") && obj["note"].isString() &&
            !obj["note"].toString().isEmpty()) {
        QContactNote note;
        note.setNote(obj["note"].toString());
        contact.saveDetail(&note);
    }*/

    // Deleting all existing phones numbers
    QList<QContactPhoneNumber> ln = contact.details<QContactPhoneNumber>();
    for (QList<QContactPhoneNumber>::iterator it = ln.begin(); it != ln.end(); ++it) {
        contact.removeDetail(&(*it));
    }

    if (obj["phones"].isArray()) {
        QJsonArray arr = obj["phones"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("number") && obj["number"].isString() &&
                       !obj["number"].toString().isEmpty()) {

                   QContactPhoneNumber number;
                   number.setNumber(obj["number"].toString());

                   if (obj.contains("context") && obj["context"].isString() &&
                           !obj["context"].toString().isEmpty()) {
                       QString context = obj["context"].toString();
                       if (context == "work")
                           number.setContexts(QContactDetail::ContextWork);
                       if (context == "home")
                           number.setContexts(QContactDetail::ContextHome);
                       if (context == "other")
                           number.setContexts(QContactDetail::ContextOther);
                   }

                   if (obj.contains("type") && obj["type"].isString() &&
                           !obj["type"].toString().isEmpty()) {
                       QString type = obj["type"].toString();
                       if (type == "mobile") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeMobile);
                           number.setSubTypes(types);
                       } else if (type == "assistant") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeAssistant);
                           number.setSubTypes(types);
                       } else if (type == "pager") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypePager);
                           number.setSubTypes(types);
                       } else if (type == "video") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeVideo);
                           number.setSubTypes(types);
                       } else if (type == "fax") {
                           QList<int> types;
                           types.append(QContactPhoneNumber::SubTypeFax);
                           number.setSubTypes(types);
                       }
                   }

                   contact.saveDetail(&number);
               } else {
                   qWarning() << "Phone without number!";
               }
           }
        }
    }

    // Deleting all existing emials
    QList<QContactEmailAddress> le = contact.details<QContactEmailAddress>();
    for (QList<QContactEmailAddress>::iterator it = le.begin(); it != le.end(); ++it) {
        contact.removeDetail(&(*it));
    }

    if (obj["emails"].isArray()) {
        QJsonArray arr = obj["emails"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("address") && obj["address"].isString() &&
                       !obj["address"].toString().isEmpty()) {

                   QContactEmailAddress email;
                   email.setEmailAddress(obj["address"].toString());

                   if (obj.contains("context") && obj["context"].isString() &&
                           !obj["context"].toString().isEmpty()) {
                       QString context = obj["context"].toString();
                       if (context == "work")
                           email.setContexts(QContactDetail::ContextWork);
                       if (context == "home")
                           email.setContexts(QContactDetail::ContextHome);
                       if (context == "other")
                           email.setContexts(QContactDetail::ContextOther);
                   }

                   contact.saveDetail(&email);
               } else {
                   qWarning() << "Email without address!";
               }
           }
        }
    }

    if (!cm->saveContact(&contact)) {
        qWarning() << "Error while updating contact, error id:" << cm->error();
        return false;
    }

    return true;

#else
    return false;
#endif
#else
    return false;
#endif
}

bool Server::deleteContact(int id)
{
#ifdef SAILFISH
#ifdef CONTACTS

    if (!cm->removeContact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)))) {
        qWarning() << "Unable to remove contact with id =" << id;
        return false;
    }

    return true;

#else
    return false;
#endif
#else
    return false;
#endif
}

QByteArray Server::encrypt(const QByteArray & data)
{
    Settings* s = Settings::instance();
    QByteArray key = QCryptographicHash::hash(s->getCryptKey(),QCryptographicHash::Sha1);

    QByteArray e_data(data);

    char * array = e_data.data();

    int ksize = key.length();
    int l = e_data.length();
    int counter = l, temp, index;
    while (counter > 0) {
        counter--;
        index = (key.at(counter % ksize) * counter) % l;
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
    }

#ifdef SAILFISH
    QJsonObject obj;
    obj.insert("id",QString("encrypted_container"));
    obj.insert("ver",QString("1"));
    obj.insert("data", QLatin1String(e_data.toBase64()));
    e_data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#else
    e_data = "";
#endif

    return e_data;
}


/*QByteArray Server::encrypt(const QByteArray & data)
{
    Settings* s = Settings::instance();
    QByteArray key = s->getCryptKey();

    QByteArray out;

    int ksize = key.size();
    if (ksize == 0) {
        out = data;
    } else {
        for (int i = 0; i < data.size(); ++i) {
            //qDebug() << data.at(i) << "-" << (data.at(i) ^ key.at(i % ksize));
            out += data.at(i) ^ key.at(i % ksize);
        }
    }

#ifdef SAILFISH
    QJsonObject obj;
    obj.insert("id",QString("encrypted_container"));
    obj.insert("algorithm",QString("XOR1"));
    obj.insert("data", QString(out.toBase64()));
    out = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#else
    out = "";
#endif

    return out;
}*/

QByteArray Server::decrypt(const QByteArray & json)
{
    QByteArray data;

#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse json data!" << parseError.errorString();
        qDebug() << json;
        return "";
    }

    if (!doc.isObject()) {
        qWarning() << "Encrypted data is not JSON object!";
        return "";
    }

    QJsonObject obj = doc.object();

    if (!obj.contains("id") || !obj["id"].isString() ||
            obj["id"].toString() != "encrypted_container") {
        qWarning() << "Encrypted data is not in valid container!";
        return "";
    }

    if (!obj.contains("ver") || !obj["ver"].isString() ||
            obj["ver"].toString() != "1") {
        qWarning() << "Unknown version!";
        return "";
    }

    if (!obj.contains("data") || !obj["data"].isString()) {
        qWarning() << "Data param is missing!";
        return "";
    }

    data = QByteArray::fromBase64(obj["data"].toString().toLocal8Bit());
#endif

    Settings* s = Settings::instance();
    QByteArray key = QCryptographicHash::hash(s->getCryptKey(),QCryptographicHash::Sha1);

    QByteArray d_data(data);
    char * array = d_data.data();

    int ksize = key.length();
    int counter = 0, temp, index;
    int l = d_data.length();
    while (counter < l) {
        index = (key.at(counter % ksize) * counter) % l;
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
        counter++;
    }

    return d_data;
}

/*QByteArray Server::decrypt(const QByteArray & json)
{
    QByteArray data;

#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse json data!" << parseError.errorString();
        qDebug() << json;
        return "";
    }

    if (!doc.isObject()) {
        qWarning() << "Encrypted data is not JSON object!";
        return "";
    }

    QJsonObject obj = doc.object();

    if (!obj.contains("id") || !obj["id"].isString() || obj["id"].toString() != "encrypted_container") {
        qWarning() << "Encrypted data is not in valid container!";
        return "";
    }

    // Supports only XOR algorithm
    if (!obj.contains("algorithm") || !obj["algorithm"].isString() ||
            obj["algorithm"].toString() != "XOR1") {
        qWarning() << "Unknown algorithm!";
        return "";
    }

    if (!obj.contains("data") || !obj["data"].isString()) {
        qWarning() << "Data param is missing!";
        return "";
    }

    data = obj["data"].toString().toUtf8();
#endif

    Settings* s = Settings::instance();
    QByteArray key = s->getCryptKey();

    QByteArray out = QByteArray::fromBase64(data);

    int ksize = key.size();
    if (ksize == 0)
        return out;

    // XOR
    char * c_out = out.data();
    int l_out = out.size();
    for (int i = 0; i < l_out; ++i) {
        c_out[i] ^= key.at(i % ksize);
    }

    return out;
}*/

QByteArray Server::getOptions()
{
    Settings* s = Settings::instance();

#ifdef SAILFISH
    QJsonObject obj;
    obj.insert("platform",QString("sailfish"));
    obj.insert("api_version",QString("2.1"));
    obj.insert("encryption_enabled", s->getCrypt());
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
#endif
    return "";
}


