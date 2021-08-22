#!/bin/bash
# xfce4-clipman-history -l

if [[ $1 == '-l' ]]
then
  shift
fi

show_secure_item=0
if [[ $1 == '--show-secure-item' ]]
then
  shift
  show_secure_item=1
fi

fake_history='
xfce4-clipman-history
delete an entry
~/.cache/xfce4/clipman
1.6.1git-c71f6f7
C: #no just kiding
https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/25
IMAGE
via the command line helper to manage clipboard history:\n * delete an entry by ID: `xfce4-clipman-history -d 123` (delete entry numbered 123 from history)\n *   * delete an entry by content: `xfce4-clipman-history -d -c "$password_value"`\n *     * list entries: `xfce4-clipman-history -l` (output text entry with id as prefix)\n *       * set an item of the clipman history as `secure`: `xfce4-clipman-history -s 123` (make item number 123 as\n *       secure)\n *         * insert a secure item directly: `xfce4-clipman-history -a -s "$password"` (output the new id inserted\n *         item) \n
'

#fake_password=$(pwqgen)
#id=$(($RANDOM % 200))

fake_password=weapon5Riot2Cold
id=58

IFS=$'\n'
for l in $fake_history
do
  len=${#l}

  if [[ $len -lt 80 ]]
  then
    echo "$id $l"
  else
    echo "$id LONG_TEXT($len) ${l:0:40} ..."
  fi
  id=$(($id + 1))
done


if [[ $show_secure_item -eq 0 ]]
then
  echo "$id SECURE ************"
else
  echo "$id $fake_password"
fi

