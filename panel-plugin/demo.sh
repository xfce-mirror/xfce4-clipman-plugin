#!/usr/bin/env bash

clipman_cli()
{
  local cmd=$*
  echo "$ ./clipman_cli.sh $cmd"
  eval "./clipman_cli.sh $@"
  echo
}

./clipman_cli.sh clear

# populate history
(
  ./clipman_cli.sh add "This version of clipman is a PoC"
  ./clipman_cli.sh add "DBus method API"
  ./clipman_cli.sh add "xfce4-clipman-plugin fork"
  ./clipman_cli.sh add "'https://gitlab.xfce.org/Sylvain/xfce4-clipman-plugin'"
) > /dev/null

clear

clipman_cli list
clipman_cli add "'mylogin@xfce.org'"
clipman_cli add -s '"$(pwqgen)"'
clipman_cli list

id=$(./clipman_cli.sh list | awk '/SECURE/ { print $1; exit;}')
clipman_cli get $id
clipman_cli del $id
clipman_cli list
