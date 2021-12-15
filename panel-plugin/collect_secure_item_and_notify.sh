#!/bin/bash
#
# collect_secure_item_and_notify.sh

SCRIPT_DIR=$(dirname $(realpath $0))

visual_notify()
{
  local msg=$1
  if [[ -x $(which notify-send) ]]
  then
    notify-send -u normal -i "gcr-key" 'pass_clip.sh' "$msg"
  fi
}

clipman_cli=$SCRIPT_DIR/clipman_cli.sh

$clipman_cli collect_secure
id=$($clipman_cli check_new_secure_item_collected 20)
r=$?
if [[ $r -eq 0 ]]
then
  visual_notify "item secured: $id"
fi
