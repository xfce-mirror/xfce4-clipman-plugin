if MAINTAINER_MODE
NULL = 

INCLUDES =								\
	-I${top_srcdir}							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

noinst_PROGRAMS = test-actions test-daemon test-targets
test_actions_SOURCES = test_actions.c ../panel-plugin/actions.c ../panel-plugin/actions.h ../panel-plugin/common.h
test_actions_CFLAGS = @GTK_CFLAGS@ @EXO_CFLAGS@ @LIBXFCE4UTIL_CFLAGS@ @LIBXFCEGUI4_CFLAGS@
test_actions_LDADD = @GTK_LIBS@ @EXO_LIBS@ @LIBXFCE4UTIL_LIBS@ @LIBXFCEGUI4_LIBS@ 
test_daemon_SOURCES = test_daemon.c
test_daemon_CFLAGS = @GTK_CFLAGS@
test_daemon_LDADD = $(top_builddir)/daemon/libdaemon.la @GTK_LIBS@
test_targets_SOURCES = test_targets.c
test_targets_CFLAGS = @GTK_CFLAGS@
test_targets_LDADD = @GTK_LIBS@
endif

