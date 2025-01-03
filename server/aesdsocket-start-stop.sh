#!/bin/sh
#
# Starts aesdsocket daemon.
#

case "$1" in
  start)
  	start-stop-daemon -S -n -a /usr/bin/aesdsocket -r
	;;
  stop)
  	start-stop-daemon -K -n aesdsocket
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit 0
