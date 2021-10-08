#!/usr/bin/env bash
nb_item=$1


append_items()
{
  local start_index=$1
  local nb_item=$2
  local i
  for i in $(seq $((start_index+1)) $((start_index+$nb_item)))
  do
    v=item_${i}_$RANDOM
    echo "$v => $(./clipman_cli.sh add $v)"
  done
}

list=$(./clipman_cli.sh list)
nb_line=$(echo "$list" | wc -l)
last_index=$(echo "$list" | awk ' BEGIN {max=0;} /item_/ { if($1 > max) { max = $1; } } END { print max; }')
last_inserted_index=$(echo "$list" | awk -F _ 'BEGIN {max=0;} /item_/ { if($2 > max) { max = $2; } } END { print max; }')

if [[ -z $last_inserted_index ]]
then
  last_inserted_index=0
fi

echo "nb_item $nb_line last_index $last_index last_inserted_index $last_inserted_index"
tmp=$(mktemp)
append_items $last_inserted_index $nb_item > $tmp
pr -m -t $tmp <(./clipman_cli.sh list)
rm $tmp
