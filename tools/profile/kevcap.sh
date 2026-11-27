#!/bin/sh
# Where a running program's busiest thread spends its time on the phone: records
# QNX's kernel event trace while you use the app, decodes it on this machine
# and summarizes it with kevsum.py.
#
#   tools/profile/kevcap.sh HOST PROCESS [SECONDS] [TID]
#
# HOST is the phone's ssh name, with a root login (tracelogger needs it).
# PROCESS is matched against the program's path as pidin shows it. The trace
# is of the whole system and goes to RAM on the phone (/dev/shmem), so keep it
# short: 3 s by default. The .kev and .txt files stay in OUT (default .).
# Decoding uses traceprinter from an NDK image (TRACE_IMAGE).
set -eu
[ $# -ge 2 ] || { sed -n '2,12s/^# \{0,1\}//p' "$0"; exit 2; }
HOST=$1 NAME=$2 SECS=${3:-3} TID=${4:-}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$(cd "${OUT:-.}" && pwd)
IMAGE=${TRACE_IMAGE:-yamsergey/bb10-ndk:0.6}
TRACEPRINTER=/root/bbndk/host_10_3_1_12/linux/x86/usr/bin/traceprinter
BASE=kev-$(date +%Y%m%d-%H%M%S)
KEV=/dev/shmem/$BASE.kev

PID=$(ssh "$HOST" "pidin -F '%a %N'" | awk -v name="$NAME" 'index($2, name) { print $1; exit }')
[ -n "$PID" ] || { echo "no process matching $NAME on $HOST" >&2; exit 1; }
echo "pid $PID: recording $SECS s..."
trap 'ssh "$HOST" "rm -f $KEV"' EXIT
ssh "$HOST" "tracelogger -w -s $SECS -f $KEV"
scp -q "$HOST:$KEV" "$OUT/$BASE.kev"
docker run --rm --platform linux/386 -v "$OUT:/t" --entrypoint "$TRACEPRINTER" "$IMAGE" \
    -f "/t/$BASE.kev" >"$OUT/$BASE.txt"
python3 "$HERE/kevsum.py" "$OUT/$BASE.txt" "$PID" $TID
