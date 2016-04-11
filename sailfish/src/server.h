/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QString>
#include <QNetworkConfigurationManager>
#include <QByteArray>
#include <QMap>

#ifdef SAILFISH
#include <QClipboard>
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#ifdef CONTACTS
#include <QtContacts/QContactManager>
#endif
#endif

#include "qhttpserver/qhttpserver.h"
#include "qhttpserver/qhttprequest.h"
#include "qhttpserver/qhttpresponse.h"

#include "proxyclient.h"

class Server : public QObject
{
    Q_OBJECT

    Q_PROPERTY (bool localServerRunning READ isRunning NOTIFY runningChanged)
    Q_PROPERTY (bool proxyOpen READ isProxyOpen NOTIFY proxyOpenChanged)
    Q_PROPERTY (bool proxyBusy READ isProxyBusy NOTIFY proxyBusyChanged)

public:
    explicit Server(QObject *parent = 0);
    ~Server();

    Q_INVOKABLE QString getLocalServerUrl();
    Q_INVOKABLE QString getProxyUrl();
    Q_INVOKABLE QString getWebClientProxyUrl();
    Q_INVOKABLE void startLocalServer();
    Q_INVOKABLE void stopLocalServer();
    Q_INVOKABLE void connectProxy();
    Q_INVOKABLE void disconnectProxy();
    Q_INVOKABLE void sendDataToProxy(const QString &data);

public slots:
    void handleLocalServer(QHttpRequest *req, QHttpResponse *resp);
    void handleProxy(const QJsonObject &obj);

private slots:
    void proxyOpenHandler();
    void proxyBusyHandler();
    void proxyDataReceived(QByteArray data);
    void proxyErrorHandler(int code);

    void bodyReceived();
    /*void bodyReceivedForSetClipboard();
    void bodyReceivedForUpdateNote();
    void bodyReceivedForCreateNote();
    void bodyReceivedForCreateBookmark();
    void bodyReceivedForUpdateBookmark();*/
#ifdef SAILFISH
    void clipboardChanged(QClipboard::Mode);
#endif
    void onlineStateChanged(bool state);
    void cookieChangedHandler();

signals:
    void newEvent(QString text);

    void runningChanged();
    void proxyOpenChanged();
    void proxyBusyChanged();

private:
    QByteArray encrypt(const QByteArray & data);
    QByteArray decrypt(const QByteArray & json);

    QHttpServer *server;
    QMap<QHttpRequest*,QHttpResponse*> respMap;
    QNetworkConfigurationManager ncm;
    QString clipboardData;
    ProxyClient proxy;
    bool autoStartProxy;

#ifdef SAILFISH
    QClipboard *clipboard;
#ifdef CONTACTS
    QtContacts::QContactManager * cm;
#endif
    QSqlDatabase notesDB;
    QString getNotesDBfile();
    QJsonArray readBookmarks();
    bool writeBookmarks(const QJsonArray &array);
    bool openNotesDB();
    void closeNotesDB();
#endif

    bool isListening;
    void launchBrowser(QString data);
    void setClipboard(QString data);
    QString getClipboard();
    QByteArray getNotes();
    QByteArray getBookmarks();
    QByteArray getBookmarksFile();
    QByteArray getNote(int id);
    bool deleteBookmark(int id);
    bool deleteNote(int id);
    bool createNote(const QByteArray &json);
    bool createBookmark(const QByteArray &json);
    bool updateBookmark(int id, const QByteArray &json);
    bool setBookmarksFile(const QByteArray &data);
    bool updateNote(int id, const QByteArray &json);
    bool getWebContent(const QString &file, QByteArray &data);
    bool isRunning();
    bool isProxyOpen();
    bool isProxyBusy();
    void sendResponse(QHttpRequest *req, QHttpResponse *resp, int status = 204,
                      const QString &contentType = "",
                      const QByteArray &body = "",
                      bool encrypt = false);
    void sendProxyResponse(const QString &uid, int status = 204,
                           const QString &contentType = "",
                           const QByteArray &body = "",
                           bool encrypt = false);
    void handleLocalServerNewApi(QHttpRequest *req, QHttpResponse *resp);
    void handleLocalServerOldApi(QHttpRequest *req, QHttpResponse *resp);
    QString getContentType(const QString & file);
    QByteArray getContacts(const QString & filter);
    QByteArray getContact(int id);
    bool createContact(const QByteArray &json);
    bool updateContact(int id, const QByteArray &json);
    bool deleteContact(int id);
    QByteArray getOptions();
};

#endif // SERVER_H
