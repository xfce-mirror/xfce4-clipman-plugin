appdatadir = $(datadir)/metainfo
appdata_in_files = xfce4-clipman.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(appdata_in_files) \
	meson.build \
	$(NULL)

CLEANFILES = $(appdata_DATA)
