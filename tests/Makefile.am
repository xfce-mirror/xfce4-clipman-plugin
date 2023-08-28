NULL =

AM_CPPFLAGS =								\
	-I${top_srcdir}							\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

check_PROGRAMS = test-actions test-targets
test_actions_SOURCES = test_actions.c ../panel-plugin/actions.c ../panel-plugin/actions.h ../panel-plugin/common.h
test_actions_CFLAGS = @GTK_CFLAGS@ @LIBXFCE4UTIL_CFLAGS@ @LIBXFCE4UI_CFLAGS@
test_actions_LDADD = @GTK_LIBS@ @LIBXFCE4UTIL_LIBS@ @LIBXFCE4UI_LIBS@
test_targets_SOURCES = test_targets.c
test_targets_CFLAGS = @GTK_CFLAGS@
test_targets_LDADD = @GTK_LIBS@

if HAVE_LIBX11
check_PROGRAMS += test-daemon
test_daemon_SOURCES = test_daemon.c
test_daemon_CFLAGS = @GTK_CFLAGS@
test_daemon_LDADD = $(top_builddir)/clipboard-manager/libdaemon.la @GTK_LIBS@
endif

if HAVE_LIBXTST
check_PROGRAMS += test-xfakekey
test_xfakekey_SOURCES = test_xfakekey.c
test_xfakekey_CFLAGS = @GTK_CFLAGS@ @LIBX11_CFLAGS@ @LIBXTST_CFLAGS@
test_xfakekey_LDADD = @GTK_LIBS@ @LIBX11_LIBS@ @LIBXTST_LIBS@
endif
