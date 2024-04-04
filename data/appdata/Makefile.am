appdatadir = $(datadir)/metainfo
appdata_in_files = xfce4-clipman.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

# Not using HAVE_APPSTREAM here because a lot of distributions put
# appdata.{loc,its} on binary packages instead of development packages.
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@ || \
	cp $< $@

EXTRA_DIST = $(appdata_in_files)
CLEANFILES = $(appdata_DATA)
