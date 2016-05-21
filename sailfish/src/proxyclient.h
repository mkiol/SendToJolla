/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QByteArray>
#include <QSslError>
#include <QList>

#ifdef SAILFISH
#include <QJsonObject>
#endif

class ProxyClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY (bool open READ isOpen NOTIFY openChanged)
    Q_PROPERTY (bool closing READ isClosing NOTIFY closingChanged)
    Q_PROPERTY (bool opening READ isOpening NOTIFY openingChanged)

public:
    explicit ProxyClient(QObject *parent = 0);
    ~ProxyClient();
    bool open(const QString &clientId, const QUrl &proxyUrl);
    bool close();
    bool isOpen();
    bool isClosing();
    bool isOpening();
    //bool sendData(const QByteArray &data);
    bool sendData(const QString &tid, const QByteArray &data);

signals:
    void error(int code);
    void connected();
    void openChanged();
    void openingChanged();
    void closingChanged();
    void dataReceived(QByteArray data);

private slots:
    void errorHandler(QNetworkReply::NetworkError code);
    void outFinishedHandler();
    void inFinishedHandler();
    void closeHandler();
    void sslErrorHandle(QNetworkReply*,QList<QSslError>);

private:
    static const int closeDelay = 6000;
    QNetworkAccessManager* nam;
    QNetworkReply* inReply;
    QString clientId;
    QUrl proxyUrl;
    int wait;
    int sid;
    bool closing;
    bool opening;
    bool opened;

    bool reconnect();
#ifdef SAILFISH
    QJsonObject parseJson(const QByteArray &json);
#endif
    void resetReply(QNetworkReply *reply);
    bool sendOutRequest(const QByteArray &body);
    void setOpen(bool value);
    void setOpening(bool value);
    void setClosing(bool value);
};

#endif // PROXYCLIENT_H
