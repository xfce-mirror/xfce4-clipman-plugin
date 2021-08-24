#!/usr/bin/env bash
dbus-send --session           \
  --dest=org.xfce.clipman.GDBus.service \
  --type=method_call          \
  --reply-timeout=1000 \
  --print-reply               \
  /org/xfce/clipman/GDBus/service       \
  org.xfce.clipman.GDBus.service.get_item_by_id \
  uint32:$1
