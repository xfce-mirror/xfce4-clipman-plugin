#!/usr/bin/env bash
#
# bash wrapper to perform simple dbus call on clipman DBus API
# This code will be next written in C with all needed controls and argument parsing.
# This code Usage is a draft and can be changed
#
# Usage: ./clipman_cli.sh add [-s] TEXT_ITEM_VALUE
#        ./clipman_cli.sh list
#        ./clipman_cli.sh del ITEM_ID
#        ./clipman_cli.sh get ITEM_ID
#        ./clipman_cli.sh get_secure ITEM_ID
#        ./clipman_cli.sh clear
#        ./clipman_cli.sh clear_secure
#        ./clipman_cli.sh set_secure  ITEM_ID
#        ./clipman_cli.sh set_clear_text ITEM_ID
#        ./clipman_cli.sh collect_secure [NUM_COLLECTED_ITEM_TO_SECURE]
#        ./clipman_cli.sh set_history_size [SIZE]
#
# Arguments:
#   TEXT_ITEM_VALUE               string to add to history.
#   ITEM_ID                       uint16 a clipman history ID (use list or add
#                                 to retreive it).
#   NUM_COLLECTED_ITEM_TO_SECURE  uint16 the next collected item from clipboard
#                                 will be set secured by clipman automatically.
#                                 Useful for keyboard shortcut before copying
#                                 [default: 1]
#   SIZE                          unit16 the new size between 5 and 5000.
#
# See also case bellow, usage may be no updated.

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

  # format dbus-send output to remove some extra stuff
  echo "$out" | sed -e '/method return/ d' \
      -e 's/ \+string "//' \
      -e '$ s/"$//'
}

case $action in
  list)
    call_dbus list_history
    ;;
  get)
    call_dbus get_item_by_id boolean:false uint16:$1
    ;;
  get_secure)
    call_dbus get_item_by_id boolean:true uint16:$1
    ;;
  del)
    call_dbus delete_item_by_id uint16:$1
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
  set_secure)
    call_dbus set_secure_by_id boolean:true uint16:$1
    ;;
  set_clear_text)
    call_dbus set_secure_by_id boolean:false uint16:$1
    ;;
  secure_collect|collect_secure)
    nb_next_item_secured=1
    if [[ -n $1 ]]
    then
      nb_next_item_secured=$1
    fi
    call_dbus collect_next_item_secure uint16:$nb_next_item_secured
    ;;
  set_history_size|resize_history)
    call_dbus resize_history uint16:$1
    ;;
  *)
    >&2 echo "unknown method: $action"
    ;;
esac
