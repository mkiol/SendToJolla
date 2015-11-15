TARGET = harbour-sendtojolla

#SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256
SAILFISHAPP_ICONS = 86x86

DEFINES += SAILFISH CONTACTS
PKGCONFIG += Qt5Contacts

## sailfishapp.prf ##
QT += quick qml network sql
target.path = /usr/bin
qml.files = qml/sailfish
qml.path = /usr/share/$${TARGET}/qml
desktop.files = $${TARGET}.desktop
desktop.path = /usr/share/applications
!isEmpty(SAILFISHAPP_ICONS):for(size, SAILFISHAPP_ICONS) {
    icon$${size}.files = icons/$${size}/$${TARGET}.png
    icon$${size}.path = /usr/share/icons/hicolor/$${size}/apps
    INSTALLS += icon$${size}
} else {
    icon.files = $${TARGET}.png
    icon.path = /usr/share/icons/hicolor/86x86/apps
    INSTALLS += icon
}
INSTALLS += target qml desktop
CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp
INCLUDEPATH += /usr/include/sailfishapp
QMAKE_RPATHDIR += /usr/share/$${TARGET}/lib
OTHER_FILES += $$files(rpm/*)
##

# Web client
webclient.path = /usr/share/$${TARGET}
webclient.files = webclient
INSTALLS += webclient

SOURCES += src/main_sailfish.cpp \
    src/server.cpp \
    src/settings.cpp \
    src/utils.cpp \
    src/proxyclient.cpp

# QHttpServer
include(qhttpserver/qhttpserver.pri)

OTHER_FILES += qml/sailfish/main.qml \
    qml/sailfish/cover/CoverPage.qml \
    qml/sailfish/pages/FirstPage.qml \
    qml/sailfish/pages/SecondPage.qml \
    rpm/harbour-sendtojolla.spec \
    rpm/harbour-sendtojolla.yaml \
    rpm/harbour-sendtojolla.changes \
    harbour-sendtojolla.desktop

HEADERS += \
    src/server.h \
    src/settings.h \
    src/utils.h \
    src/proxyclient.h
