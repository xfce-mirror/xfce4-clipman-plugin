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
#        ./clipman_cli.sh get_last_item_id
#        ./clipman_cli.sh check_new_secure_item_collected [DURATION_SECOND]
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

get_last_item_id()
{
  call_dbus list_history | tail -1 | awk '{print $1}'
}

get_item()
{
  local secure=false
  if [[ $1 == '-s' ]]
  then
    secure=true
    shift
  fi
  call_dbus get_item_by_id boolean:$secure uint16:$1
}

wait_for_new_item()
{
  # default duration is 10 sec
  local duration_second=${1:-10}
  local last_id=$(get_last_item_id)
  local i new_id
  local found=1
  for i in $(seq $duration_second)
  do
    new_id=$(get_last_item_id)
    if [[ $last_id != $new_id ]]
    then
      found=0
      break
    fi
    new_id=""
    sleep .8
  done
  echo "$new_id"
  return $found
}


case $action in
  list)
    call_dbus list_history
    ;;
  get)
    get_item "$1"
    ;;
  get_secure)
    get_item -s "$1"
    ;;
  del|delete)
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
  get_last_item_id)
    get_last_item_id
    ;;
  check_new_secure_item_collected)
    new_id=$(wait_for_new_item "$1")
    if [[ $? -eq 0 ]]
    then
      v=$(get_item "$new_id")
      #echo "found: $new_id: $v"
      if [[ ${v:0:1} == "⛔" ]]
      then
        # new secured item found
        echo "$new_id"
        exit 0
      fi
    fi
    exit 1
    ;;
  wait_for_new_item)
    wait_for_new_item "$1"
    ;;
  *)
    >&2 echo "unknown method: $action"
    ;;
esac
