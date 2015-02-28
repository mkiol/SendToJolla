INCLUDEPATH += ./qhttpserver
DEPENDPATH  += ./qhttpserver

HEADERS += \
    http_parser.h \
    qhttpconnection.h \
    qhttprequest.h \
    qhttpresponse.h \
    qhttpserver.h \
    qhttpserverapi.h \
    qhttpserverfwd.h

SOURCES += \
    http_parser.cpp \
    qhttpconnection.cpp \
    qhttprequest.cpp \
    qhttpresponse.cpp \
    qhttpserver.cpp