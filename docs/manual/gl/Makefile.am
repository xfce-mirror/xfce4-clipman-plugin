## Makefile.am file for the Docbook to HTML generation
##
## Copyright (C) 2006 The Xfce development team.
##
## Written by Benedikt Meurer <benny@xfce.org>, 2006.

NULL = 

SUBDIRS =								\
	images								\
	$(NULL)

TARGET_DIR = $(datadir)/xfce4/doc/gl
STYLESHEET = ../xfce-nochunk.xsl
DOCUMENT = $(PACKAGE).xml

DOC_STAMPS = html-build.stamp

EXTRA_DIST = $(DOCUMENT)
CLEANFILES = $(DOC_STAMPS)

if ENABLE_XSLTPROC
all-local: html-build.stamp

html-build.stamp: $(srcdir)/$(DOCUMENT) $(srcdir)/$(STYLESHEET)
	@echo "*** Building HTML ***"
	@-chmod -R u+w $(srcdir)
	$(XSLTPROC) --nonet -o $(srcdir)/ $(srcdir)/$(STYLESHEET) $(srcdir)/$(DOCUMENT)
	touch html-build.stamp
else
all-local:
endif

install-data-local:
	installfiles=`echo $(srcdir)/*.html`;				\
	if test "$$installfiles" = '$(srcdir)/*.html'; then		\
		echo "--- Nothing to install";				\
	else								\
		$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR);		\
		for file in $$installfiles; do				\
			echo "--- Installing "$$file;			\
			$(INSTALL_DATA) $$file $(DESTDIR)$(TARGET_DIR);	\
		done;							\
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(TARGET_DIR)/*

if ENABLE_XSLTPROC
dist-check-xsltproc: all
else
dist-check-xsltproc:
	@echo "*** xsltproc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-xsltproc dist-hook-local
	-cp $(srcdir)/*.html $(distdir)

.PHONY: dist-hook-local

