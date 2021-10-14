QT = core gui

CONFIG -= app_bundle
CONFIG += console

equals(QT_MAJOR_VERSION, 4) {
	win32-msvc*: QMAKE_CXXFLAGS += /std:c++11
	else: QMAKE_CXXFLAGS += -std=c++11
}

SOURCES += main.cpp
