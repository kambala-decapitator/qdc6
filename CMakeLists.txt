cmake_minimum_required(VERSION 3.14)

project(qdc6
	VERSION 1.1
	LANGUAGES CXX
)

option(SVG "Build with SVG support" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
if(SVG)
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Svg REQUIRED)
	add_compile_definitions(SVG_ENABLED=1)
endif()

add_executable(qdc6
	main.cpp
	resources.qrc
)
target_link_libraries(qdc6 PRIVATE Qt${QT_VERSION_MAJOR}::Gui)
if(SVG)
	target_link_libraries(qdc6 PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
endif()
