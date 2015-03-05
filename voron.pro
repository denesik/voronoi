TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    image.cpp \
    Voronoi.cpp \
    geometry.cpp \
    lodepng/lodepng.cpp \
    Lloyd.cpp

HEADERS += \
    image.h \
    Voronoi.h \
    geometry.h \
    lodepng/lodepng.h \
    Lloyd.h \
    gif-h/gif.h

