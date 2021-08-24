#!/usr/bin/env bash
#
# bash wrapper to simulate dbus call
#

action=$1
shift

call_dbus()
{
  local method_name=$1
  shift
  local out=$(dbus-send --session           \
    --dest=org.xfce.clipman.GDBus.service \
    --type=method_call          \
    --reply-timeout=1000 \
    --print-reply               \
    /org/xfce/clipman/GDBus/service       \
    org.xfce.clipman.GDBus.service.$method_name \
    $*)

  # format dbus-send output to remove some extrat stuff
  sed -e '/method return/ d' -e 's/ \+string "//' -e '$ s/"$//' <<< "$out"
}

case $action in
  list)
    call_dbus list_history
    ;;
  get)
    call_dbus get_item_by_id uint32:$1
    ;;
  del)
    call_dbus delete_item_by_id uint32:$1
    ;;
  *)
    >&2 echo "unknown method: $action"
    ;;
esac


