#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include <QObject>

class ProxyClient : public QObject
{
    Q_OBJECT
public:
    explicit ProxyClient(QObject *parent = 0);

signals:

public slots:

};

#endif // PROXYCLIENT_H
