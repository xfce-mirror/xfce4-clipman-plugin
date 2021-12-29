for i in $(seq 21 40) ; do ./clipman_cli.sh add val_$i; done
./clipman_cli.sh set_history_size 6
./clipman_cli.sh list
./clipman_cli.sh add pipo
./clipman_cli.sh list
./clipman_cli.sh set_history_size 40
