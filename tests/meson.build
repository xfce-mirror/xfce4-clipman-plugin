add_test_setup(
  'default',
  exclude_suites: 'gui',
  is_default: true,
)
add_test_setup(
  'full',
)

test_actions = executable(
  'test-actions',
  [
    'test_actions.c',
    '..' / 'panel-plugin' / 'actions.c',
    '..' / 'panel-plugin' / 'actions.h',
    '..' / 'panel-plugin' / 'common.h',
  ],
  include_directories: [
    include_directories('..')
  ],
  dependencies: [
    gtk,
    libxfce4ui,
    libxfce4util,
  ],
  install: false,
)
test('test-actions', test_actions, suite: 'gui')

test_targets = executable(
  'test-targets',
  [
    'test_targets.c',
  ],
  include_directories: [
    include_directories('..')
  ],
  dependencies: [
    gtk,
  ],
  install: false,
)
test('test-targets', test_targets, suite: 'gui')

test_daemon = executable(
  'test-daemon',
  [
    'test_daemon.c',
  ],
  include_directories: [
    include_directories('..')
  ],
  dependencies: [
    gtk,
  ],
  link_with: [
    clipboard_manager,
  ],
  install: false,
)
test('test-daemon', test_daemon, suite: 'gui')

if enable_x11 and xtst.found()
  test_xfakekey = executable(
    'test-xfakekey',
    [
      'test_xfakekey.c',
    ],
    include_directories: [
      include_directories('..')
    ],
    dependencies: [
      gtk,
      x11_deps,
      xtst,
    ],
    install: false,
  )
  test('test-xfakekey', test_xfakekey, suite: 'gui')
endif
