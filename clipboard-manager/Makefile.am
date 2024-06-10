NULL = 

AM_CPPFLAGS =								\
	-DGSEAL_ENABLE							\
	-I${top_srcdir}							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(NULL)

noinst_LTLIBRARIES = libdaemon.la

libdaemon_la_SOURCES =							\
	clipboard-manager.c		clipboard-manager.h		\
	$(NULL)

if ENABLE_X11
libdaemon_la_SOURCES += \
	clipboard-manager-x11.c \
	clipboard-manager-x11.h \
	$(NULL)
endif

if ENABLE_WAYLAND
libdaemon_la_SOURCES += \
	clipboard-manager-wayland.c \
	clipboard-manager-wayland.h \
	$(NULL)
endif

libdaemon_la_CFLAGS =							\
	@LIBX11_CFLAGS@							\
	@WAYLAND_CLIENT_CFLAGS@							\
	@GIO_UNIX_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	$(NULL)

libdaemon_la_LIBADD =

if ENABLE_WAYLAND
libdaemon_la_LIBADD += \
	$(top_builddir)/protocols/libprotocols.la
endif

libdaemon_la_LIBADD +=							\
	@LIBX11_LIBS@							\
	@WAYLAND_CLIENT_LIBS@							\
	@GIO_UNIX_LIBS@							\
	@GTK_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	$(NULL)
