settings_dialog_ui_h = custom_target(
  'settings-dialog_ui.h',
  input: 'settings-dialog.ui',
  output: 'settings-dialog_ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=settings_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

xfce4_popup_clipman_sources = [
  'xfce4-popup-clipman.c',
  'common.c',
  'common.h',
]

executable(
  'xfce4-popup-clipman',
  xfce4_popup_clipman_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    x11,
    xtst,
    gtk,
    libxfce4ui,
    libxfce4util,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

xfce4_clipman_history_sources = [
  'xfce4-clipman-history.c',
  'actions.c',
  'common.c',
  'common.h',
  'collector.c',
  'history.c',
  'menu.c',
  'plugin.c',
]

executable(
  'xfce4-clipman-history',
  xfce4_clipman_history_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    x11,
    xtst,
    gio,
    gtk,
    libxfce4ui,
    libxfce4util,
    xfconf,
    qrencode,
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
  'common.h',
  'actions.c',
  'actions.h',
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
    x11,
    gtk,
    libxfce4ui,
    libxfce4util,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

if enable_x11
  xfce4_clipman_sources = [
    'main-status-icon.c',
    'common.c',
    'common.h',
    'plugin.c',
    'plugin.h',
    'actions.c',
    'actions.h',
    'collector.c',
    'collector.h',
    'history.c',
    'history.h',
    'menu.c',
    'menu.h',
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
      x11,
      xtst,
      gtk,
      libxfce4ui,
      libxfce4util,
      xfconf,
      qrencode,
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
  'main-panel-plugin.c',
  'common.c',
  'common.h',
  'plugin.c',
  'plugin.h',
  'actions.c',
  'actions.h',
  'collector.c',
  'collector.h',
  'history.c',
  'history.h',
  'menu.c',
  'menu.h',
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
    x11,
    xtst,
    gtk,
    libxfce4util,
    libxfce4ui,
    libxfce4panel,
    xfconf,
    qrencode,
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
