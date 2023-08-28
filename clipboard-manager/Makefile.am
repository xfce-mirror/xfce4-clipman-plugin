NULL = 

AM_CPPFLAGS =								\
	-DGSEAL_ENABLE							\
	-I${top_srcdir}							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(NULL)

noinst_LTLIBRARIES = libdaemon.la

libdaemon_la_SOURCES =							\
	daemon.h							\
	clipboard-manager.c		clipboard-manager.h		\
	$(NULL)

if HAVE_LIBX11
libdaemon_la_SOURCES += \
	clipboard-manager-x11.c \
	clipboard-manager-x11.h \
	$(NULL)
endif

libdaemon_la_CFLAGS =							\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@							\
	$(NULL)

libdaemon_la_LIBADD =							\
	@LIBX11_LIBS@							\
	@GTK_LIBS@							\
	$(NULL)
