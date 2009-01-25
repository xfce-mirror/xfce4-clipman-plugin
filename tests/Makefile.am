NULL = 

INCLUDES =								\
	-I${top_srcdir}							\
	-DDATADIR=\"$(datadir)\"					\
	-DSRCDIR=\"$(top_srcdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(NULL)

noinst_PROGRAMS = test-actions
test_actions_SOURCES = test_actions.c ../panel-plugin/actions.c ../panel-plugin/actions.h ../panel-plugin/common.h
test_actions_CFLAGS = @GTK_CFLAGS@ @EXO_CFLAGS@ @LIBXFCE4UTIL_CFLAGS@ @LIBXFCEGUI4_CFLAGS@
test_actions_LDADD = @GTK_LIBS@ @EXO_LIBS@ @LIBXFCE4UTIL_LIBS@ @LIBXFCEGUI4_LIBS@ 
