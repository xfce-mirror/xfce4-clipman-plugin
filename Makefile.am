ACLOCAL_AMFLAGS = -I m4  ${ACLOCAL_FLAGS}

NULL = 

SUBDIRS =								\
	data								\
	clipboard-manager								\
	panel-plugin							\
	tests								\
	po								\
	$(NULL)

distclean-local:
	rm -rf *.cache *~

EXTRA_DIST =								\
	AUTHORS								\
	COPYING								\
	NEWS								\
	README.md								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(NULL)

CLEANFILES =								\
	$(NULL)

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	po/.intltool-merge-cache.lock			\
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
