QT += core
QT -= gui

CONFIG += c++11

TARGET = SHALoader
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    project.cpp

HEADERS += \
    project.h
