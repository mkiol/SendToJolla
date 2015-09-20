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
#include <QFile>

#ifdef BB10
#include <bps/navigator.h>
#include <bb/system/Clipboard>
#include <QByteArray>
#endif

#ifdef SAILFISH
#include <QDesktopServices>
#include <QGuiApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfoList>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#endif

#include <QHostAddress>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QProcess>
#include <QNetworkConfiguration>
#include <QDateTime>

#include "server.h"
#include "settings.h"
#include "sailfishapp.h"
#include "utils.h"

Server::Server(QObject *parent) :
    QObject(parent), isListening(false)
{
    qsrand(QDateTime::currentDateTime().toTime_t());

    // --
    /*QList<QNetworkConfiguration> activeConfigs = ncm.allConfigurations(QNetworkConfiguration::Active);
    QList<QNetworkConfiguration>::iterator i = activeConfigs.begin();
    while (i != activeConfigs.end()) {
        QNetworkConfiguration c = (*i);
        qDebug() << c.bearerTypeName() << c.identifier() << c.name();
        if (c.bearerType()==QNetworkConfiguration::BearerWLAN ||
            c.bearerType()==QNetworkConfiguration::BearerEthernet) {
            break;
        }
        ++i;
    }*/
    // --

    connect(&ncm, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));

#ifdef SAILFISH
    clipboard = QGuiApplication::clipboard();
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));
    openNotesDB();
#endif

    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handle(QHttpRequest*, QHttpResponse*)));
}

Server::~Server()
{
    closeNotesDB();
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
    bool webClient = false;

    Settings* s = Settings::instance();
    if (path.length()>1 && path.at(1)==s->getCookie()) {

        if (path.length()>2 && path.at(2)=="get-clipboard") {
            QString data = getClipboard();
            resp->setHeader("Content-Length", QString::number(data.size()));
            resp->setHeader("Content-Type", "text/plain; charset=utf-8");
            resp->setHeader("Connection", "close");
            resp->writeHead(200);
            resp->end(data.toUtf8());
            ok = true;
        }

        if (path.length()>2 && path.at(2)=="get-notes-list") {
            QByteArray data = getNotes();
            if (data == "") {
                resp->setHeader("Content-Length", "0");
                resp->setHeader("Connection", "close");
                resp->writeHead(500);
                resp->end("");
            } else {
                resp->setHeader("Content-Length", QString::number(data.size()));
                resp->setHeader("Content-Type", "application/json");
                resp->setHeader("Connection", "close");
                resp->writeHead(200);
                resp->end(data);
            }
            ok = true;
        }

        if (path.length()>3 && path.at(2)=="get-note") {
            QByteArray data = getNote(path.at(3).toInt());
            if (data == "") {
                resp->setHeader("Content-Length", "0");
                resp->setHeader("Connection", "close");
                resp->writeHead(500);
                resp->end("");
            } else {
                resp->setHeader("Content-Length", QString::number(data.size()));
                resp->setHeader("Content-Type", "application/json");
                resp->setHeader("Connection", "close");
                resp->writeHead(200);
                resp->end(data);
            }
            ok = true;
        }

        if (path.length()>4 && path.at(2)=="update-note" && req->method()==QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForUpdateNote()));
            req->storeBody();
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(204);
            resp->end("");
            ok = true;
        }

        if (path.length()>3 && path.at(2)=="create-note" && req->method()==QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForCreateNote()));
            req->storeBody();
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(204);
            resp->end("");
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
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForSetClipboard()));
            req->storeBody();
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(204);
            resp->end("");
            ok = true;
        }

        // Web client

        QString filename = path.length()==3 ? path.at(2) : "";

        // Redirection server_url/cookie -> server_url/cookie/
        if (path.length()==2) {
            resp->setHeader("Location", s->getCookie()+"/");
            resp->setHeader("Content-Length", "0");
            resp->setHeader("Connection", "close");
            resp->writeHead(301);
            resp->end("");
            ok = true; webClient = true;
        }

        if (path.length()==3) {
            QByteArray data;
            if (getWebContent(filename, data)) {

                // Get extension
                QStringList l = filename.split(".");
                QString ext = "";
                if (l.length()>0)
                    ext = l[l.length()-1];

                resp->setHeader("Content-Type", ext == "css" ? "text/css; charset=utf-8" :
                                                ext == "gif" ? "image/gif" :
                                                ext == "js" ? "application/x-javascript" :
                                                "text/html; charset=utf-8");
                resp->setHeader("Content-Length", QString::number(data.size()));
                resp->setHeader("Connection", "close");
                resp->writeHead(200);
                resp->end(data);
                ok = true; webClient = true;
            }
        }
    }

    if (ok) {
        emit newEvent(webClient ? QString("Web client request from %1 received.").arg(req->remoteAddress()) :
                                  QString("%1 request from %2 received.").arg(path.at(2)).arg(req->remoteAddress()));
    } else {
        resp->setHeader("Content-Length", "0");
        resp->setHeader("Connection", "close");
        resp->writeHead(400);
        resp->end("");
        emit newEvent(tr("Unknown request from %1 received.").arg(req->remoteAddress()));
    }
}

void Server::bodyReceivedForSetClipboard() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());
    setClipboard(QString(req->body()));
}

void Server::bodyReceivedForUpdateNote() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());

    QStringList path = req->url().path().split("/");
    if (path.length()>4 && path.at(2)=="update-note" && req->method()==QHttpRequest::HTTP_POST) {
        if (!updateNote(path.at(3).toInt(),QUrl::fromPercentEncoding(path.at(4).toUtf8()),QString(req->body()))) {
            qWarning() << "Error updatig note with id:" << path.at(3).toInt();
        }
    }
}

void Server::bodyReceivedForCreateNote() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());

    QStringList path = req->url().path().split("/");
    if (path.length()>3 && path.at(2)=="create-note" && req->method()==QHttpRequest::HTTP_POST) {
        if (!createNote(QUrl::fromPercentEncoding(path.at(3).toUtf8()),QString(req->body()))) {
            qWarning() << "Error creating new note";
        }
    }
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

bool Server::getWebContent(const QString &filename, QByteArray &data)
{
    QFile file(SailfishApp::pathTo("webclient/" + (filename == "" ? "index.html" : filename)).toLocalFile());

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
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return "";
    }

    QJsonArray array;

    QSqlQuery query(notesDB);
    if(query.exec("SELECT pagenr, color, body FROM notes ORDER BY pagenr")) {
        while(query.next()) {

            QString text = query.value(2).toString();
            text = text.length() <= 30 ? text : text.left(30);

            QJsonObject obj;
            obj.insert("id",query.value(0).toInt());
            obj.insert("color",query.value(1).toString());
            obj.insert("text",text);
            array.append(obj);

        }
    } else {
        qWarning() << "SQL execution error:" << query.lastError().text();
        return "";
    }

    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

QByteArray Server::getNote(int id)
{
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return "";
    }

    QSqlQuery query(notesDB);
    if(query.exec(QString("SELECT color, body FROM notes WHERE pagenr = %1").arg(id))) {
        while(query.next()) {

            QJsonObject obj;
            obj.insert("id",id);
            obj.insert("color",query.value(0).toString());
            obj.insert("text",query.value(1).toString());
            return QJsonDocument(obj).toJson(QJsonDocument::Compact);

        }
    } else {
        qWarning() << "SQL execution error:" << query.lastError().text();
    }

    return "";
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

bool Server::createNote(const QString &color, const QString &body)
{
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
    query2.addBindValue(color);
    query2.addBindValue(body);

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
}

bool Server::updateNote(int id, const QString &color, const QString &body)
{
    if (!notesDB.isOpen()) {
        qWarning() << "Notes DB is not open!";
        return false;
    }

    QSqlQuery query(notesDB);
    query.prepare("UPDATE notes SET body = ?, color = ? WHERE pagenr = ?");
    query.addBindValue(body);
    query.addBindValue(color);
    query.addBindValue(id);

    if(!query.exec()) {
        qWarning() << "SQL execution error:" << query.lastError().text();
        return false;
    }

    return true;
}

void Server::closeNotesDB()
{
    notesDB.close();
    QSqlDatabase::removeDatabase("qt_sql_sendtojolla_connection");
}

bool Server::isRunning()
{
    return isListening;
}
