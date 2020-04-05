appdatadir = $(datadir)/metainfo
appdata_in_files = xfce4-clipman.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
EXTRA_DIST = $(appdata_in_files)
CLEANFILES = $(appdata_DATA)
