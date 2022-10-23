##
# This file is part of the libbmsg project.
#
# Copyright (C) 2013, Broadcom Corporation
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
##

VERSION=1.0.0
BMSG_INCDIR = include/bmsg
BMSG_SRCDIR = libbmsg

PUBLIC_HEADERS = $$BMSG_INCDIR/libbmsg_global.h

QT -= gui
TARGET = bmsg
TEMPLATE = lib
DEFINES += BMSG_LIBRARY
INCLUDEPATH += $$BMSG_INCDIR
HEADERS += $$PUBLIC_HEADERS \
    include/bmsg/bta_ma_util.h \
    include/bmsg/bta_ma_def.h \
    include/bmsg/bta_ma_co.h \
    include/bmsg/bta_ma_api.h \
    include/bmsg/BMessageManager.h \
    include/bmsg/BMessageConstants.h
SOURCES += libbmsg/bta_ma_api.c \
    libbmsg/bta_ma_co.c \
    libbmsg/bta_ma_util.c \
    libbmsg/BMessageManager.cpp \
    libbmsg/app_utils.c

DESTDIR = $$BMSG_LIBDIR

INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/app_common/include
INCLUDEPATH += ../../../../../../3rdparty/embedded/bsa_examples/linux/libbsa/include



unix: {
    # install library and headers
    isEmpty(PREFIX) {
      PREFIX = /usr/local
    }
    target.path = $$PREFIX/lib
    INSTALLS += target

    incfiles.path = $$PREFIX/include/bmsg
    incfiles.files = $$PUBLIC_HEADERS
    INSTALLS += incfiles

    # install pkg-config file (libbmsg.pc)
    CONFIG += create_pc create_prl
    QMAKE_PKGCONFIG_REQUIRES = QtCore
    QMAKE_PKGCONFIG_LIBDIR = $$target.path
    QMAKE_PKGCONFIG_INCDIR = $$incfiles.path
    QMAKE_PKGCONFIG_DESTDIR = pkgconfig
}
