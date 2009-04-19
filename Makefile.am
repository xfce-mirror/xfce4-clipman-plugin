NULL = 

SUBDIRS =								\
	icons								\
	panel-plugin							\
	tests								\
	po								\
	po-doc								\
	docs								\
	$(NULL)

distclean-local:
	rm -rf *.cache *~

desktopdir = $(datadir)/applications
desktop_in_files = xfce4-clipman-plugin.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCHECK_CONFIGURE_FLAGS =						\
	--enable-xml2po							\
	--enable-xsltproc						\
	$(NULL)

EXTRA_DIST =								\
	AUTHORS								\
	ChangeLog							\
	COPYING								\
	INSTALL								\
	NEWS								\
	README								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(desktop_in_files)						\
	$(NULL)

CLEANFILES =								\
	$(desktop_DATA)							\
	$(NULL)

DISTCLEANFILES =							\
	aclocal.m4							\
	compile								\
	config.guess							\
	config.h.in							\
	config.sub							\
	configure							\
	configure.ac							\
	depcomp								\
	install-sh							\
	ltmain.sh							\
	missing								\
	mkinstalldirs							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
