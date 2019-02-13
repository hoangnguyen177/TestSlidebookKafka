QT += core
QT -= gui

# QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -std=c++0x -Wcpp
TARGET = TestSlidebookKafka
SUBDIRS += lib
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    capturedataframe.cpp \
    slidebook.cpp \
    largemessagesegment.cpp \
    segmentserializer.cpp

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib/ -lrdkafka++ -lSlideBook6Reader -ljsoncpp -lfmt

HEADERS += \
    capturedataframe.h \
    slidebook.h \
    largemessagesegment.h \
    segmentserializer.h

DISTFILES += \
    README.txt

