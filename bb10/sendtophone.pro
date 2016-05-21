APP_NAME = sendtophone

CONFIG += qt warn_on cascades10
QT += network
LIBS += -lbbsystem -lbbplatform -lbbdevice -lbbpim

DEFINES += BB10
DEFINES += CONTACTS

TRANSLATIONS = $${TARGET}.ts

include(QJson/json.pri)

include(../qhttpserver/qhttpserver.pri)

include(config.pri)
