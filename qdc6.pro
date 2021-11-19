QT = core gui

CONFIG -= app_bundle
CONFIG += console

CONFIG(svg) {
	QT += svg
	DEFINES += SVG_ENABLED=1
}

equals(QT_MAJOR_VERSION, 4) {
	win32-msvc*: QMAKE_CXXFLAGS += /std:c++11
	else: QMAKE_CXXFLAGS += -std=c++11
}

SOURCES += main.cpp
RESOURCES += resources.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
