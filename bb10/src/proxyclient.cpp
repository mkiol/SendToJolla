/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

  This file is part of SendToPhone application.

  This Source Code Form is subject to the terms of
  the Mozilla Public License, v.2.0. If a copy of
  the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <QNetworkRequest>
#include <QByteArray>
#include <QStringList>
#include <QTimer>
#include <QDebug>

#ifdef SAILFISH
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#endif

#include "proxyclient.h"
#include "settings.h"

ProxyClient::ProxyClient(QObject *parent) :
    QObject(parent),
    wait(60),
    sid (0),
    clientId(""),
    closing(false), opening(false), opened(false),
    proxyUrl("")
{
    inReply = NULL;
    nam = new QNetworkAccessManager(this);

    connect(nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrorHandle(QNetworkReply*,QList<QSslError>)));
}

ProxyClient::~ProxyClient()
{
    if (isOpen())
        close();
    delete nam;
}

void ProxyClient::closeHandler()
{
    //qDebug() << "Close handler";

    if (closing && inReply != NULL) {
        inReply->abort();
        //inReply->deleteLater();
        //inReply = NULL;
    }

    setClosing(false);
}

bool ProxyClient::close()
{
    if (closing) {
        qWarning() << "Waiting for disconnect!";
        return false;
    }

    if (opening) {
        qWarning() << "Waiting for connect!";
        return false;
    }

    if (!isOpen()) {
        qWarning() << "Not connected!";
        return true;
    }

    QString body = QString("{\"id\":\"disconnect\",\"app\":\"%1\",\"sid\":%2}")
            .arg(this->clientId).arg(this->sid);

    if (sendOutRequest(body.toUtf8())) {
        setClosing(true);
        QTimer::singleShot(ProxyClient::closeDelay, this, SLOT(closeHandler()));
    } else {
        setOpen(false);
        if (inReply != NULL) {
            inReply->abort();
            //inReply->deleteLater();
            //inReply = NULL;
        }

        setClosing(false);
    }

    return true;
}

bool ProxyClient::isOpen()
{
    return opened;
}

bool ProxyClient::isOpening()
{
    return opening;
}

bool ProxyClient::isClosing()
{
    return closing;
}

bool ProxyClient::open(const QString & clientId, const QUrl &proxyUrl)
{
    if (closing) {
        qWarning() << "Waiting for disconnect!";
        return false;
    }

    if (opening) {
        qWarning() << "Waiting for connect!";
        return false;
    }

    if (clientId == "") {
        qWarning() << "Client ID is invalid!";
        return false;
    }

    this->clientId = clientId;

    if (!proxyUrl.isValid()) {
        qWarning() << "Url is invalid!";
        return false;
    }

    if (isOpen()) {
        qWarning() << "Already connected!";
        return false;
    }

    this->proxyUrl = proxyUrl;
    this->sid = 0;

    QString data = QString("{\"id\":\"connect\",\"app\":\"%1\",\"wait\":%2}")
            .arg(this->clientId).arg(this->wait);

    //qDebug() << "connect" << data;

    QNetworkRequest request(proxyUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    inReply = nam->post(request, data.toUtf8());

    connect(inReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorHandler(QNetworkReply::NetworkError)));
    connect(inReply, SIGNAL(finished()), this, SLOT(inFinishedHandler()));

    setOpening(true);

    return true;
}

bool ProxyClient::reconnect()
{
    if (closing) {
        qWarning() << "Waiting for disconnect!";
        return false;
    }

    if (opening) {
        qWarning() << "Waiting for connect!";
        return false;
    }

    if (!isOpen()) {
        qWarning() << "Not connected!";
        return false;
    }

    if (!proxyUrl.isValid()) {
        qWarning() << "Url is invalid!";
        emit error(410);
        return false;
    }

    if (inReply != NULL) {
        inReply->close();
        inReply->deleteLater();
    }

    QString data = QString("{\"id\":\"connect\",\"app\":\"%1\",\"wait\":%2,\"sid\":%3}")
            .arg(this->clientId).arg(this->wait).arg(this->sid);

    //qDebug() << "reconnect" << data;

    QNetworkRequest request(proxyUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    inReply = nam->post(request, data.toUtf8());

    connect(inReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorHandler(QNetworkReply::NetworkError)));
    connect(inReply, SIGNAL(finished()), this, SLOT(inFinishedHandler()));

    return true;
}

void ProxyClient::errorHandler(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (reply == inReply)
        qDebug() << "Error in in-request!";
    else
        qDebug() << "Error in out-request!";

    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray phrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray();

    /*qWarning() << "Network error!" << "Url:" << reply->url().toString()
               << "HTTP code:" << httpCode << phrase;*/

    qWarning() << "Network error!" << "Url:" << reply->url().toString() << "Error code:" << code
               << "HTTP code:" << httpCode << phrase << "Content:" << reply->readAll();

}

#ifdef SAILFISH
QJsonObject ProxyClient::parseJson(const QByteArray &json)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Can not parse JSON data!" << parseError.errorString();
        qDebug() << json;
        return QJsonObject();
    }

    if (!doc.isObject()) {
        qWarning() << "JSON is not an object!";
        qDebug() << json;
        return QJsonObject();
    }

    return doc.object();
}
#endif

void ProxyClient::resetReply(QNetworkReply *reply)
{
    if (reply != NULL) {
        reply->close();
        reply->deleteLater();

        if (reply == inReply) {
            inReply = NULL;
            setOpening(false);
            setClosing(false);
            setOpen(false);
        }
    }
}

bool ProxyClient::sendData(const QString &tid, const QByteArray &data)
{
    if (!isOpen()) {
        qWarning() << "Not connected!";
        return false;
    }

    if (opening) {
        qWarning() << "Waiting for connect!";
        return false;
    }

    if (data.isEmpty()) {
        qWarning() << "Data is empty!";
        return false;
    }

    if (tid.isEmpty()) {
        qWarning() << "Transaction ID is empty!";
        return false;
    }

    QString body = QString("{\"id\":\"data\",\"app\":\"%1\",\"sid\":%2,\"tid\":\"%3\",\"data\":\"%4\"}")
            .arg(this->clientId).arg(this->sid).arg(tid, QString(data.toBase64()));

    return sendOutRequest(body.toUtf8());
}

bool ProxyClient::sendOutRequest(const QByteArray &body)
{
    if (body.isEmpty()) {
        qWarning() << "Data is empty!";
        return false;
    }

    //qDebug() << "Sending:" << body;
    //qDebug() << "Sending";

    QNetworkRequest request(proxyUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = nam->post(request, body);

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorHandler(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(outFinishedHandler()));

    return true;
}

void ProxyClient::inFinishedHandler()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();

    setOpening(false);

    if (reply->error() == QNetworkReply::NoError) {

       //qDebug() << "Network in reply received!" << "Content:" << data;
#ifdef SAILFISH
        QJsonObject obj = parseJson(data);
        if (obj.isEmpty()) {
            qWarning() << "JSON is empty";
            emit error(401);
            resetReply(reply);
            return;
        }

        if (obj["id"].isString()) {

            if (!obj["sid"].isDouble() || obj["sid"].toInt() == 0) {
                qWarning() << "SID param is missing!";
                emit error(402);
                resetReply(reply);
                return;
            }

            if (obj["id"].toString() == "connect_ack") {

                if (!obj["wait"].isDouble() || obj["wait"].toInt() == 0) {
                    qDebug() << "Disconnecting in-session!";
                    setClosing(false);
                    resetReply(reply);
                    return;
                }

                if (this->sid == 0) {
                    emit connected();
                    setOpen(true);
                }

                this->sid = obj["sid"].toInt();
                this->wait = obj["wait"].toInt();

                reconnect();
                return;

            } else if (obj["id"].toString() == "data") {

                if (!obj["data"].isString() || obj["data"].toString().isEmpty()) {
                    qWarning() << "Data param is missing!";
                    emit error(403);
                    resetReply(reply);
                    return;
                }

                //qDebug() << "New data arrived!";
                QStringList dataList = obj["data"].toString().split(";", QString::SkipEmptyParts);
                for (QStringList::iterator it = dataList.begin(); it != dataList.end(); ++it) {
                    emit dataReceived(QByteArray::fromBase64((*it).toUtf8()));
                }

                reconnect();
                return;
            }
        }
#endif

        qWarning() << "Unknown reply!";
        emit error(404);
        resetReply(reply);
        return;
    }

    // Error
    /*QNetworkReply::NetworkError code = reply->error();
    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray phrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray();
    qDebug() << "Network reply error!" << "Url:" << reply->url().toString() << "Error code:" << code
             << "HTTP code:" << httpCode << phrase << "Content:" << data;*/

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        //qDebug() << "User disconnect!";
        resetReply(reply);
        return;
    }

    if (reply->error() == QNetworkReply::TimeoutError) {
        //qDebug() << "Network reply timeout!";
        emit error(reply->error());
        resetReply(reply);
        return;
    }

    if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
        qDebug() << "SSL error!";
        emit error(reply->error());
        resetReply(reply);
        return;
    }

    qWarning() << "Unknown error!";
    emit error(reply->error());
    resetReply(reply);
    return;
}

void ProxyClient::outFinishedHandler()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();

    if (reply->error() == QNetworkReply::NoError) {

        //qDebug() << "Network out reply received!" << "Content:" << data;
#ifdef SAILFISH
        QJsonObject obj = parseJson(data);
        if (obj.isEmpty()) {
            qWarning() << "JSON is empty";
            resetReply(reply);
            return;
        }

        if (obj["id"].isString()) {

            if (!obj["sid"].isDouble() || obj["sid"].toInt() == 0) {
                qWarning() << "SID param is missing!";
                resetReply(reply);
                return;
            }

            if (obj["id"].toString() == "data_ack") {
                //qDebug() << "New data_ack arrived!";
                reply->close();
                return;

            } else if (obj["id"].toString() == "disconnect_ack") {

                //qDebug() << "New disconnect_ack arrived!";

                resetReply(reply);
                return;
            }
        }
#endif

        qWarning() << "Unknown reply!";
        resetReply(reply);
        return;
    }

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        //qDebug() << "User disconnect!";
        resetReply(reply);
        return;
    }

    if (reply->error() == QNetworkReply::TimeoutError) {
        //qDebug() << "Network reply timeout!";
        resetReply(reply);
        return;
    }

    qWarning() << "Unknown error!";
    resetReply(reply);
    return;
}

void ProxyClient::setClosing(bool value)
{
    if (value != closing) {
        closing = value;
        emit closingChanged();
    }
}

void ProxyClient::setOpening(bool value)
{
    if (value != opening) {
        opening = value;
        emit openingChanged();
    }
}

void ProxyClient::setOpen(bool value)
{
    if (value != opened) {
        opened = value;
        emit openChanged();
    }
}

void ProxyClient::sslErrorHandle(QNetworkReply * reply, QList<QSslError> error)
{
    Q_UNUSED(error)

    Settings* s = Settings::instance();
    if (s->isIgnoreSSL())
        reply->ignoreSslErrors();
}
