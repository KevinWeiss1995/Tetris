QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tetris
TEMPLATE = app

SOURCES += main.cpp \
           tetrisboard.cpp

HEADERS += tetrisboard.h