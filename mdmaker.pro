QT += quick
CONFIG += c++11
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG+=webview

!webengine:!webview: CONFIG += webview

webview {
    QT += webview
    DEFINES += WEBVIEW
} else {
    QT += webengine
}

HEADERS += \
    markdownmaker.h
SOURCES += main.cpp \
    markdownmaker.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

include(Axq.pri)
INCLUDEPATH += ../AxqLib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DISTFILES += \
    index.html \
    markdown.css \
    releases/macdeploy.sh \
    releases/linuxdeploy.sh \
    html/marked.js \
    html/darth.js \
    html/darth.jpg \
    html/markdown.css \
    html/index.html
