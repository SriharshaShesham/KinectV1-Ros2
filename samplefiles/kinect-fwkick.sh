#!/bin/bash
# Launch micview in the background to trigger firmware upload
/usr/local/bin/freenect-micview &
PID=$!

# Give it a few seconds to upload firmware
sleep 5

# Kill micview so the device is free for other apps
kill $PID 2>/dev/null
