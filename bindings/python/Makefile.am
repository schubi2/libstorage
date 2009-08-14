#
# Makefile.am for libstorage/bindings/python
#

EXTRA_DIST = libstorage.i

AM_CXXFLAGS = -fPIC -DPIC

PYLIBVER ?= python$(PYTHON_VERSION)

INCLUDES = -I${top_srcdir} -I/usr/include/$(PYLIBVER)

LIBS = $(top_builddir)/storage/libstorage.la

python_PYTHON = libstorage.py

pyexec_LTLIBRARIES = _libstorage.la

_libstorage_la_LDFLAGS = -module -avoid-version
_libstorage_la_HEADERS: $(top_builddir)/config.h

_libstorage_la_DEPENDENCIES =				\
	${top_srcdir}/storage/StorageInterface.h	\
	${top_srcdir}/storage/HumanString.h		\
	${top_srcdir}/storage/Graph.h			\
	${top_builddir}/storage/libstorage.la

nodist__libstorage_la_SOURCES  = libstorage_wrap.cxx

libstorage.py libstorage_wrap.cxx: libstorage.i
	swig -o libstorage_wrap.cxx -c++ -python ${INCLUDES} libstorage.i

CLEANFILES = libstorage.py libstorage_wrap.cxx