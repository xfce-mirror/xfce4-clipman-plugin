plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-clipman-plugin

xfce4_clipman_plugin_SOURCES = \
	clipman.h \
	clipman.c \
	clipman-dialogs.h \
	clipman-dialogs.c

xfce4_clipman_plugin_CFLAGS =	 \
	-I$(top_srcdir) \
	@LIBXFCEGUI4_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_clipman_plugin_LDFLAGS = \
	@LIBXFCE4PANEL_LIBS@ \
	@LIBXFCEGUI4_LIBS@

desktop_in_in_files = clipman.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 				\
	$(desktop_in_in_files)

DISTCLEANFILES =			\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@EXTERNAL_PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		$< > $@
