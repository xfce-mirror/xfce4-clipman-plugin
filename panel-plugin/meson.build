settings_dialog_ui_h = custom_target(
  'settings-dialog_ui.h',
  input: 'settings-dialog.ui',
  output: 'settings-dialog_ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=settings_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

xfce4_popup_clipman_sources = [
  'common.c',
  'common.h',
  'xfce4-popup-clipman.c',
]

executable(
  'xfce4-popup-clipman',
  xfce4_popup_clipman_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gtk,
    libxfce4ui,
    libxfce4util,
    x11_deps,
    xtst,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

xfce4_clipman_history_sources = [
  'actions.c',
  'collector.c',
  'common.c',
  'common.h',
  'history.c',
  'menu.c',
  'plugin.c',
  'xfce4-clipman-history.c',
]

executable(
  'xfce4-clipman-history',
  xfce4_clipman_history_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio,
    gtk,
    libxfce4ui,
    libxfce4util,
    qrencode,
    x11_deps,
    xfconf,
    xtst,
  ],
  link_with: [
    clipboard_manager,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

xfce4_clipman_settings_sources = [
  'xfce4-clipman-settings.c',
  settings_dialog_ui_h,
  'actions.c',
  'actions.h',
  'common.h',
]

executable(
  'xfce4-clipman-settings',
  xfce4_clipman_settings_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio,
    gtk,
    libxfce4ui,
    libxfce4util,
    x11_deps,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

if enable_x11
  xfce4_clipman_sources = [
    'actions.c',
    'actions.h',
    'collector.c',
    'collector.h',
    'common.c',
    'common.h',
    'history.c',
    'history.h',
    'main-status-icon.c',
    'menu.c',
    'menu.h',
    'plugin.c',
    'plugin.h',
  ]

  executable(
    'xfce4-clipman',
    xfce4_clipman_sources,
    sources: xfce_revision_h,
    include_directories: [
      include_directories('..'),
    ],
    dependencies: [
      gio,
      gtk,
      libxfce4ui,
      libxfce4util,
      qrencode,
      x11_deps,
      xfconf,
      xtst,
    ],
    c_args: [
      '-DSTATUS_ICON',
    ],
    link_with: [
      clipboard_manager,
    ],
    install: true,
    install_dir: get_option('prefix') / get_option('bindir'),
  )
endif

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_sources = [
  'actions.c',
  'actions.h',
  'collector.c',
  'collector.h',
  'common.c',
  'common.h',
  'history.c',
  'history.h',
  'main-panel-plugin.c',
  'menu.c',
  'menu.h',
  'plugin.c',
  'plugin.h',
] + xfce_revision_h

plugin_lib = shared_library(
  'clipman',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DPANEL_PLUGIN',
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-clipman-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio,
    gtk,
    libxfce4util,
    libxfce4ui,
    libxfce4panel,
    qrencode,
    x11_deps,
    xfconf,
    xtst,
  ],
  link_with: [
    clipboard_manager,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'xfce4-clipman-actions.xml.in',
  output: 'xfce4-clipman-actions.xml',
  po_dir: '..' / 'po',
  type: 'xml',
  data_dirs: '..' / 'data',
  install: true,
  install_dir: get_option('prefix') / get_option('sysconfdir') / 'xdg' / 'xfce4' / 'panel',
)

desktop_files = [
  'xfce4-clipman.desktop',
  'xfce4-clipman-settings.desktop',
]
foreach desktop_file : desktop_files
  i18n.merge_file(
    input: '@0@.in'.format(desktop_file),
    output: desktop_file,
    po_dir: '..' / 'po',
    type: 'desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'applications',
  )
endforeach

i18n.merge_file(
  input: 'xfce4-clipman-plugin.desktop.in',
  output: 'xfce4-clipman-plugin.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'xfce4-clipman-plugin-autostart.desktop.in',
  output: 'xfce4-clipman-plugin-autostart.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('sysconfdir') / 'xdg' / 'autostart',
)

install_symlink(
  'xfce4-popup-clipman-actions',
  pointing_to: 'xfce4-popup-clipman',
  install_dir: get_option('prefix') / get_option('bindir'),
)
