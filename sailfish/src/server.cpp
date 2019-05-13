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
#include <QPair>
#include <QList>
#include <QTextStream>

#ifdef BB10
#include <bps/navigator.h>
#include <bb/system/Clipboard>
#include <bb/pim/notebook/Notebook>
#include <bb/pim/account/Account>
#include <bb/pim/account/Service>
#include <bb/pim/account/AccountService>
#include <bb/pim/account/Provider>
#include <bb/pim/notebook/NotebookService>
#include <bb/pim/contacts/ContactService>
#include <bb/pim/contacts/ContactSearchFilters>
#include <bb/pim/contacts/Contact>
#include <bb/pim/contacts/ContactAttribute>
#include <bb/pim/contacts/ContactBuilder>
#include <bb/pim/contacts/ContactAttributeBuilder>

#include <QByteArray>
#include <QtGui/QDesktopServices>
#include <QtCore/qmath.h>

#include "QJson/serializer.h"
#include "QJson/parser.h"

using namespace bb::pim::account;
using namespace bb::pim::notebook;
using namespace bb::pim::contacts;
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
#include <QtMath>
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
#include <QNetworkAddressEntry>

#include "server.h"
#include "settings.h"
#include "utils.h"

#ifdef SAILFISH
#ifdef CONTACTS
using namespace QtContacts;
#endif
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

#ifdef PROXY
    // Proxy
    connect(&proxy, SIGNAL(dataReceived(QByteArray)), this, SLOT(proxyDataReceived(QByteArray)));
    connect(&proxy, SIGNAL(openChanged()), this, SLOT(proxyOpenHandler()));
    connect(&proxy, SIGNAL(error(int)), this, SLOT(proxyErrorHandler(int)));
    connect(&proxy, SIGNAL(closingChanged()), this, SLOT(proxyBusyHandler()));
    connect(&proxy, SIGNAL(openingChanged()), this, SLOT(proxyBusyHandler()));
#endif

    // Local server
    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handleLocalServer(QHttpRequest*, QHttpResponse*)));

    // New cookie
    Settings* s = Settings::instance();
    connect(s, SIGNAL(cookieChanged()), this, SLOT(cookieChangedHandler()));

    // Contacts
#ifdef SAILFISH
#ifdef CONTACTS
    Utils::setPrivileged(true);
    cm = new QContactManager();
    Utils::setPrivileged(false);
#endif
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

    //qDebug() << "onlineStateChanged" << state;

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

const QHostAddress Server::getAddress()
{
    QList<QNetworkInterface> nlist = QNetworkInterface::allInterfaces();
    //emit newEvent("Server::getAddress");
    for (QList<QNetworkInterface>::iterator it = nlist.begin(); it != nlist.end(); ++it) {
        QNetworkInterface interface = *it;

        /*qDebug() << "interface:" << interface.name() << interface.flags().testFlag(QNetworkInterface::IsUp) << interface.flags().testFlag(QNetworkInterface::IsRunning);
        emit newEvent(interface.name() + " " + (interface.flags().testFlag(QNetworkInterface::IsUp) ? "true" : "false") + " " + (interface.flags().testFlag(QNetworkInterface::IsRunning) ? "true" : "false"));
        QList<QNetworkAddressEntry> aalist = interface.addressEntries();
                    for (QList<QNetworkAddressEntry>::iterator it = aalist.begin(); it != aalist.end(); ++it) {
                        QHostAddress haddr = (*it).ip();
                        if (haddr.protocol() == QAbstractSocket::IPv4Protocol) {
                            qDebug() << haddr.toString();
                            emit newEvent(haddr.toString());
                        }
                    }*/
        if (interface.isValid() && !interface.flags().testFlag(QNetworkInterface::IsLoopBack)
                && interface.flags().testFlag(QNetworkInterface::IsUp)
                && interface.flags().testFlag(QNetworkInterface::IsRunning)
#ifdef SAILFISH
                && (interface.name() == "eth0" || interface.name() == "wlan0" || interface.name() == "tether" || interface.name() == "rndis0")) {
#elif BB10
                && (interface.name() == "en0" || interface.name() == "bcm0" || interface.name() == "bcm1")) {
#endif
            //qDebug() << "interface:" << interface.name();
            QList<QNetworkAddressEntry> alist = interface.addressEntries();
            for (QList<QNetworkAddressEntry>::iterator it = alist.begin(); it != alist.end(); ++it) {
                QHostAddress haddr = (*it).ip();
                if (haddr.protocol() == QAbstractSocket::IPv4Protocol) {
                    //qDebug() << haddr.toString();
                    return haddr;
                }
            }
        }
    }
    return QHostAddress();
}

bool Server::startLocalServer()
{
    /*bool bearerOk = false;
    QList<QNetworkConfiguration> activeConfigs = ncm.allConfigurations(QNetworkConfiguration::Active);
    QList<QNetworkConfiguration>::iterator i = activeConfigs.begin();
    while (i != activeConfigs.end()) {
        QNetworkConfiguration c = (*i);
        qDebug() << c.bearerTypeName() << c.identifier() << c.name() << c.bearerType();
        if (c.bearerType()==QNetworkConfiguration::BearerWLAN ||
            c.bearerType()==QNetworkConfiguration::BearerEthernet) {
            bearerOk = true;
            break;
        }
        ++i;
    }*/

    if (getAddress().isNull()) {
        //qWarning() << "Local server is failed to start because WLAN is not active.";
        emit newEvent(tr("Local server is failed to start because WLAN is not active"));
        return false;
    }

    Settings* s = Settings::instance();
    int port = s->getPort();

    //qDebug() << "isListening1" << isListening;
    if (isListening != server->listen(port)) {
        isListening = !isListening;
        emit runningChanged();
    }
    //qDebug() << "isListening2" << isListening;

    if (isListening) {
        //qDebug() << "Server listening on" << port << "port.";
        emit newEvent(tr("Local server is started"));
    } else {
        qWarning() << "Local server is failed to start on" << port << "port";
        emit newEvent(tr("Local server is failed to start on %1 port").arg(port));
    }

    return isListening;
}
#ifdef PROXY
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

        if (api == "get-bookmarks-file") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json; charset=utf-8", getBookmarksFile(), s->getCrypt());
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

        if (api == "set-bookmarks-file") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendProxyResponse(uid, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!setBookmarksFile(body)) {
                qWarning() << "Error in bookmarks file!";
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

#ifdef CONTACTS
        if (api == "get-contacts") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendProxyResponse(uid, 200, "application/json", getContacts(data), s->getCrypt());
            return;
        }

        if (api == "get-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendProxyResponse(uid,400);
                return;
            }

            QByteArray json = getContact(data.toInt());
            if (json.isEmpty())
                sendProxyResponse(uid, 404);
            else
                sendProxyResponse(uid, 200, "application/json", json, s->getCrypt());
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

            if (!createContact(body)) {
                qWarning() << "Error creating contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);

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

            if (!updateContact(data.toInt(), body)) {
                qWarning() << "Error updating contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);

            return;
        }

        if (api == "delete-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendProxyResponse(uid,400);
                return;
            }

            if (!deleteContact(data.toInt())) {
                qWarning() << "Error deleting contact!";
                sendProxyResponse(uid, 400);
                return;
            }

            sendProxyResponse(uid);

            return;
        }
#endif

    }

    qWarning() << "Unknown request!";
    sendProxyResponse(uid,404);
}
#endif

void Server::bodyReceived()
{
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());
    QHttpResponse* resp = respMap.take(req);

    //qDebug() << "bodyReceived, url: " << req->url().toString();

    // Hack for jquery.min.js
    if (req->url().toString() == "/jquery.min.js") {
        QByteArray body;
        if (getWebContent("jquery.min.js", body)) {
            sendResponse(req, resp, 200, "application/x-javascript; charset=utf-8", body);
            return;
        }
        sendResponse(req, resp, 404);
        return;
    }

#ifdef SAILFISH
    QUrlQuery query(req->url());
    if (query.hasQueryItem("app")) {
#elif BB10
    if (req->url().hasQueryItem("app")) {
#endif
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

#ifdef SAILFISH
    QUrlQuery query(req->url());
    QString cookie = query.queryItemValue("app");
    if (cookie != s->getCookie()) {
        qWarning() << "Invalid cookie!";
        sendResponse(req, resp, 404);
        return;
    }
    if (query.hasQueryItem("api")) {
        QString api = query.queryItemValue("api");
        QString data = query.queryItemValue("data");
#elif BB10
    QString cookie = req->url().queryItemValue("app");
    if (cookie != s->getCookie()) {
        qWarning() << "Invalid cookie!";
        sendResponse(req, resp, 404);
        return;
    }
    if (req->url().hasQueryItem("api")) {
        QString api = req->url().queryItemValue("api");
        QString data = req->url().queryItemValue("data");
#endif
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

        if (api == "get-bookmarks-file") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getBookmarksFile(), s->getCrypt());
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

        if (api == "set-bookmarks-file") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (s->getCrypt()) {
                body = decrypt(body);
                if (body.length() == 0) {
                    qWarning() << "Error decrypting!";
                    sendResponse(req, resp, 400, "application/json", "{\"id\": \"error\", \"type\": \"decryption_failed\"}");
                    return;
                }
            }

            if (!setBookmarksFile(body)) {
                qWarning() << "Error in bookmark file!";
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

            setClipboard(QString::fromUtf8(body.constData()));
            sendResponse(req, resp);
            return;
        }

        if (api == "get-accounts") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getAccounts(), s->getCrypt());
            return;
        }

#ifdef CONTACTS
        if (api == "get-contacts") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "application/json", getContacts(data), s->getCrypt());
            return;
        }

        if (api == "get-contacts-vcard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));
            sendResponse(req, resp, 200, "text/vcard", getContactsVCard(data), s->getCrypt());
            return;
        }

        if (api == "get-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

            QByteArray json = getContact(data.toInt());
            if (json.isEmpty())
                sendResponse(req, resp, 404);
            else
                sendResponse(req, resp, 200, "application/json", json, s->getCrypt());
            return;
        }

        if (api == "get-contact-vcard") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

            QByteArray vc = getContactVCard(data.toInt());
            if (vc.isEmpty())
                sendResponse(req, resp, 404);
            else
                sendResponse(req, resp, 200, "text/vcard", vc, s->getCrypt());
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

            if (!createContact(body)) {
                qWarning() << "Error creating contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);

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

            if (!updateContact(data.toInt(), body)) {
                qWarning() << "Error updating contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);

            return;
        }

        if (api == "delete-contact") {
            emit newEvent(QString("%1 request from %2").arg(api).arg(source));

            if (data.isEmpty()) {
                sendResponse(req, resp, 400);
                return;
            }

            if (!deleteContact(data.toInt())) {
                qWarning() << "Error deleting contact!";
                sendResponse(req, resp, 400);
                return;
            }

            sendResponse(req, resp);

            return;
        }
#endif

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

#ifdef PROXY
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
#endif

void Server::sendResponse(QHttpRequest *req, QHttpResponse *resp, int status, const QString &contentType, const QByteArray &body, bool encrypt)
{
    Q_UNUSED(req)

    QByteArray nbody = body;

    if (status == 204) {
        resp->setHeader("Content-Length", "0");
    } else {
        if (encrypt) {
            nbody = this->encrypt(body);
            if (nbody.isEmpty()) {
                // Something is wrong. Enrypted data cannot be empty.
                status = 500;
                resp->setHeader("Content-Length", "0");
            } else {
                resp->setHeader("Content-Type", "application/json");
            }
        } else {
            if (status == 200 && body.isEmpty()) {
                status = 204;
            } else {
                resp->setHeader("Content-Type", contentType);
            }
        }
        resp->setHeader("Content-Length", QString::number(nbody.size()));
    }
    resp->setHeader("Connection", "close");
    resp->writeHead(status);
    resp->end(nbody);
}

QString Server::getLocalServerUrl()
{
    QHostAddress addr = getAddress();

    if (addr.isNull())
        return "";

    Settings* s = Settings::instance();

    return QString("http://%1:%2/%3").arg(addr.toString()).arg(s->getPort()).arg(s->getCookie());
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
    return QString::fromUtf8(clipboard.value("text/plain").constData());
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
    QString icon;
    if (obj["icon"].isString() &&
        (obj["icon"].toString().startsWith("http://",Qt::CaseInsensitive) ||
        obj["icon"].toString().startsWith("https://",Qt::CaseInsensitive) ||
        obj["icon"].toString().startsWith("data:image/",Qt::CaseInsensitive))) {
        icon = obj["icon"].toString();
    } else {
        icon = "icon-launcher-bookmark";
    }

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
    /*if (obj["icon"].isString() && !obj["icon"].toString().isEmpty())
        oldObj["favicon"] = obj["icon"];*/

    QJsonValue icon = QJsonValue::fromVariant("icon-launcher-bookmark");

    if (obj.contains("icon")) {
        if (obj["icon"].toString().startsWith("http://",Qt::CaseInsensitive) ||
            obj["icon"].toString().startsWith("https://",Qt::CaseInsensitive) ||
            obj["icon"].toString().startsWith("data:image/",Qt::CaseInsensitive)) {
            oldObj["favicon"] = obj["icon"];
        } else if (obj["icon"].toString().isEmpty()) {
            oldObj["favicon"] = icon;
        }
    }

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

QByteArray Server::getBookmarksFile()
{
#ifdef SAILFISH
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
            "/org.sailfishos/sailfish-browser/bookmarks.json");

    if (!file.exists()) {
        qWarning() << "Bookmarks file doesn't exist!" << file.fileName();
        return "";
    }

    if(!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Can not open bookmarks file!" << file.errorString();
        return "";
    }

    return file.readAll();
#else
    return "";
#endif
}

bool Server::setBookmarksFile(const QByteArray &data)
{
#ifdef SAILFISH
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data,&parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse bookmarks file!" << parseError.errorString();
        return false;
    }

    if (!doc.isArray()) {
        qWarning() << "Bookmarks file is not JSON array!";
        return false;
    }

    return writeBookmarks(doc.array());
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
                icon.startsWith("https://",Qt::CaseInsensitive) ||
                icon.startsWith("data:image/",Qt::CaseInsensitive))
                newObj.insert("icon",icon);

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
#ifdef PROXY
    return proxy.isOpen();
#else
    return false;
#endif
}

#ifdef PROXY
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
#endif

bool Server::isProxyBusy()
{
#ifdef PROXY
    return proxy.isClosing() || proxy.isOpening();
#else
    return false;
#endif
}

QString Server::getProxyUrl()
{
#ifdef PROXY
    Settings* s = Settings::instance();
    return s->getProxyUrl();
#else
    return "";
#endif
}

QString Server::getWebClientProxyUrl()
{
#ifdef PROXY
    Settings* s = Settings::instance();
    return s->getProxyUrl() + "?int=out&app=" + s->getCookie() + "&api=web-client";
#else
    return "";
#endif
}

void Server::cookieChangedHandler()
{
#ifdef PROXY
    if (proxy.isOpen()) {
        autoStartProxy = true;
        proxy.close();
    }
#endif
}

#ifdef CONTACTS
QByteArray Server::getContacts(const QString &filter)
{
#ifdef SAILFISH
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

        if (!name.firstName().isEmpty() || !name.lastName().isEmpty()) {
            QJsonObject obj;
            obj.insert("id",id);
            obj.insert("label", label.label());
            array.append(obj);
        }
    }

    return QJsonDocument(array).toJson(QJsonDocument::Compact);
#elif BB10
    QVariantList jsonArray;

    ContactService service;

    QList< QPair<SortColumn::Type,SortOrder::Type> > sortList;
    sortList << QPair<SortColumn::Type, SortOrder::Type>(SortColumn::FirstName, SortOrder::Ascending);
    sortList << QPair<SortColumn::Type, SortOrder::Type>(SortColumn::LastName, SortOrder::Ascending);

    QList<Contact> list;
    if (filter.isEmpty()) {
        ContactListFilters filters;
        filters.setLimit(50);
        filters.setSortBy(sortList);
        list = service.contacts(filters);
    } else {
        ContactSearchFilters filters;
        filters.setLimit(50);
        filters.setSearchValue(filter);
        filters.setSortBy(sortList);
        list = service.searchContacts(filters);
    }

    if (!list.isEmpty()) {
        QList<Contact>::iterator it = list.begin();
        while(it != list.end()) {
            /*qDebug() << "Contact: " << (*it).displayName();
            qDebug() << (*it).firstName() << (*it).lastName();
            qDebug() << (*it).id();
            qDebug() << (*it).type();

            QList<ContactAttribute> alist = (*it).attributes();
            qDebug() << "ContactAttribute list size:" << alist.count();
            for (QList<ContactAttribute>::iterator ait = alist.begin(); ait != alist.end(); ++ait) {
                qDebug() << (*ait).label();
            }*/
            QVariantMap map;
            map.insert("id", (*it).id());
            map.insert("label", (*it).displayName());
            jsonArray.append(map);
            ++it;
        }
    }

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(jsonArray, &ok);

    if (!ok) {
      qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
      return "";
    }

    return json;
#endif
    return "";
}

QByteArray Server::getContact(int id)
{
#ifdef SAILFISH
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
#elif BB10
    /*AccountService aservice;
    QList<Account> acList = aservice.accounts(Service::Contacts);
    for (QList<Account>::iterator ait = acList.begin(); ait != acList.end(); ++ait) {
            qDebug() << (*ait).displayName() << (*ait).id() << (*ait).provider().name();
    }
    QList<Provider> apList = aservice.providers();
    for (QList<Provider>::iterator ait = apList.begin(); ait != apList.end(); ++ait) {
            qDebug() << (*ait).name();
    }*/

    // Finding contact with given ID
    ContactService service;
    Contact contact = service.contactDetails(id);
    if (contact.id() < 1) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return "";
    }

    /*
    qDebug() << "ContactAttribute list size:" << alist.count();
    qDebug() << "Contact sourceAccountCount:" << contact.sourceAccountCount();
    QList<ContactAttribute> alist = contact.attributes();
    for (QList<ContactAttribute>::iterator ait = alist.begin(); ait != alist.end(); ++ait) {
        qDebug() << (*ait).attributeDisplayLabel() << (*ait).kind() << (*ait).subKind() << (*ait).label() << (*ait).value();
    }*/

    QVariantMap obj;
    obj.insert("id",id);

    // Getting names
    if (!contact.displayName().isEmpty())
        obj.insert("label", contact.displayName());
    if (!contact.firstName().isEmpty())
        obj.insert("first_name", contact.firstName());
    if (!contact.lastName().isEmpty())
        obj.insert("last_name", contact.lastName());

    // Getting avatar
    if (!contact.smallPhotoFilepath().isEmpty()) {
        QFile file(contact.smallPhotoFilepath());
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

    // Getting account IDs
    QList<AccountId> aclist = contact.sourceAccountIds();
    if (!aclist.isEmpty()) {
        QVariantList array;
        for (QList<AccountId>::iterator ait = aclist.begin(); ait != aclist.end(); ++ait) {
            array.append(*ait);
        }
        obj.insert("accounts", array);
    }

    // Getting note
    QList<ContactAttribute> notes = contact.filteredAttributes(AttributeKind::Note);
    if (!notes.isEmpty() && !notes[0].value().isEmpty()) {
        obj.insert("note", notes[0].value());
    }

    // Getting numbers
    QList<ContactAttribute> numbers = contact.phoneNumbers();
    if (!numbers.isEmpty()) {
        QVariantList array;
        for (QList<ContactAttribute>::iterator it = numbers.begin(); it != numbers.end(); ++it) {
            QVariantMap obj; obj.insert("number", (*it).value());
            AttributeSubKind::Type type = (*it).subKind();
            switch (type) {
            case AttributeSubKind::PhoneMobile:
                obj.insert("type", QString("mobile"));
                break;
            case AttributeSubKind::Home:
                obj.insert("type", QString("home"));
                break;
            case AttributeSubKind::Work:
                obj.insert("type", QString("work"));
                break;
            case AttributeSubKind::Other:
                obj.insert("type", QString("other"));
                break;
            }
            array.append(obj);
        }
        obj.insert("phones", array);
    }

    // Getting emails
    QList<ContactAttribute> emails = contact.emails();
    if (!emails.isEmpty()) {
        QVariantList array;
        for (QList<ContactAttribute>::iterator it = emails.begin(); it != emails.end(); ++it) {
            QVariantMap obj; obj.insert("address", (*it).value());
            AttributeSubKind::Type type = (*it).subKind();
            switch (type) {
            case AttributeSubKind::Home:
                obj.insert("type", QString("home"));
                break;
            case AttributeSubKind::Work:
                obj.insert("type", QString("work"));
                break;
            case AttributeSubKind::Other:
                obj.insert("type", QString("other"));
                break;
            }
            array.append(obj);
        }
        obj.insert("emails", array);
    }

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(obj, &ok);

    if (!ok) {
      qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
      return "";
    }

    return json;
#endif
    return "";
}

QByteArray Server::getContactsVCard(const QString &filter)
{
#ifdef SAILFISH
    QContactSortOrder sort; sort.setDirection(Qt::DescendingOrder);
    QList<QContactSortOrder> sortList; sortList.push_front(sort);
    QList<QContact> results = filter.isEmpty() ?
                cm->contacts(sortList) :
                cm->contacts(QContactDisplayLabel::match(filter), sortList);

    QByteArray data;
    QTextStream out(&data);

    for (QList<QContact>::iterator it = results.begin(); it != results.end(); ++it) {
        int id = (*it).id().toString().split("-").last().toInt();
        out << getContactVCard(id);
    }

    out.flush();
    return data;
#endif
    return QByteArray();
}

QByteArray Server::getContactVCard(int id)
{
#ifdef SAILFISH
    QContact contact = cm->contact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)));

    if (contact.isEmpty()) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return QByteArray();
    }

    QByteArray data;
    QTextStream out(&data);

    QString label = contact.detail<QContactDisplayLabel>().label();
    QString fname = contact.detail<QContactName>().firstName();
    QString lname = contact.detail<QContactName>().lastName();
    QString avatar = contact.detail<QContactAvatar>().imageUrl().toLocalFile();
    QString org = contact.detail<QContactOrganization>().name();
    QString note = contact.detail<QContactNote>().note();

    out << "BEGIN:VCARD\n"
        << "VERSION:3.0\n";

    if (!label.isEmpty())
        out << "FN:" << label << "\n";
    if (!fname.isEmpty() || !lname.isEmpty()) {
        out << "N:"
            << (lname.isEmpty() ? "" : lname) << ";"
            << (fname.isEmpty() ? "" : fname) << ";"
            << ";;\n";
    }
    if (!org.isEmpty())
        out << "ORG:" << org << "\n";
    if (!note.isEmpty())
        out << "NOTE:" << note << "\n";
    if (!avatar.isEmpty()) {
        QFile file(avatar);
        if (file.exists()) {
            if(file.open(QIODevice::ReadOnly)) {
                out << "PHOTO;ENCODING=b;TYPE=JPEG:"
                    << file.readAll().toBase64() << "\n";
            } else {
                qWarning() << "Can not open avatar file!" << file.errorString();
            }
        } else {
            qWarning() << "Avatar file doesn't exist!" << file.fileName();
        }
    }

    QList<QContactPhoneNumber> numbers = contact.details<QContactPhoneNumber>();
    if (!numbers.isEmpty()) {
        for (QList<QContactPhoneNumber>::iterator it = numbers.begin(); it != numbers.end(); ++it) {
            QString number = (*it).number();
            if (!number.isEmpty()) {
                QString type;
                QList<int> contexts = (*it).contexts();
                for (QList<int>::iterator it = contexts.begin(); it != contexts.end(); ++it) {
                    switch (*it) {
                    case QContactDetail::ContextHome:
                        type = "home";
                        break;
                    case QContactDetail::ContextWork:
                        type = "work";
                        break;
                    case QContactDetail::ContextOther:
                        type = "other";
                        break;
                    }
                }
                out << "TEL" << (type.isEmpty() ? "" : ";TYPE=" + type)
                    << ":" << number << "\n";
            }
        }
    }

    QList<QContactEmailAddress> emails = contact.details<QContactEmailAddress>();
    if (!emails.isEmpty()) {
        for (QList<QContactEmailAddress>::iterator it = emails.begin(); it != emails.end(); ++it) {
            QString address = (*it).emailAddress();
            if (!address.isEmpty()) {
                QString type;
                QList<int> contexts = (*it).contexts();
                for (QList<int>::iterator it = contexts.begin(); it != contexts.end(); ++it) {
                    switch (*it) {
                    case QContactDetail::ContextHome:
                        type = "home";
                        break;
                    case QContactDetail::ContextWork:
                        type = "work";
                        break;
                    case QContactDetail::ContextOther:
                        type = "other";
                        break;
                    }
                }
                out << "EMAIL" << (type.isEmpty() ? "" : ";TYPE=" + type)
                    << ":" << address << "\n";
            }
        }
    }

    out << "END:VCARD\n";
    out.flush();
    return data;
#elif BB10
    // Finding contact with given ID
    ContactService service;
    return service.contactToVCard(id, VCardPhotoEncoding::BASE64, 0);
#endif
    return "";
}

bool Server::createContact(const QByteArray &json)
{
#ifdef SAILFISH
    // Parsing JSON data
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

    // Note
    if (obj.contains("note") && obj["note"].isString() &&
            !obj["note"].toString().isEmpty()) {
        QContactNote note;
        note.setNote(obj["note"].toString());
        contact.saveDetail(&note);
    }

    // Phones
    if (obj["phones"].isArray()) {
        QJsonArray arr = obj["phones"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("number") && !obj["number"].toString().isEmpty()) {

                   QContactPhoneNumber number;
                   number.setNumber(obj["number"].toString());

                   if (obj.contains("context") && !obj["context"].toString().isEmpty()) {
                       QString context = obj["context"].toString();
                       if (context == "work")
                           number.setContexts(QContactDetail::ContextWork);
                       if (context == "home")
                           number.setContexts(QContactDetail::ContextHome);
                       if (context == "other")
                           number.setContexts(QContactDetail::ContextOther);
                   }

                   if (obj.contains("type") && !obj["type"].toString().isEmpty()) {
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

    // Emails
    if (obj["emails"].isArray()) {
        QJsonArray arr = obj["emails"].toArray();
        for (QJsonArray::iterator it = arr.begin(); it != arr.end(); ++it) {
           if ((*it).isObject()) {
               QJsonObject obj = (*it).toObject();
               if (obj.contains("address") && !obj["address"].toString().isEmpty()) {
                   QContactEmailAddress email;
                   email.setEmailAddress(obj["address"].toString());

                   if (obj.contains("context") && !obj["context"].toString().isEmpty()) {
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

    // Saving new Contact
    if (!cm->saveContact(&contact)) {
        qWarning() << "Error while adding contact, error id:" << cm->error();
        return false;
    }

    return true;
#elif BB10
    // Parsing JSON data
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

    if (obj["first_name"].toString().isEmpty() && obj["last_name"].toString().isEmpty()) {
        qWarning() << "Contact data is not valid!";
        return false;
    }

    ContactService service;
    ContactBuilder builder;

    // Adding names
    QString firstName = obj["first_name"].toString();
    QString lastName = obj["last_name"].toString();
    builder.addAttribute(ContactAttributeBuilder().setKind(AttributeKind::Name).setSubKind(AttributeSubKind::NameGiven).setValue(firstName));
    builder.addAttribute(ContactAttributeBuilder().setKind(AttributeKind::Name).setSubKind(AttributeSubKind::NameSurname).setValue(lastName));

    // Adding note
    if (obj.contains("note") && obj["note"].type() == QVariant::String) {
        builder.setNotes(obj["note"].toString());
    }

    /// Adding phones
    if (obj["phones"].type() == QVariant::List) {
        QVariantList array = obj["phones"].toList();
        for (QVariantList::iterator it = array.begin(); it != array.end(); ++it) {
            if ((*it).type() == QVariant::Map) {
                QVariantMap obj = (*it).toMap();
                if (obj.contains("number") && !obj["number"].toString().isEmpty()) {
                    ContactAttributeBuilder abuilder;
                    abuilder.setKind(AttributeKind::Phone).setValue(obj["number"].toString());

                    if (obj.contains("type") && !obj["type"].toString().isEmpty()) {
                        QString type = obj["type"].toString();
                        if (type == "mobile") {
                            abuilder.setSubKind(AttributeSubKind::PhoneMobile);
                        } else if (type == "home") {
                            abuilder.setSubKind(AttributeSubKind::Home);
                        } else if (type == "work") {
                            abuilder.setSubKind(AttributeSubKind::Work);
                        } else if (type == "other") {
                            abuilder.setSubKind(AttributeSubKind::Other);
                        }
                    }

                    builder.addAttribute(abuilder);
                } else {
                    qWarning() << "Phone without number!";
                }
            }
        }
    }

    // Adding emails
    if (obj["emails"].type() == QVariant::List) {
        QVariantList array = obj["emails"].toList();
        for (QVariantList::iterator it = array.begin(); it != array.end(); ++it) {
            if ((*it).type() == QVariant::Map) {
                QVariantMap obj = (*it).toMap();
                if (obj.contains("address") && !obj["address"].toString().isEmpty()) {
                    ContactAttributeBuilder abuilder;
                    qDebug() << obj["address"].toString();
                    abuilder.setKind(AttributeKind::Email).setValue(obj["address"].toString());

                    if (obj.contains("type") && !obj["type"].toString().isEmpty()) {
                        QString type = obj["type"].toString();
                        qDebug() << type;
                        if (type == "home") {
                            abuilder.setSubKind(AttributeSubKind::Home);
                        } else if (type == "work") {
                            abuilder.setSubKind(AttributeSubKind::Work);
                        } else if (type == "other") {
                            abuilder.setSubKind(AttributeSubKind::Other);
                        }
                    }

                    builder.addAttribute(abuilder);
                } else {
                    qWarning() << "Email without address!";
                }
            }
        }
    }

    // Saving new Contact
    service.createContact(builder, false);

    return true;
#endif
    return false;
}

bool Server::updateContact(int id, const QByteArray &json)
{
#ifdef SAILFISH
    // Finding contact with given ID
    QContact contact = cm->contact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)));
    if (contact.isEmpty()) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return false;
    }

    // Parsing JSON data
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

    // Updating note
    if (obj.contains("note")) {
        QContactNote note;
        note.setNote(obj["note"].toString());
        contact.saveDetail(&note);
    }

    // Deleting all existing phones numbers
    QList<QContactPhoneNumber> ln = contact.details<QContactPhoneNumber>();
    for (QList<QContactPhoneNumber>::iterator it = ln.begin(); it != ln.end(); ++it) {
        contact.removeDetail(&(*it));
    }

    // Adding new phones
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

    // Deleting all existing emails
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
#elif BB10
    // Finding contact with given ID
    ContactService service;
    Contact contact = service.contactDetails(id);
    if (contact.id() < 1) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return false;
    }

    // Parsing JSON data
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

    // Checking if contact data is valid
    if (obj["first_name"].toString().isEmpty() && obj["last_name"].toString().isEmpty()) {
        qWarning() << "Contact data is not valid!";
        return false;
    }

    // Builder for contact edit
    ContactBuilder builder = contact.edit();

    // Deleting all existing data that will be updated
    QList<ContactAttribute> alist = contact.attributes();
    for (QList<ContactAttribute>::iterator ait = alist.begin(); ait != alist.end(); ++ait) {
        if ((*ait).kind() == AttributeKind::Name && ((*ait).subKind() == AttributeSubKind::NameGiven || (*ait).subKind() == AttributeSubKind::NameSurname)) {
            builder.deleteAttribute((*ait));
        } else if ((*ait).kind() == AttributeKind::Phone || (*ait).kind() == AttributeKind::Email) {
            builder.deleteAttribute((*ait));
        }
    }

    // Updating names
    QString firstName = obj["first_name"].toString();
    QString lastName = obj["last_name"].toString();
    builder.addAttribute(ContactAttributeBuilder().setKind(AttributeKind::Name).setSubKind(AttributeSubKind::NameGiven).setValue(firstName));
    builder.addAttribute(ContactAttributeBuilder().setKind(AttributeKind::Name).setSubKind(AttributeSubKind::NameSurname).setValue(lastName));

    // Updating note
    if (obj.contains("note") && obj["note"].type() == QVariant::String) {
        builder.setNotes(obj["note"].toString());
    }

    // Updating phones
    if (obj["phones"].type() == QVariant::List) {
        QVariantList array = obj["phones"].toList();
        for (QVariantList::iterator it = array.begin(); it != array.end(); ++it) {
            if ((*it).type() == QVariant::Map) {
                QVariantMap obj = (*it).toMap();
                if (obj.contains("number") && !obj["number"].toString().isEmpty()) {
                    ContactAttributeBuilder abuilder;
                    abuilder.setKind(AttributeKind::Phone).setValue(obj["number"].toString());

                    if (obj.contains("type") && !obj["type"].toString().isEmpty()) {
                        QString type = obj["type"].toString();
                        if (type == "mobile") {
                            abuilder.setSubKind(AttributeSubKind::PhoneMobile);
                        } else if (type == "home") {
                            abuilder.setSubKind(AttributeSubKind::Home);
                        } else if (type == "work") {
                            abuilder.setSubKind(AttributeSubKind::Work);
                        } else if (type == "other") {
                            abuilder.setSubKind(AttributeSubKind::Other);
                        }
                    }

                    builder.addAttribute(abuilder);
                } else {
                    qWarning() << "Phone without number!";
                }
            }
        }
    }

    // Updating emails
    if (obj["emails"].type() == QVariant::List) {
        QVariantList array = obj["emails"].toList();
        for (QVariantList::iterator it = array.begin(); it != array.end(); ++it) {
            if ((*it).type() == QVariant::Map) {
                QVariantMap obj = (*it).toMap();
                if (obj.contains("address") && !obj["address"].toString().isEmpty()) {
                    ContactAttributeBuilder abuilder;
                    qDebug() << obj["address"].toString();
                    abuilder.setKind(AttributeKind::Email).setValue(obj["address"].toString());

                    if (obj.contains("type") && !obj["type"].toString().isEmpty()) {
                        QString type = obj["type"].toString();
                        qDebug() << type;
                        if (type == "home") {
                            abuilder.setSubKind(AttributeSubKind::Home);
                        } else if (type == "work") {
                            abuilder.setSubKind(AttributeSubKind::Work);
                        } else if (type == "other") {
                            abuilder.setSubKind(AttributeSubKind::Other);
                        }
                    }

                    builder.addAttribute(abuilder);
                } else {
                    qWarning() << "Email without address!";
                }
            }
        }
    }

    service.updateContact(builder);

    return true;
#endif
    return false;
}

bool Server::deleteContact(int id)
{
#ifdef SAILFISH
    if (!cm->removeContact(QContactId::fromString(
        "qtcontacts:org.nemomobile.contacts.sqlite::sql-" + QString::number(id)))) {
        qWarning() << "Unable to remove contact with id =" << id;
        return false;
    }

    return true;
#elif BB10
    // Finding contact with given ID
    ContactService service;
    Contact contact = service.contactDetails(id);
    if (contact.id() < 1) {
        qWarning() << "Contact with id =" << id << "does not exist!";
        return false;
    }

    service.deleteContact(id);

    return true;
#endif
    return false;
}
#endif // CONTACTS

QByteArray Server::getAccounts()
{
#ifdef BB10
    QVariantList jsonArray;

    // Accounts
    AccountService aservice;
    QList<Account> aclist = aservice.accounts(Service::Contacts);
    if (!aclist.isEmpty()) {
        for (QList<Account>::iterator it = aclist.begin(); it != aclist.end(); ++it) {
            //qDebug() << (*ait).displayName() << (*ait).id() << (*ait).provider().name();
            QVariantMap map;
            map.insert("id", (*it).id());
            map.insert("label", (*it).displayName().isEmpty() ? (*it).provider().name() : (*it).displayName());
            jsonArray.append(map);
        }
    }

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(jsonArray, &ok);

    if (!ok) {
      qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
      return "";
    }

    return json;
#endif
    return "";
}

QByteArray Server::encrypt(const QByteArray & data)
{
    Settings* s = Settings::instance();
    QByteArray key = QCryptographicHash::hash(s->getCryptKey(),QCryptographicHash::Sha1);

    QByteArray e_data(data);
    char * array = e_data.data();

    int ksize = key.size();
    int l = e_data.size();
    int counter = l, temp, index;

    while (counter > 0) {
        counter--;
        index = (static_cast<unsigned char>(key.at(counter % ksize)) * counter) % l;
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
    }

    /*int ksize = key.size();
    int l = data.size();

    char *array = new char[l + 1];
    strcpy(array, data.constData());
    qDebug() << ">>>>> l=" << l;

    int counter = l, temp = 0, index = 0;
    while (counter > 0) {
        //qDebug() << ">>>>>" << array[counter];
        counter--;
        index = (static_cast<unsigned char>(key.at(counter % ksize)) * counter) % l;
        //qDebug() << ">>>>> counter=" << counter << " index=" << index << "counter % ksize=" << counter % ksize << "key.at()" << static_cast<unsigned char>(key.at(counter % ksize));
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
    }

    qDebug() << "enrypted key:";
    for (int i = 0; i < ksize; ++i) {
        qDebug() <<  key.at(i) << static_cast<unsigned char>(key.at(i));
    }

    qDebug() << "enrypted data:";
    for (int i = 0; i < l; ++i) {
        qDebug() <<  array[i] << static_cast<unsigned char>(array[i]);
    }

    qDebug() << ">>>>>77";
    //return "";

    QByteArray e_data(array);

    //qDebug() << e_data;

    qDebug() << ">>>>>33";*/

#ifdef SAILFISH
    // Creating JSON container
    QJsonObject obj;
    obj.insert("id",QString("encrypted_container"));
    obj.insert("ver",QString("1"));
    obj.insert("data", QLatin1String(e_data.toBase64()));
    e_data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return e_data;
#elif BB10
    // Creating JSON container
    QVariantMap obj;
    obj.insert("id",QString("encrypted_container"));
    obj.insert("ver",QString("1"));
    obj.insert("data", QLatin1String(e_data.toBase64()));

    QJson::Serializer serializer; bool ok;
    e_data = serializer.serialize(obj, &ok);
    if (!ok) {
      qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
      return "";
    }
    return e_data;
#endif
    return "";
}

QByteArray Server::decrypt(const QByteArray & json)
{
    QByteArray data;

#ifdef SAILFISH
    // Parsing JSON data
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "An error occurred during parsing JSON!" << parseError.errorString();
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
#elif BB10
    // Parsing JSON data
    QJson::Parser parser;
    bool ok;
    QVariant result = parser.parse(json, &ok);
    if (!ok) {
        qWarning() << "An error occurred during parsing JSON!";
        return "";
    }
    if (result.type() != QVariant::Map) {
        qWarning() << "Encrypted data is not JSON object!";
        return "";
    }

    QVariantMap obj = result.toMap();

    if (!obj.contains("id") || obj["id"].toString() != "encrypted_container") {
        qWarning() << "Encrypted data is not in valid container!";
        return "";
    }

    if (!obj.contains("ver") || obj["ver"].toString() != "1") {
        qWarning() << "Unknown version!";
        return "";
    }

    if (!obj.contains("data") || obj["data"].type() != QVariant::String) {
        qWarning() << "Data param is missing!";
        return "";
    }
#endif
    data = QByteArray::fromBase64(obj["data"].toString().toLocal8Bit());

    Settings* s = Settings::instance();
    QByteArray key = QCryptographicHash::hash(s->getCryptKey(),QCryptographicHash::Sha1);

    QByteArray d_data(data);
    char * array = d_data.data();

    int ksize = key.length();
    int counter = 0, temp, index;
    int l = d_data.length();
    while (counter < l) {
        index = (static_cast<unsigned char>(key.at(counter % ksize)) * counter) % l;
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
        counter++;
    }

    return d_data;
}

QByteArray Server::getOptions()
{
    Settings* s = Settings::instance();

#ifdef SAILFISH
    QJsonObject obj;
    obj.insert("platform",QString("sailfish"));
#elif BB10
    QVariantMap obj;
    obj.insert("platform",QString("bb10"));
#endif
    obj.insert("encryption_enabled", s->getCrypt());
    obj.insert("api_version",QString("2.1"));

    QStringList apiList;
    apiList << "options" <<
               "open-url" <<
               "get-clipboard" <<
               "set-clipboard" <<
#ifdef SAILFISH
               "get-bookmarks" <<
               "update-bookmark" <<
               "create-bookmark" <<
               "delete-bookmark" <<
               "get-bookmarks-file" <<
               "set-bookmarks-file" <<
#endif
               "get-notes-list" <<
               "get-note" <<
               "delete-note" <<
               "update-note" <<
               "create-note" <<
#ifdef CONTACTS
               "get-contacts" <<
               "get-contact" <<
               "get-contact-vcard" <<
               "get-contacts-vcard" <<
               "create-contact" <<
               "update-contact" <<
               "delete-contact" <<
#endif
#ifdef BB10
               "get-accounts" <<
#endif
               "web-client";
#ifdef SAILFISH
    obj.insert("api_list", QJsonArray::fromStringList(apiList));

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
#elif BB10
    obj.insert("api_list", apiList);

    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(obj, &ok);
    if (!ok) {
        qWarning() << "Can not serialize JSON:" << serializer.errorMessage();
        return "";
    }

    return json;
#endif
    return "";
}


