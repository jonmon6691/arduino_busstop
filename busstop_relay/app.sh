#!/bin/sh
# Simple startup script for Docker container

# Ensure persistent home directory exists
mkdir -p /root
cat << "EOF"
                A R D U I N O
  ____  _    _  _____ _____ _______ ____  _____  
 |  _ \| |  | |/ ____/ ____|__   __/ __ \|  __ \ 
 | |_) | |  | | (___| (___    | | | |  | | |__) |
 |  _ <| |  | |\___ \\___ \   | | | |  | |  ___/ 
 | |_) | |__| |____) |___) |  | | | |__| | |     
 |____/ \____/|_____/_____/   |_|  \____/|_|     
                  R E L A Y

Data directory: /app
EOF

# Run relay.py and restart if it crashes, unless stopped
while true; do
	python3 /app/relay.py
	echo "relay.py crashed. Restarting in 5 seconds..."
	sleep 5
done
