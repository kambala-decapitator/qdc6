QtGuiApplication {
	property bool svg: false

	consoleApplication: true
	files: [
		"main.cpp",
		"resources.qrc",
	]

	cpp.cxxLanguageVersion: "c++11"
	cpp.defines: [
		"QT_DISABLE_DEPRECATED_BEFORE=0x060000",
	]

	Depends {
		condition: svg
		name: "Qt.svg"
	}
	Properties {
		condition: svg
		cpp.defines: base.concat(["SVG_ENABLED=1"])
	}
}
