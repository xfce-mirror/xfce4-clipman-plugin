#!/usr/bin/env bash
#
# rofi menu for clipman_cli actions
#
# GUI interface for pre-defined action orchestrated via clipman_cli
#

# actions code will be based on the first word of the following menu
options="
🔐 secure next copy
🚿 clean fancy UTF-8
🔗 merge two entries
📎 markdown merge two entries
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

columnize_stdin_with_datatime()
{
  timeout 1s sed -e 's/\([0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}\) \([0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\)/\1_\2/g' | column -t
}

replace_last_clipboard()
{
  local last_id=$(add_clipboard "$@")
  if [[ -n $last_id ]]
  then
    $clipman_cli del $last_id
  fi
}

# call: add_clipboard [--html] content [msg]
add_clipboard()
{
  local html=""
  if [[ $# -ge 2 && $1 == '--html' ]]
  then
    html='--html'
    shift
  fi

  local content="$1"
  local msg="$2"
  if [[ -z $msg ]]
  then
    msg="clipboard updated"
  fi

  # same content detection
  local last_id=$($clipman_cli get_last_item_id)
  local old_content=$($clipman_cli get $last_id)

  if [[ $old_content != $content ]]
  then
    # cannot catch $? on local definition must be two line call
    local new_id
    new_id=$($clipman_cli add $html "$content")

    if [[ $? -eq 0 ]]
    then
      echo "$last_id" | awk '{print $2}'
      visual_notify "$msg: $new_id"
    else
      visual_notify "😞clipman_cli add error"
    fi
  else
    visual_notify "unchanged"
  fi
}

make_link()
{
  local mode=$1
  local text="$2"
  local url="$3"

  # transform some based on URL termination match
  regexp='/(([0-9]+)|((CWIT|PO)-[0-9]+))$'
  if [[ $url =~ $regexp ]]
  then
    text="$text (#${BASH_REMATCH[1]})"
  fi

  local out
  case $mode in
    markdown)
      out="[$text]($url)"
      ;;
    *)
      out="<a href=\"$url\">$text</a>"
  esac
  echo "$out"
}

open_rofi_menu()
{
  # filter options removing empty lines
  local filtered_options=$(echo "$options" | sed -n -e '/^./p')
  local nb=$(echo "$filtered_options" | wc -l)
  local rofi_theme=Paper
  local old_IFS=$IFS
  IFS=$'\n'
  local r=$(echo "$filtered_options" \
    | rofi -lines $nb -dmenu -p "custom clipman behavior" -theme $rofi_theme \
    | awk '{print $2}'
  )
  IFS=$old_IFS
  echo "$r"
}

SCRIPT_DIR=$(dirname $(realpath $0))
clipman_cli=$SCRIPT_DIR/../panel-plugin/clipman_cli.sh
transform_clipboard=$SCRIPT_DIR/transform_clipboard.py
remove_fancy_utf_8_char=$SCRIPT_DIR/remove_fancy_utf-8_char.py

if [[ $# -ge 1 ]]
then
  action=$1
else
  # no argument default behavbior
  action=$(open_rofi_menu)
fi

# action is based on the first word of the menu
case $action in
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
      replace_last_clipboard "$html_content"
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
  clean)
    cleaned="$($clipman_cli get_clipboard | python3 $remove_fancy_utf_8_char)"
    # one argument
    # also prefix with timestamp
    replace_last_clipboard "$(date "+%Y-%m-%d %H:%M:%S")"$'\n'"$cleaned"
    ;;
  columnize)
    item_id=$($clipman_cli get_last_item_id)
    if [[ -n $item_id ]]
    then
      new_content=$($clipman_cli get $item_id | columnize_stdin_with_datatime)
      replace_last_clipboard "$new_content" "last_item column formated"
    fi
    ;;
  merge|markdown)
    last_item_id=$($clipman_cli get_last_item_id)
    prev_last_item_id=$($clipman_cli get_last_item_id 1)
    last_item=$($clipman_cli get $last_item_id)
    prev_last_item=$($clipman_cli get $prev_last_item_id)
    out_clipboard='--html'
    if [[ $prev_last_item  == http* ]]
    then
      new_value=$(make_link $action "$last_item" "$prev_last_item")
    else
      new_value=$(make_link $action "$prev_last_item" "$last_item")
    fi
    if [[ $action == 'markdown' ]]
    then
      out_clipboard=""
    fi
    add_clipboard $out_clipboard "$new_value" "last two item merged"
    ;;
  *)
    echo "unkwon action: '$action' Done nothing"
    exit 1
    ;;
esac
