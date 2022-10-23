AUTOMAKE_OPTIONS = subdir-objects
noinst_PROGRAMS = ipc_test
noinst_LTLIBRARIES = libjsonipc.la

AM_CFLAGS = $(JSONC_CFLAGS)
AM_CPPFLAGS = $(JSONC_CFLAGS)

if DEBUG
AM_CFLAGS += -DDEBUG=1
AM_CPPFLAGS += -DDEBUG=1
endif

libjsonipc_la_LIBADD = $(JSONC_LIBS)
libjsonipc_la_LDFLAGS = -pthread
libjsonipc_la_SOURCES = ipc.c ipc.h
libjsonipc_ladir = $(includedir)/ipc
libjsonipc_la_HEADERS = ipc.h

ipc_test_CFLAGS = -DDEBUG
ipc_test_SOURCES = test/test_main.c
ipc_test_LDADD = libjsonipc.la
