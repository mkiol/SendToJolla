TARGET = harbour-sendtojolla

CONFIG += sailfishapp

DEFINES += SAILFISH
DEFINES += CONTACTS
PKGCONFIG += Qt5Contacts

QT += sql

# QHttpServer
include(qhttpserver/qhttpserver.pri)

HEADERS += \
    src/server.h \
    src/settings.h \
    src/utils.h \
    src/proxyclient.h

SOURCES += src/main.cpp \
    src/server.cpp \
    src/settings.cpp \
    src/utils.cpp \
    src/proxyclient.cpp

OTHER_FILES += qml/*.qml \
    rpm/harbour-sendtojolla.spec \
    rpm/harbour-sendtojolla.yaml \
    rpm/harbour-sendtojolla.changes \
    harbour-sendtojolla.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

TRANSLATIONS += translations/sendtojolla-en.ts \
                translations/sendtojolla-pl.ts

# Web client
webclient.path = /usr/share/$${TARGET}
webclient.files = webclient
INSTALLS += webclient

# Translations
translations.files = translations
translations.path = /usr/share/$${TARGET}
INSTALLS += translations



