plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libclipman.la

libclipman_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libclipman_la_SOURCES =							\
	clipman.c

libclipman_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libclipman_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
