#!/bin/sh
#
# Starts aesdsocket daemon.
#

case "$1" in
  start)
  	start-stop-daemon -S -n aesdsocket --exec /usr/bin/aesdsocket  -- -d
	;;
  stop)
  	start-stop-daemon -K -n aesdsocket
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit 0
