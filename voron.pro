TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    lodepng/lodepng.cpp \
    image.cpp \
    Voronoi.cpp \
    geometry.cpp

HEADERS += \
    lodepng/lodepng.h \
    image.h \
    Voronoi.h \
    geometry.h

