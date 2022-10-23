#-------------------------------------------------
#
# Project created by QtCreator 2013-07-01T18:28:57
#
#-------------------------------------------------


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets



TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += libvcard \
	     libbmsg \
           sampleapp


sampleapp.depends  = libvcard
sampleapp.depends  = libbmsg

