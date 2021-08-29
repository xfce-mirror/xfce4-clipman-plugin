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
    "$@")

  # format dbus-send output to remove some extrat stuff
  echo "$out" | sed -e '/method return/ d' \
      -e 's/ \+string "//' \
      -e '$ s/"$//'
}

case $action in
  list)
    call_dbus list_history
    ;;
  get)
    call_dbus get_item_by_id boolean:false uint32:$1
    ;;
  get_secure)
    call_dbus get_item_by_id boolean:true uint32:$1
    ;;
  del)
    call_dbus delete_item_by_id uint32:$1
    ;;
  add)
    secure=false
    if [[ $1 == '-s' ]]
    then
      secure=true
      shift
      # encode secure_text
      input_text="⛔$(echo -n "$1" | base64)"
    else
      input_text="$1"
    fi
    call_dbus add_item boolean:$secure string:"$input_text"
    ;;
  clear)
    call_dbus clear_history boolean:false
    ;;
  clear_secure)
    call_dbus clear_history boolean:true
    ;;
  *)
    >&2 echo "unknown method: $action"
    ;;
esac
