QT += quick webview
CONFIG += c++14
DEFINES += QT_DEPRECATED_WARNINGS

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    index.html \
    markdown.css
