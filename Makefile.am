#
# Makefile.am for libstorage
#

SUBDIRS = storage doc bindings data po examples testsuite testsuite-real

AUTOMAKE_OPTIONS = foreign dist-bzip2 no-dist-gzip

doc_DATA = AUTHORS COPYING

EXTRA_DIST = $(doc_DATA) VERSION LIBVERSION


libstorage-$(VERSION).tar.bz2: dist-bzip2

package-local: libstorage-$(VERSION).tar.bz2
	rm -f package/libstorage-*.tar.bz2
	rm -f package/*~
	rm -f package/*.bak
	mv libstorage-$(VERSION).tar.bz2 package/

package: package-local

