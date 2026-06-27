QT       += core gui \
    network

greaterThan(QT_MAJOR_VERSION, 4): QT += core gui widgets openglwidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    conn_settings.cpp \
    main.cpp \
    mainwindow.cpp \
    motion_controller/PtzControlWidget.cpp \
    motion_controller/motion_controller.cpp \
    rtspreceiver.cpp \
    target_escort.cpp \
    videowidget.cpp

HEADERS += \
    UDPReceiver.h \
    UDPSender.h \
    conn_settings.h \
    func_and_structure.h \
    mainwindow.h \
    motion_controller/PtzControlWidget.h \
    motion_controller/motion_controller.h \
    rtspreceiver.h \
    target_escort.h \
    videowidget.h

FORMS += \
    conn_settings.ui \
    mainwindow.ui \
    motion_controller/motion_controller.ui

INCLUDEPATH += C:/Dev_department/gstreamer/msvc_x86_64/include/gstreamer-1.0
INCLUDEPATH += C:/Dev_department/gstreamer/msvc_x86_64/include/glib-2.0
INCLUDEPATH += C:/Dev_department/gstreamer/msvc_x86_64/lib/glib-2.0/include


LIBS += -LC:/Dev_department/gstreamer/msvc_x86_64/lib
LIBS += -LC:/Dev_department/gstreamer/msvc_x86_64/bin
#LIBS += -LC:/Dev_department/RTSP_View/RTSP_View
LIBS += -lgstreamer-1.0
LIBS += -lgstrtp-1.0
LIBS += -lgstapp-1.0
LIBS += -lglib-2.0
LIBS += -lgobject-2.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    img.qrc
