# vim: set ft=make ts=8 :

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

INCLUDES = -I src

extractordir = $(libdir)/tracker-$(TRACKER_API_VERSION)/extract-modules
extractor_LTLIBRARIES = src/tracker-extract/libextract-mime-message.la

src_tracker_extract_libextract_mime_message_la_SOURCES = src/tracker-extract/tracker-extract-mime-message.c
src_tracker_extract_libextract_mime_message_la_LDFLAGS = -module -avoid-version -no-undefined
src_tracker_extract_libextract_mime_message_la_CFLAGS = $(GMIME_CFLAGS) $(TRACKER_CFLAGS)
src_tracker_extract_libextract_mime_message_la_LIBADD = $(GMIME_LIBS) $(TRACKER_LIBS)

MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in

