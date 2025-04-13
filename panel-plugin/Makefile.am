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

if ENABLE_X11
bin_PROGRAMS += xfce4-clipman
endif

BUILT_SOURCES =
BUILT_SOURCES += clipman-dialogs-resources.c

clipman_dialogs_glade_files = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/clipman-dialogs.gresource.xml)

clipman-dialogs-resources.c: $(srcdir)/clipman-dialogs.gresource.xml $(clipman_dialogs_glade_files) Makefile
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

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
	@LIBXTST_CFLAGS@						\
	@GTK_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	@LIBXFCE4UTIL_CFLAGS@						\
	$(NULL)

xfce4_popup_clipman_LDADD =						\
	@LIBX11_LIBS@							\
	@LIBXTST_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	@LIBXFCE4UTIL_LIBS@						\
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
	clipman-dialogs-resources.c					\
	xfce4-clipman-settings.c					\
	common.h							\
	actions.c			actions.h			\
	$(NULL)

xfce4_clipman_settings_CFLAGS =						\
	-DGSEAL_ENABLE							\
	@GIO_CFLAGS@							\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	@LIBXFCE4UTIL_CFLAGS@						\
	@XFCONF_CFLAGS@							\
	$(NULL)

xfce4_clipman_settings_LDADD =						\
	@GIO_LIBS@							\
	@LIBX11_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	@LIBXFCE4UTIL_LIBS@						\
	@XFCONF_LIBS@							\
	$(NULL)

#
# Status Icon
#

if ENABLE_X11

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
actions_in_in_files = xfce4-clipman-actions.xml.in.in
actions_in_files = $(actions_in_in_files:.xml.in.in=.xml.in)
actions_DATA = $(actions_in_files:.xml.in=.xml)
BUILT_SOURCES += $(actions_in_files)

$(actions_in_files): $(actions_in_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s,\@OPEN_CMD\@,exo-open,g" $< >$@

$(actions_DATA): $(actions_in_files) Makefile
	$(AM_V_GEN)GETTEXTDATADIRS=$(top_srcdir)/data $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

#
# Desktop Entry File
#

pluginsdir = $(datadir)/xfce4/panel/plugins
plugins_in_files = xfce4-clipman-plugin.desktop.in
plugins_DATA = $(plugins_in_files:.desktop.in=.desktop)
$(plugins_DATA): $(plugins_in_files) Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

appsdir = $(datadir)/applications
apps_in_files = xfce4-clipman-settings.desktop.in
xfce4-clipman-settings.desktop: xfce4-clipman-settings.desktop.in Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
if ENABLE_X11
apps_in_files += xfce4-clipman.desktop.in
xfce4-clipman.desktop: xfce4-clipman.desktop.in Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
endif
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

#
# Autostart File
#

if ENABLE_X11

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = xfce4-clipman-plugin-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
$(autostart_DATA): $(autostart_in_files) Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

endif

#
# Extra
#

EXTRA_DIST =								\
	clipman-dialogs.gresource.xml					\
	settings-dialog.ui						\
	$(actions_in_in_files)						\
	$(autostart_in_files)						\
	$(plugins_in_files)						\
	$(apps_in_files)						\
	meson.build							\
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
