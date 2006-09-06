# $Id$

plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-clipman-plugin

xfce4_clipman_plugin_SOURCES =						\
	clipman.h							\
	clipman.c							\
	clipman-dialogs.h						\
	clipman-dialogs.c

xfce4_clipman_plugin_CFLAGS =						\
	-I$(top_srcdir)							\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_clipman_plugin_LDADD =						\
	$(LIBXFCE4PANEL_LIBS)						\
	$(LIBXFCEGUI4_LIBS)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = clipman.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
%.desktop.in: %.desktop.in.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	$(desktop_in_in_files)

CLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)
