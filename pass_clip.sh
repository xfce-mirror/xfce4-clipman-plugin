#!/bin/bash
#
#

DELETE_DELAY=30
SCRIPT_DIR=$(dirname $0)

keep_password=0
if [[ $1 == '-k' ]]
then
  keep_password=1
  shift
fi

clipman_cli=$SCRIPT_DIR/panel-plugin/clipman_cli.sh
content=$(pass show "$1")
if [[ -n $content ]]
then
  id=$($clipman_cli add -s "$(head -1 <<< "$content")" | awk '{print $2}')
  unset content
  if [[ -n $id ]]
  then
    echo -n "entry copied: $id"
    if [[ $keep_password -eq 0 ]]
    then
      ( sleep $DELETE_DELAY && $clipman_cli del $id; ) &
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
