ACLOCAL_AMFLAGS = -I m4  ${ACLOCAL_FLAGS}

NULL = 

SUBDIRS =								\
	protocols								\
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
	meson.build							\
	meson_options.txt						\
	po/meson.build							\
	xfce-revision.h.in						\
	$(NULL)

CLEANFILES =								\
	$(NULL)

DISTCLEANFILES =							\
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
