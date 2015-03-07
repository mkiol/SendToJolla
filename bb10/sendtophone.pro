APP_NAME = sendtophone

CONFIG += qt warn_on cascades10
QT += network
LIBS += -lbbsystem -lbbplatform -lbbdevice

DEFINES += BB10

TRANSLATIONS = $${TARGET}.ts

include(../qhttpserver/qhttpserver.pri)

include(config.pri)
