
################## CONFIG ########################

TARGET       = harbour-bibleme
TEMPLATE     = app

QT          += core network gui qml quick

MOC_DIR      = _moc
OBJECTS_DIR  = _obj
RCC_DIR      = _rcc

INCLUDEPATH += /usr/include/sailfishapp

############### SOURCES & CONTENT #####################

include ($$PWD/src/libQtOsisBibleEngine/QtOsisBibleEngine.pri)

SOURCES     += src/harbour-bibleme.cpp
HEADERS     +=

OTHER_FILES += \
    $$PWD/rpm/harbour-bibleme.yaml \
    $$PWD/qml/harbour-bibleme.qml \
    $$PWD/qml/pages/BookmarksPage.qml \
    $$PWD/qml/pages/SearchPage.qml \
    $$PWD/qml/pages/SelectPage.qml \
    $$PWD/qml/pages/FilesPage.qml \
    $$PWD/qml/pages/ViewPage.qml \
    $$PWD/qml/pages/SettingsPage.qml \
    $$PWD/qml/cover/CoverPage.qml \
    $$PWD/harbour-bibleme.desktop \
    $$PWD/icons/harbour-bibleme.svg \
    $$PWD/icons/86x86/harbour-bibleme.png \
    $$PWD/icons/108x108/harbour-bibleme.png \
    $$PWD/icons/128x128/harbour-bibleme.png \
    $$PWD/icons/256x256/harbour-bibleme.png

RESOURCES   += \
    $$PWD/data.qrc

################ LOCALIZATION ########################

TRANSLATIONS += \
    lang/en.ts \
    lang/fr.ts \
    lang/fi.ts \
    lang/de.ts

qt_linguist_only {
SOURCES += \
    qml/harbour-bibleme.qml \
    qml/pages/BookmarksPage.qml \
    qml/pages/SearchPage.qml \
    qml/pages/SelectPage.qml \
    qml/pages/FilesPage.qml \
    qml/pages/ViewPage.qml \
    qml/pages/SettingsPage.qml \
    qml/cover/CoverPage.qml
}

################## PACKAGING ########################

CONFIG       += link_pkgconfig
PKGCONFIG    += sailfishapp

target.files  = $${TARGET}
target.path   = /usr/bin
desktop.files = $${TARGET}.desktop
desktop.path  = /usr/share/applications
icon86.files  = $$PWD/icons/86x86/$${TARGET}.png
icon86.path   = /usr/share/icons/hicolor/86x86/apps
icon108.files = $$PWD/icons/108x108/$${TARGET}.png
icon108.path  = /usr/share/icons/hicolor/108x108/apps
icon128.files = $$PWD/icons/128x128/$${TARGET}.png
icon128.path  = /usr/share/icons/hicolor/128x128/apps
icon256.files = $$PWD/icons/256x256/$${TARGET}.png
icon256.path  = /usr/share/icons/hicolor/256x256/apps
INSTALLS     += target desktop icon86 icon108 icon128 icon256
