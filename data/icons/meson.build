
sizes = [16, 24, 32, 48, 64]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'xfce4-clipman-plugin.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  'scalable' / 'xfce4-clipman-plugin.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

install_data(
  '16x16' / 'clipman-symbolic.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '16x16' / 'apps',
)
