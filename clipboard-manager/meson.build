clipboard_manager_sources = [
  'clipboard-manager.c',
  'clipboard-manager.h',
]

if enable_x11
  clipboard_manager_sources += [
    'clipboard-manager-x11.c',
    'clipboard-manager-x11.h',
  ]
endif

if enable_wayland
  clipboard_manager_sources += [
    'clipboard-manager-wayland.c',
    'clipboard-manager-wayland.h',
  ] + wayland_protocols_generated_sources
endif

clipboard_manager = static_library(
  'daemon',
  clipboard_manager_sources,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    x11,
    wayland_client,
    gio_unix,
    gtk,
    libxfce4ui,
  ],
  install: false,
)
