NULL = 

INCLUDES =								\
	-I${top_srcdir}							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(NULL)

noinst_LTLIBRARIES = libdaemon.la

libdaemon_la_SOURCES =							\
	daemon.h							\
	gsd-clipboard-manager.c		gsd-clipboard-manager.h		\
	list.c				list.h				\
	xutils.c			xutils.h			\
	$(NULL)

libdaemon_la_CFLAGS =							\
	@LIBX11_CFLAGS@							\
	@GTK_CFLAGS@							\
	$(NULL)

libdaemon_la_LIBADD =							\
	@LIBX11_LIBS@							\
	@GTK_LIBS@							\
	$(NULL)
