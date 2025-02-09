project(
  'xfce4-clipman-plugin',
  'c',
  version: '1.6.7-dev',
  license: 'GPL-2.0-or-later',
  meson_version: '>= 0.61.0',
  default_options: [
    'c_std=gnu11',
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)

project_namespace = 'panel-plugins'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')
fs = import('fs')

dependency_versions = {
  'glib': '>= 2.60.0',
  'gtk': '>= 3.22.29',
  'xfce4': '>= 4.18.0',
  'x11': '>= 1.6.7',
  'xproto': '>= 7.0.0',
  'wayland': '>= 1.15.0',
  'xtst': '>= 1.0.0',
  'libqrencode': '>= 3.3.0',
}

gio = dependency('gio-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
libxfce4panel = dependency('libxfce4panel-2.0', version: dependency_versions['xfce4'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
xfconf = dependency('libxfconf-0', version: dependency_versions['xfce4'])

xdt_csource = find_program('xdt-csource')

# Feature: 'x11'
x11 = dependency('x11', version: dependency_versions['x11'], required: get_option('x11'))
gdk_x11 = dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
xproto = dependency('xproto', version: dependency_versions['xproto'], required: get_option('x11'))
x11_deps = [x11, gdk_x11, xproto]

# Feature: 'wayland'
gdk_wayland = dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))
wayland_scanner = dependency('wayland-scanner', version: dependency_versions['wayland'], required: get_option('wayland'), native: true)
wayland_client = dependency('wayland-client', version: dependency_versions['wayland'], required: get_option('wayland'))
wayland_deps = [gdk_wayland, wayland_scanner, wayland_client]

feature_cflags = []

enable_x11 = not get_option('x11').disabled()
foreach dep : x11_deps
  enable_x11 = enable_x11 and dep.found()
endforeach
if enable_x11
  feature_cflags += '-DENABLE_X11=1'
  xtst = dependency('xtst', version: dependency_versions['xtst'], required: get_option('xtst'))
  if xtst.found()
    feature_cflags += '-DHAVE_LIBXTST=1'
  endif
else
  xtst = dependency('', required: false)
endif

enable_wayland = not get_option('wayland').disabled()
foreach dep : wayland_deps
  enable_wayland = enable_wayland and dep.found()
endforeach
if enable_wayland
  feature_cflags += '-DENABLE_WAYLAND=1'

  wayland_scanner_path = wayland_scanner.get_variable(pkgconfig: 'wayland_scanner')
  wayland_scanner_bin = find_program(wayland_scanner_path, required: true)

  wlr_protocols = dependency('wlr-protocols', required: false)
  if wlr_protocols.found()
    wlr_protocols_pkgdatadir = wlr_protocols.get_variable(pkgconfig: 'pkgdatadir')
  else
    wlr_protocols_pkgdatadir = meson.project_source_root() / 'protocols' / 'wlr-protocols'
    if not fs.is_dir(wlr_protocols_pkgdatadir / 'unstable')
      error(wlr_protocols_pkgdatadir + ' seems empty, did you clone the wlr-protocols submodule?')
    endif
  endif
endif

if not enable_x11 and not enable_wayland
  error('At least one of the X11 and Wayland backends must be enabled')
endif

qrencode = dependency('libqrencode', version: dependency_versions['libqrencode'], required: get_option('qrencode'))
if qrencode.found()
  feature_cflags += '-DHAVE_QRENCODE=1'
endif

if cc.check_header('locale.h')
  feature_cflags += '-DHAVE_LOCALE_H=1'
endif

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_URL="https://docs.xfce.org/@0@/@1@/start"'.format(project_namespace, meson.project_name()),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DDOCDIR="@0@"'.format(get_option('prefix') / 'doc' / meson.project_name()),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('protocols')
subdir('data')
subdir('po')
subdir('clipboard-manager')
subdir('panel-plugin')
subdir('tests')
