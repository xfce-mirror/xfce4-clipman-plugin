#!/usr/bin/env bash
dbus-send --session           \
  --dest=org.xfce.clipman.GDBus.service \
  --type=method_call          \
  --print-reply               \
  /org/xfce/clipman/GDBus/service       \
  org.xfce.clipman.GDBus.service.list_history
