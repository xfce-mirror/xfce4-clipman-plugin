INCLUDES =								\
	-I$(top_srcdir)							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"



bin_PROGRAMS = xfce4-popup-clipman

xfce4_popup_clipman_SOURCES =						\
	xfce4-popup-clipman.h						\
	xfce4-popup-clipman.c

xfce4_popup_clipman_CFLAGS =						\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@

xfce4_popup_clipman_LDADD =						\
	@LIBX11_LDFLAGS@						\
	@GTK_LIBS@



plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-clipman-plugin

xfce4_clipman_plugin_SOURCES =						\
	clipman-dialogs.h						\
	clipman-dialogs.c						\
	clipman.h							\
	clipman.c

xfce4_clipman_plugin_CFLAGS =						\
	@LIBXFCE4PANEL_CFLAGS@						\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBXFCEGUI4_CFLAGS@

xfce4_clipman_plugin_LDADD =						\
	@LIBXFCE4PANEL_LIBS@						\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCEGUI4_LIBS@



desktop_in_in_files = clipman.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

EXTRA_DIST =								\
	$(desktop_in_in_files)

CLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
