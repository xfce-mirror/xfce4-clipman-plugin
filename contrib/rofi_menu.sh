#!/usr/bin/env bash
#
# rofi menu for clipman_cli actions
#
# GUI interface for pre-defined action orchestrated via clipman_cli
#

options="
🔐 secure next copy
🔳 html black box
🗑️ clear secure items
⚙️ columnize last clipboard entry
"

visual_notify()
{
  local icon="clipman"
  if [[ $1 == '-s' ]]
  then
    icon=gcr-key
    shift
  fi
  local msg=$1
  if [[ -x $(which notify-send) ]]
  then
    notify-send -u normal -i "$icon" 'clipman' "$msg"
  fi
}

myclip()
{
  xclip -i -selection clipboard
}

columnize_stdin_with_datatime()
{
  timeout 1s sed -e 's/\([0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}\) \([0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\)/\1_\2/g' | column -t
}

SCRIPT_DIR=$(dirname $(realpath $0))
clipman_cli=$SCRIPT_DIR/../panel-plugin/clipman_cli.sh
transform_clipboard=$SCRIPT_DIR/transform_clipboard.py

# filter options removing empty lines
filtered_options=$(echo "$options" | sed -n -e '/^./p')
nb=$(echo "$filtered_options" | wc -l)
rofi_theme=Paper
IFS=$'\n'
r=$(echo "$filtered_options" \
  | rofi -lines $nb -dmenu -p "custom clipman behavior" -theme $rofi_theme \
  | awk '{print $2}'
)

# action is based on the first word of the menu
case $r in
  secure)
    $clipman_cli collect_secure 1
    id=$($clipman_cli check_new_secure_item_collected 20)
    r=$?
    if [[ $r -eq 0 ]]
    then
      visual_notify -s "item secured: $id"
    fi
    ;;
  html)
    item_id=$($clipman_cli wait_for_new_item)
    if [[ $? -eq 0 ]]
    then
      html_content=$($clipman_cli get $item_id | python3 $transform_clipboard)
      $clipman_cli del $item_id
      echo "$html_content" | myclip
      visual_notify "new html content formated: $($clipman_cli get_last_item_id)"
    fi
    ;;
  clear)
    nb=$($clipman_cli clear_secure | awk '{print $2}')
    if [[ $nb -gt 1 ]]
    then
      msg="🗑️ $nb secure items cleared"
    else
      msg="🗑️ $nb secure item cleared"
    fi

    visual_notify -s "$msg"
    ;;
  columnize)
    item_id=$($clipman_cli get_last_item_id)
    if [[ -n $item_id ]]
    then
      new_content=$($clipman_cli get $item_id | columnize_stdin_with_datatime)
      if [[ $? -eq 0 ]]
      then
        $clipman_cli del $item_id
        echo "$new_content" | myclip
        visual_notify "last_item column formated: $($clipman_cli get_last_item_id)"
      fi
    fi
    ;;
  *)
    echo "nothing"
    ;;
esac
