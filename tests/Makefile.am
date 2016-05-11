NULL =

INCLUDES =								\
	-I${top_srcdir}							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

check_PROGRAMS = test-actions test-daemon test-targets test-xfakekey
test_actions_SOURCES = test_actions.c ../panel-plugin/actions.c ../panel-plugin/actions.h ../panel-plugin/common.h
test_actions_CFLAGS = @GTK_CFLAGS@ @LIBXFCE4UI_CFLAGS@
test_actions_LDADD = @GTK_LIBS@ @LIBXFCE4UI_LIBS@
test_daemon_SOURCES = test_daemon.c
test_daemon_CFLAGS = @GTK_CFLAGS@
test_daemon_LDADD = $(top_builddir)/x11-clipboard-manager/libdaemon.la @GTK_LIBS@
test_targets_SOURCES = test_targets.c
test_targets_CFLAGS = @GTK_CFLAGS@
test_targets_LDADD = @GTK_LIBS@
test_xfakekey_SOURCES = test_xfakekey.c
test_xfakekey_CFLAGS = @GTK_CFLAGS@ @LIBX11_CFLAGS@ @LIBXTST_CFLAGS@
test_xfakekey_LDADD = @GTK_LIBS@ @LIBX11_LIBS@ @LIBXTST_LIBS@
