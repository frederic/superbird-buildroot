#-------------------------------------------------
#
# Project created by QtCreator 2013-07-01T18:28:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sampleapp
TEMPLATE = app

RESOURCES     = sampleapp.qrc

SOURCES += main.cpp\
           bsa.cpp \
           app_manager.c \
           hfactions.cpp \
           xmlparser.cpp \
           ble.cpp \          
            ../../app_ble/source/app_ble_client.c \
            ../../app_ble/source/app_ble_server.c \
            app_disc_ble.c \
            ../../app_common/source/app_disc.c\
           ../../app_common/source/nanoxml.c \
           ../../app_common/source/app_xml_utils.c \
           ../../app_common/source/app_xml_param.c \
           ../../app_common/source/app_utils.c \
           ../../app_common/source/app_services.c \
           ../../app_common/source/app_mgt.c \
           ../../app_common/source/app_dm.c \
           ../../app_common/source/app_wav.c \
           ../../app_common/source/app_thread.c \
           ../../app_common/source/app_mutex.c \
           ../../app_common/source/app_playlist.c \
           ../../app_common/source/app_alsa.c \
           ../../app_avk/source/app_avk.c \
           ../../app_hs/source/app_hs.c \
           ../../app_av/source/app_av.c \
           ../../app_ag/source/app_ag.c \
           ../../app_pbc/source/app_pbc.c \
           ../../app_mce/source/app_mce.c \
            ../../app_ble/source/app_ble.c \
            ../../app_ble/source/app_ble_client_xml.c \
            ../../app_ble/source/app_ble_client_db.c \
    blerwinfodlg.cpp \
    app_ble_qt.c

HEADERS  += bsa.h \
            data_types.h \
            bsa_extern.h \
            cmnhf.h \
            hfactions.h \
            xmlparser.h \
            ble.h \
            ../../app_ble/include/app_ble_client.h \
            ../../app_avk/include/app_avk.h \
            ../../app_hs/include/app_hs.h \
            ../../app_av/include/app_av.h \
            ../../app_ag/include/app_ag.h \
            ../../app_pbc/source/app_pbc.h \
            ../../app_mce/source/app_mce.h \
            ../../app_common/include/app_alsa.h \
            ../../app_common/include/app_wav.h \
            ../../app_common/include/app_sec.h \
    blerwinfodlg.h \
    app_ble_qt.h


FORMS    += bsa.ui \
    blerwinfodlg.ui

LIBS += -L$$PWD/../../../../bsa_examples/linux/libbsa/build/$$BSA_QT_ARCH/sharedlib/ -lbsa
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/qt_app/sampleapp
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_common/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/libbsa/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_ag/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_av/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_avk/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_hs/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_pbc/source
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_mce/source
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_ble/include

LIBS += -lasound -lrt


QT += xml

LIBS += -L$$OUT_PWD/../libvcard/ -lvcard
INCLUDEPATH += $$PWD/../libvcard
INCLUDEPATH += $$PWD/../libvcard/include/vcard
DEPENDPATH += $$PWD/../libvcard

LIBS += -L$$OUT_PWD/../libbmsg/ -lbmsg
INCLUDEPATH += $$PWD/../libbmsg
INCLUDEPATH += $$PWD/../libbmsg/include/bmsg
DEPENDPATH += $$PWD/../libbmsg

DEFINES += QT_APP
DEFINES += PCM_ALSA
DEFINES += BLE_INCLUDED=TRUE
DEFINES += BTA_GATT_INCLUDED=TRUE

unix: {
    # install library and headers
    isEmpty(PREFIX) {
      PREFIX = /usr/local
    }
    target.path = $$PREFIX/bin
    INSTALLS += target
}
