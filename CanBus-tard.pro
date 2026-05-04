QT       += core gui serialbus multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    inc/ \

SOURCES += \
    src/process_stat.cpp \
    src/dock_send_message.cpp \
    src/dock_signal_watcher.cpp \
    src/dbc_manager.cpp \
    src/can_device.cpp \
    src/filter_manager.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    inc/dbc_parser.h \
    inc/process_stat.h \
    inc/const.h \
    inc/dock_send_message.h \
    inc/dock_signal_watcher.h \
    inc/dbc_manager.h \
    inc/can_device.h \
    inc/filter_manager.h \
    inc/mainwindow.h \
    inc/message_filter.h \
    inc/message_to_send.h

FORMS += \
    forms/dbc_manager.ui \
    forms/filter_manager.ui \
    forms/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: RC_ICONS = icon.ico

DISTFILES += \
    .clang-format \
    .gitignore \
    README.md \
    installer.iss

RESOURCES += \
    res/res.qrc
