NULL =

AM_CPPFLAGS =								\
	-I${top_srcdir}							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATAROOTDIR=\"$(datarootdir)\"				\
	-DDATADIR=\"$(datadir)\"					\
	-DDOCDIR=\"$(docdir)\"						\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(NULL)

bin_PROGRAMS = xfce4-popup-clipman xfce4-clipman-settings xfce4-clipman-history

if HAVE_LIBX11
bin_PROGRAMS += xfce4-clipman
endif

#
# Maintainer Mode
#

if MAINTAINER_MODE
BUILT_SOURCES = settings-dialog_ui.h
settings-dialog_ui.h: settings-dialog.ui
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=settings_dialog_ui $< >$@
endif

#
# Popup Command
#

xfce4_popup_clipman_SOURCES =						\
	xfce4-popup-clipman.c						\
	common.c							\
	common.h							\
	$(NULL)

xfce4_popup_clipman_CFLAGS =                                            \
	-DGSEAL_ENABLE							\
	@LIBX11_CFLAGS@							\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	$(NULL)

xfce4_popup_clipman_LDADD =						\
	@LIBX11_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	$(NULL)

#
# History Command
#

xfce4_clipman_history_SOURCES =						\
	xfce4-clipman-history.c						\
	actions.c							\
	common.c							\
	common.h							\
	collector.c							\
	history.c							\
	menu.c							\
	plugin.c							\
	$(NULL)

xfce4_clipman_history_CFLAGS =                                            \
	-DGSEAL_ENABLE							\
	@LIBX11_CFLAGS@							\
	@LIBXTST_CFLAGS@						\
	@GIO_CFLAGS@							\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@					\
	@LIBXFCE4UI_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_history_LDADD =						\
	$(top_builddir)/clipboard-manager/libdaemon.la							\
	@LIBX11_LIBS@							\
	@LIBXTST_LIBS@							\
	@GIO_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCE4UI_LIBS@						\
	@XFCONF_LIBS@							\
	@QRENCODE_LIBS@							\
	$(NULL)

#
# Settings dialog
#

xfce4_clipman_settings_SOURCES =					\
	xfce4-clipman-settings.c					\
	common.h							\
	settings-dialog_ui.h						\
	actions.c			actions.h			\
	$(NULL)

xfce4_clipman_settings_CFLAGS =						\
	-DGSEAL_ENABLE							\
	@GIO_CFLAGS@							\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_settings_LDADD =						\
	@GIO_LIBS@							\
	@LIBX11_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	@XFCONF_LIBS@							\
	$(NULL)

#
# Status Icon
#

if HAVE_LIBX11

xfce4_clipman_SOURCES =							\
	main-status-icon.c						\
	common.c			common.h			\
	plugin.c			plugin.h			\
	actions.c			actions.h			\
	collector.c			collector.h			\
	history.c			history.h			\
	menu.c				menu.h				\
	$(NULL)

xfce4_clipman_CFLAGS =							\
	-DGSEAL_ENABLE							\
	-DSTATUS_ICON							\
	@GIO_CFLAGS@							\
	@LIBX11_CFLAGS@							\
	@LIBXTST_CFLAGS@						\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@					\
	@LIBXFCE4UI_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_LDADD =							\
	$(top_builddir)/clipboard-manager/libdaemon.la							\
	@GIO_LIBS@							\
	@LIBX11_LIBS@							\
	@LIBXTST_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCE4UI_LIBS@						\
	@XFCONF_LIBS@							\
	@QRENCODE_LIBS@							\
	$(NULL)

endif

#
# Panel Plugin
#

plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libclipman.la

libclipman_la_SOURCES =							\
	main-panel-plugin.c						\
	common.c			common.h			\
	plugin.c			plugin.h			\
	actions.c			actions.h			\
	collector.c			collector.h			\
	history.c			history.h			\
	menu.c				menu.h				\
	$(NULL)

libclipman_la_CFLAGS =							\
	-DPANEL_PLUGIN							\
	@GIO_CFLAGS@							\
	@LIBX11_CFLAGS@							\
	@LIBXTST_CFLAGS@						\
	@GDKX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@					\
	@LIBXFCE4UI_CFLAGS@						\
	@LIBXFCE4PANEL_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(PLATFORM_CFLAGS)						\
	$(NULL)

libclipman_la_LDFLAGS =							\
	-avoid-version							\
	-module								\
	-no-undefined							\
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)						\
	$(NULL)

libclipman_la_LIBADD =							\
	$(top_builddir)/clipboard-manager/libdaemon.la							\
	@GIO_LIBS@							\
	@LIBX11_LIBS@							\
	@LIBXTST_LIBS@							\
	@GDKX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCE4UI_LIBS@						\
	@LIBXFCE4PANEL_LIBS@						\
	@XFCONF_LIBS@							\
	@QRENCODE_LIBS@
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

pluginsdir = $(datadir)/xfce4/panel/plugins
plugins_in_files = xfce4-clipman-plugin.desktop.in
plugins_DATA = $(plugins_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appsdir = $(datadir)/applications
apps_in_files = xfce4-clipman-settings.desktop.in
if HAVE_LIBX11
apps_in_files += xfce4-clipman.desktop.in
endif
apps_DATA = $(apps_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

#
# Autostart File
#

if HAVE_LIBX11

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = xfce4-clipman-plugin-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

endif

#
# Extra
#

EXTRA_DIST =								\
	settings-dialog.ui						\
	$(actions_in_files)						\
	$(autostart_in_files)						\
	$(plugins_in_files)						\
	$(apps_in_files)						\
	$(NULL)

CLEANFILES =								\
	$(actions_DATA)							\
	$(autostart_DATA)						\
	$(plugins_DATA)							\
	$(apps_DATA)							\
	$(NULL)

DISTCLEANFILES =							\
	$(BUILT_SOURCES)						\
	$(NULL)

install-exec-hook:
	cd $(DESTDIR)$(exec_prefix)/bin && \
	$(LN_S) -f xfce4-popup-clipman xfce4-popup-clipman-actions
