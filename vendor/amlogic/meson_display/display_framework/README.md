Meson display framework for linux.

Dependencies
============

1. libdrm for drm-extension and weston-extension
2. json-c is need for weston-extension

How to build
============

1. Build without modify Makefile.am/configure.ac

      ./configure
	  make
	  make install

2. Build after modify Makefile.am/configure.ac

       autoreconf --install
	   ./configure
	   make
	   make install

More detailed build instructions
================================

1. Configure for package
   1. enable debug

         ./configure --enable-debug

