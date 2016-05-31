NULL = 

AM_CPPFLAGS =								\
	-DGSEAL_ENABLE							\
	-I${top_srcdir}							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(NULL)

noinst_LTLIBRARIES = libdaemon.la

libdaemon_la_SOURCES =							\
	daemon.h							\
	gsd-clipboard-manager.c		gsd-clipboard-manager.h		\
	$(NULL)

libdaemon_la_CFLAGS =							\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@							\
	$(NULL)

libdaemon_la_LIBADD =							\
	@LIBX11_LIBS@							\
	@GTK_LIBS@							\
	$(NULL)
