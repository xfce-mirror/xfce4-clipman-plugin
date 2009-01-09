NULL = 

INCLUDES =								\
	-I${top_srcdir}							\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-clipman-plugin

xfce4_clipman_plugin_SOURCES =						\
	panel-plugin.c							\
	collector.c			collector.h			\
	history.c			history.h			\
	menu.c				menu.h				\
	$(NULL)

xfce4_clipman_plugin_CFLAGS =						\
	@GTK_CFLAGS@							\
	@EXO_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4PANEL_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_plugin_LDADD =						\
	@GTK_LIBS@							\
	@EXO_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4PANEL_LIBS@						\
	@XFCONF_LIBS@							\
	$(NULL)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = xfce4-clipman-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

%.desktop.in: %.desktop.in.in
	sed -e "s^@EXTERNAL_PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		$< > $@

CLEANFILES =								\
	$(desktop_DATA)							\
	$(NULL)

