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
#include <QTextStream>

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
    QStringList path = req->url().path().split("/");

    Settings* s = Settings::instance();
    if (path.length() > 2 && path.at(1) == s->getCookie()) {

        // Redirection server_url/cookie -> server_url/cookie/
        if (path.length() == 2) {
            resp->setHeader("Location", s->getCookie() + "/");
            sendResponse(req, resp, 301);
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

        if (path.length() > 4 && path.at(2) == "update-note" && req->method() == QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForUpdateNote()));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "create-note" && req->method() == QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForCreateNote()));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 2 && path.at(2)=="create-bookmark" && req->method()==QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForCreateBookmark()));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
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
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForUpdateBookmark()));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 3 && path.at(2) == "open-url") {
            launchBrowser(path.at(3));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        if (path.length() > 2 && path.at(2) == "set-clipboard" && req->method() == QHttpRequest::HTTP_POST) {
            connect(req, SIGNAL(end()), this, SLOT(bodyReceivedForSetClipboard()));
            sendResponse(req, resp);
            emit newEvent(QString("%1 request from %2").arg(path.at(2)).arg(req->remoteAddress()));
            return;
        }

        // Web client

        if (path.length() == 3) {

            QByteArray data;
            QString filename = path.at(2);

            if (getWebContent(filename, data)) {

                // Get extension
                QStringList l = filename.split(".");
                QString ext = "";
                if (l.length()>0)
                    ext = l[l.length()-1];
                QString contentType = ext == "css" ? "text/css; charset=utf-8" :
                                      ext == "gif" ? "image/gif" :
                                      ext == "png" ? "image/png" :
                                      ext == "js" ? "application/x-javascript; charset=utf-8" :
                                                    "text/html; charset=utf-8";

                sendResponse(req, resp, 200, contentType, data);
                emit newEvent(QString("Web client request from %1").arg(req->remoteAddress()));
                return;
            }

            sendResponse(req, resp, 404);
            emit newEvent(tr("%1 request from %2").arg(filename, req->remoteAddress()));
            return;
        }

        sendResponse(req, resp, 400);
        emit newEvent(tr("Unknown request from %1").arg(req->remoteAddress()));
        return;
    }

    sendResponse(req, resp, 400);
    emit newEvent(tr("Unknown request from %1").arg(req->remoteAddress()));
}

void Server::sendResponse(QHttpRequest *req, QHttpResponse *resp, int status, const QString &contentType, const QByteArray &data)
{
    req->storeBody();
    if (status == 204 || data.isEmpty()) {
        resp->setHeader("Content-Length", "0");
    } else {
        resp->setHeader("Content-Type", contentType);
        resp->setHeader("Content-Length", QString::number(data.size()));
    }
    resp->setHeader("Connection", "close");
    resp->writeHead(status);
    resp->end(data);
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
    if (path.length() > 3 && path.at(2) == "create-note" && req->method()==QHttpRequest::HTTP_POST) {
        if (!createNote(QUrl::fromPercentEncoding(path.at(3).toUtf8()),QString(req->body()))) {
            qWarning() << "Error creating new note";
        }
    }
}

void Server::bodyReceivedForCreateBookmark() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());

    QStringList path = req->url().path().split("/");
    if (path.length() > 2 && path.at(2) == "create-bookmark" && req->method()==QHttpRequest::HTTP_POST) {
        if (!createBookmark(req->body())) {
            qWarning() << "Error creating new bookmark";
        }
    }
}

void Server::bodyReceivedForUpdateBookmark() {
    QHttpRequest* req = dynamic_cast<QHttpRequest*>(sender());

    QStringList path = req->url().path().split("/");
    if (path.length() > 3 && path.at(2) == "update-bookmark" && req->method()==QHttpRequest::HTTP_POST) {
        if (!updateBookmark(path.at(3).toInt(), req->body())) {
            qWarning() << "Error updating bookmark";
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
            text = text.length() <= 50 ? text : text.left(50);

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
            obj.insert("id",QJsonValue(id));
            obj.insert("color",query.value(0).toString());
            obj.insert("text",query.value(1).toString());
            return QJsonDocument(obj).toJson(QJsonDocument::Compact);

        }
    } else {
        qWarning() << "SQL execution error:" << query.lastError().text();
    }

    return "";
}

bool Server::createBookmark(const QByteArray &json)
{
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

    if (obj["url"].isUndefined() || obj["url"].toString().isEmpty() ||
        obj["title"].isUndefined() || obj["title"].toString().isEmpty()) {
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
}

bool Server::updateBookmark(int id, const QByteArray &json)
{
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
}

bool Server::deleteBookmark(int id)
{
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
}

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

QByteArray Server::getBookmarks()
{
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

bool Server::deleteNote(int id)
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
