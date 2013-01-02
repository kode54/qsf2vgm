TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = qsf2vgm

LIBS += -L$$OUT_PWD/../psflib/ \
        -L$$OUT_PWD/../QSoundCore/Core/

LIBS += -lQSoundCore -lpsflib -lz

DEPENDPATH += $$PWD/../psflib \
              $$PWD/../QSoundCore/Core

PRE_TARGETDEPS += $$OUT_PWD/../psflib/libpsflib.a \
                  $$OUT_PWD/../QSoundCore/Core/libQSoundCore.a

INCLUDEPATH += ../psflib

SOURCES += main.c

