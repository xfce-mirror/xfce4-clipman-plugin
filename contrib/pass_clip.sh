#!/bin/bash
#
# PoC read password store to clipman secure

DELETE_DELAY=30
SCRIPT_DIR=$(dirname $(realpath $0))

visual_notify()
{
  local msg=$1
  if [[ -x $(which notify-send) ]]
  then
    notify-send -u normal -i "gcr-key" 'pass_clip.sh' "$msg"
  fi
}

keep_password=0
if [[ $1 == '-k' ]]
then
  keep_password=1
  shift
fi

clipman_cli=$SCRIPT_DIR/../panel-plugin/clipman_cli.sh

if [[ ! -x $clipman_cli ]]
then
  >&2 echo "error: clipman_cli not found: '$clipman_cli'"
  exit 1
fi

input="$1"
if [[ -z $input ]]
then
  >&2 echo "error: input is empty"
  exit 1
fi

content=$(pass show "$input")
if [[ -n $content ]]
then
  id=$($clipman_cli add -s "$(head -1 <<< "$content")" | awk '{print $2}')
  unset content
  if [[ -n $id ]]
  then
    echo -n "entry copied: $id"
    if [[ $keep_password -eq 0 ]]
    then
      # start a backgroup sub-shell process with the code in parentheses
      (
        sleep $DELETE_DELAY \
          && $clipman_cli del $id > /dev/null 2>&1 \
          && visual_notify "item $id removed from clipman"
      ) &
      disown $!
      echo ", will be deleted in ${DELETE_DELAY}s"
    else
      echo ", will be kept"
    fi
    exit 0
  fi
fi

# something went wrong
exit 1
