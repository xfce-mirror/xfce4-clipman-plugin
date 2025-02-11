if get_option('interactive-tests')
  executable(
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

  executable(
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

  executable(
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

  if enable_x11 and xtst.found()
    executable(
      'test-xfakekey',
      [
        'test_xfakekey.c',
      ],
      include_directories: [
        include_directories('..')
      ],
      dependencies: [
        gtk,
        x11,
        xtst,
      ],
      install: false,
    )
  endif
endif
