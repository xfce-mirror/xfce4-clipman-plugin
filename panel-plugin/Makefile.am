NULL = 

INCLUDES =								\
	-I${top_srcdir}							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATAROOTDIR=\"$(datarootdir)\"				\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

#
# Maintainer Mode
#

if MAINTAINER_MODE
BUILT_SOURCES = settings-dialog_glade.h
settings-dialog_glade.h: settings-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=settings_dialog_glade $< >$@
endif

#
# Popup Command
#

bin_PROGRAMS = xfce4-clipman-plugin xfce4-popup-clipman xfce4-clipman-settings

xfce4_popup_clipman_SOURCES =						\
	xfce4-popup-clipman.c						\
	common.h							\
	$(NULL)

xfce4_popup_clipman_CFLAGS =                                            \
	@LIBX11_CFLAGS@							\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	$(NULL)

xfce4_popup_clipman_LDADD =						\
	@LIBX11_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	$(NULL)

#
# Settings dialog
#

xfce4_clipman_settings_SOURCES =					\
	xfce4-clipman-settings.c					\
	common.h							\
	settings-dialog_glade.h						\
	actions.c			actions.h			\
	$(NULL)

xfce4_clipman_settings_CFLAGS =						\
	@GIO_CFLAGS@							\
	@EXO_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	@GLADE_CFLAGS@							\
	$(NULL)

xfce4_clipman_settings_LDADD =						\
	@GIO_LIBS@							\
	@EXO_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCEGUI4_LIBS@						\
	@XFCONF_LIBS@							\
	@GLADE_LIBS@							\
	$(NULL)


#
# Plugin
#

xfce4_clipman_plugin_SOURCES =						\
	main.c								\
	common.h							\
	actions.c			actions.h			\
	collector.c			collector.h			\
	history.c			history.h			\
	menu.c				menu.h				\
	$(NULL)

xfce4_clipman_plugin_CFLAGS =						\
	@GIO_CFLAGS@							\
	@EXO_CFLAGS@							\
	@LIBX11_CFLAGS@							\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4PANEL_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_plugin_LDADD =						\
	@GIO_LIBS@							\
	@EXO_LIBS@							\
	@LIBX11_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4PANEL_LIBS@						\
	@XFCONF_LIBS@							\
	$(NULL)

#
# XML Actions File
#

actionsdir = $(sysconfdir)/xdg/xfce4/panel
actions_in_files = xfce4-clipman-actions.xml.in
actions_DATA = $(actions_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

#
# Desktop Entry File
#

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = xfce4-clipman-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

%.desktop.in: %.desktop.in.in
	sed -e "s^@EXTERNAL_PLUGIN_PATH@^$(bindir)^" \
		$< > $@

#
# Autostart File
#

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = xfce4-clipman-plugin-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

#
# Extra
#

EXTRA_DIST =								\
	settings-dialog.glade						\
	$(actions_in_files)						\
	$(autostart_in_files)						\
	$(desktop_in_in_files)						\
	$(NULL)

CLEANFILES =								\
	$(actions_DATA)							\
	$(autostart_DATA)						\
	$(desktop_DATA)							\
	$(NULL)

