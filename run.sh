trap ctrl_c INT

function ctrl_c() {
        echo "CTRL-C stop lcd2004"
	systemctl stop lcd2004
}


systemctl stop lcd2004
rm /usr/bin/LCD2004

gcc src/LCD2004d.c src/lib/mcp3008.c -o /usr/bin/LCD2004

systemctl start lcd2004
systemctl status lcd2004
tail -f /var/log/syslog

