INCLUDEPATH += $$PWD/..

CONFIG +=c++11

SHADOW = $$shadowed($$PWD)

SHADOW_AXQ = $$replace(SHADOW, build-$$TARGET, build-Axq)
SHADOW_AXQ = $$replace(SHADOW_AXQ, $$TARGET/builds, AxqLib/builds)
SHADOW_AXQ = $${SHADOW_AXQ}/lib

!exists($$SHADOW_AXQ):error(AxqLib (in $$SHADOW_AXQ) is expected to be found next to this project)

win32:CONFIG(release, debug|release): LIBS += -L$${SHADOW_AXQ}/release -lAxq
else:win32:CONFIG(debug, debug|release): LIBS += -L$${SHADOW_AXQ}/debug -lAxq
else:unix:!macx: LIBS += -L$${SHADOW_AXQ} -lAxq
else:macx: LIBS += -L$${SHADOW_AXQ} -lAxq
else: error(Not defined)

INCLUDEPATH += \
            ../AxqLib/libs \
            ../AxqLib

DEPENDPATH += ../AxqLib/lib

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $${SHADOW_AXQ}/release/Axq.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $${SHADOW_AXQ}/debug/Axq.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $${SHADOW_AXQ}/release/Axq.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $${SHADOW_AXQ}/debug/Axq.lib
else:unix:!macx: PRE_TARGETDEPS += $${SHADOW_AXQ}/libAxq.so
else:macx:PRE_TARGETDEPS += $${SHADOW_AXQ}/libAxq.dylib
else: error(Not defined)


win32:!win32-g++:DEFINES*=NOMINMAX #MSVC problem with std::min/max

QMAKE_RPATHDIR += $${SHADOW_AXQ}
