TARGET = harbour-sendtojolla

## sailfishapp.prf ##
QT += quick qml network
target.path = /usr/bin
qml.files = qml/sailfish
qml.path = /usr/share/$${TARGET}/qml
desktop.files = $${TARGET}.desktop
desktop.path = /usr/share/applications
icon.files = $${TARGET}.png
icon.path = /usr/share/icons/hicolor/86x86/apps
INSTALLS += target qml desktop icon
CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp
INCLUDEPATH += /usr/include/sailfishapp
QMAKE_RPATHDIR += /usr/share/$${TARGET}/lib
OTHER_FILES += $$files(rpm/*) \
    rpm/harbour-sendtojolla.changes \
    qml/sailfish/FirstPage.qml
##

SOURCES += src/main.cpp \
    src/server.cpp \
    src/settings.cpp

# QHttpServer
include(qhttpserver/qhttpserver.pri)

OTHER_FILES += qml/sailfish/main.qml \
    qml/sailfish/cover/CoverPage.qml \
    qml/sailfish/pages/FirstPage.qml \
    qml/sailfish/pages/SecondPage.qml \
    rpm/harbour-sendtojolla.spec \
    rpm/harbour-sendtojolla.yaml \
    harbour-sendtojolla.desktop

HEADERS += \
    src/server.h \
    src/settings.h
