#!/bin/sh
#
# File:   start-on-target.sh
# Author: Fulup Ar Foll @ IoT.bzh
# Object: Forward signal (SIGTERM) to remote process
# Created on 24-May-2017, 09:23:37
# Usage: remote-target-populate update script under ./build directory

# Do not change manually use 'make remote-target-populate'
export RSYNC_TARGET=10.20.101.198
export PROJECT_NAME=afb-aaaa
export RSYNC_PREFIX=opt/afb-aaaa
export AFB_REMPORT=1234
export AFB_TOKEN=

exec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -tt $RSYNC_TARGET << EOF
    afb-daemon --workdir=$RSYNC_PREFIX --port=$AFB_REMPORT --roothttp=./htdocs --ldpath=./lib --verbose --token=$AFB_TOKEN &
    PID_DAEMON=\$!

    trap "echo REMOTE-SIGNAL TRAP; kill -15 \$PID_DAEMON" INT QUIT TERM EXIT

    # wait for daemon to finish
    wait \$PID_DAEMON
    exit
EOF
