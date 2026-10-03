#!/usr/bin/env bash
#
# run-integration-test.sh - start a server on a virtual display and run the
# FeatureRoundTripTest master client against it
#
# Usage: run-integration-test.sh <build dir> <lib dir> <server> <cli> <client>
#
# Needs root (system-wide configuration and keys) and Xvfb. Exits with 77
# (skipped) if either is missing, unless TAFAT_INTEGRATION_REQUIRED=1.
# The system-wide configuration is saved before and restored afterwards.

set -u

BUILD=$1
LIBDIR=$2
SERVER=$3
CLI=$4
CLIENT=$5

KEY_NAME=integrationtest
DISPLAY_NUMBER=:77
SERVER_PORT=11100

skip()
{
	echo "SKIPPED: $1"
	if [ "${TAFAT_INTEGRATION_REQUIRED:-0}" = 1 ] ; then
		exit 1
	fi
	exit 77
}

[ "$(id -u)" = 0 ] || skip "must run as root"
command -v Xvfb > /dev/null || skip "Xvfb not found"

WORK=$(mktemp -d)
XVFB_PID=
SERVER_PID=

cleanup()
{
	[ -n "$SERVER_PID" ] && kill $SERVER_PID 2> /dev/null && wait $SERVER_PID 2> /dev/null
	[ -n "$XVFB_PID" ] && kill $XVFB_PID 2> /dev/null && wait $XVFB_PID 2> /dev/null
	"$CLI" authkeys delete $KEY_NAME/private > /dev/null 2>&1
	"$CLI" authkeys delete $KEY_NAME/public > /dev/null 2>&1
	"$CLI" config clear > /dev/null 2>&1
	[ -s "$WORK/config-backup.json" ] && "$CLI" config import "$WORK/config-backup.json" > /dev/null 2>&1
	rm -rf "$WORK"
}
trap cleanup EXIT

fail()
{
	echo "FAILED: $1"
	if [ -f "$WORK/server.log" ] ; then
		echo "==== server output ===="
		tail -n 100 "$WORK/server.log"
	fi
	exit 1
}

# plugins are looked up in ../<lib dir> relative to the executables
mkdir -p "$BUILD/$LIBDIR"
find "$BUILD/plugins" -name "*.so" -exec ln -sf '{}' "$BUILD/$LIBDIR/" ';'

"$CLI" config export "$WORK/config-backup.json" > /dev/null 2>&1

"$CLI" config set Authentication/Method 1 || fail "could not select key file authentication"
"$CLI" authkeys delete $KEY_NAME/private > /dev/null 2>&1
"$CLI" authkeys delete $KEY_NAME/public > /dev/null 2>&1
"$CLI" authkeys create $KEY_NAME || fail "could not create the authentication keys"

Xvfb $DISPLAY_NUMBER -screen 0 1024x768x24 -nolisten tcp > "$WORK/xvfb.log" 2>&1 &
XVFB_PID=$!
export DISPLAY=$DISPLAY_NUMBER

"$SERVER" > "$WORK/server.log" 2>&1 &
SERVER_PID=$!

for i in $(seq 1 30) ; do
	if (echo > /dev/tcp/127.0.0.1/$SERVER_PORT) 2> /dev/null ; then
		break
	fi
	kill -0 $SERVER_PID 2> /dev/null || fail "server exited"
	sleep 1
done
(echo > /dev/tcp/127.0.0.1/$SERVER_PORT) 2> /dev/null || fail "server does not listen on port $SERVER_PORT"

QT_QPA_PLATFORM=offscreen "$CLIENT" || fail "FeatureRoundTripTest"

echo "PASSED"
