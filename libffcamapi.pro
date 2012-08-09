TEMPLATE = lib
TARGET = ffcamapi

CONFIG += qt warn_on debug_and_release cascades

# required libs when using this library:
# device { ARCH=arm }
# simulator { ARCH=x86 }
# LIBS += -lcamapi -lscreen -L../lib/$${ARCH} -lavcodec -lavformat -lavutil

INCLUDEPATH += ../src ../include
SOURCES += ../src/*.cpp
HEADERS += ../src/*.h ../include/*.h

device {
	CONFIG(release, debug|release) {
		DESTDIR = so.le-v7
	}
	CONFIG(debug, debug|release) {
		DESTDIR = so.le-v7-g
	}
}

simulator {
	CONFIG(release, debug|release) {
		DESTDIR = so
	}
	CONFIG(debug, debug|release) {
		DESTDIR = so-g
	}
}

OBJECTS_DIR = $${DESTDIR}/.obj
MOC_DIR = $${DESTDIR}/.moc
RCC_DIR = $${DESTDIR}/.rcc
UI_DIR = $${DESTDIR}/.ui

suredelete.target = sureclean
suredelete.commands = $(DEL_FILE) $${MOC_DIR}/*; $(DEL_FILE) $${RCC_DIR}/*; $(DEL_FILE) $${UI_DIR}/*
suredelete.depends = distclean

QMAKE_EXTRA_TARGETS += suredelete