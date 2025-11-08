#!/bin/bash

# Modify this if you are building the container for a different hostname
RELAY_HOSTNAME=${HOSTNAME}

CONTAINER_NAME="busstop_relay_instance"
IMAGE_NAME="busstop_relay"
KEYS_DIR="keys"
ROOT_KEY="$KEYS_DIR/root.key"
ROOT_CRT="$KEYS_DIR/root.crt"

# Check if root key/cert exist
if [ ! -f "$ROOT_KEY" ] || [ ! -f "$ROOT_CRT" ]; then
  echo "Root key/cert missing. Regenerating all keys/certs..."
  rm -f "$KEYS_DIR"/*.key "$KEYS_DIR"/*.crt "$KEYS_DIR"/*.csr "$KEYS_DIR"/*.srl
  mkdir -p "$KEYS_DIR"
  # Generate root CA key and certificate
  openssl genrsa -out "$ROOT_KEY" 4096
  openssl req -x509 -new -nodes -key "$ROOT_KEY" -sha256 -days 3650 -out "$ROOT_CRT" -subj "/CN=ArduinoBusStopRoot"
  # Generate relay key and CSR
  openssl genrsa -out "$KEYS_DIR/relay.key" 2048
  openssl req -new -key "$KEYS_DIR/relay.key" -out "$KEYS_DIR/relay.csr" -subj "/CN=${RELAY_HOSTNAME}"
  # Sign relay certificate with root CA
  openssl x509 -req -in "$KEYS_DIR/relay.csr" -CA "$ROOT_CRT" -CAkey "$ROOT_KEY" -CAcreateserial -out "$KEYS_DIR/relay.crt" -days 965 -sha256
  # Generate node key and CSR
  openssl genrsa -out "$KEYS_DIR/node.key" 2048
  openssl req -new -key "$KEYS_DIR/node.key" -out "$KEYS_DIR/node.csr" -subj "/CN=ArduinoBusStopNode"
  # Sign node certificate with root CA
  openssl x509 -req -in "$KEYS_DIR/node.csr" -CA "$ROOT_CRT" -CAkey "$ROOT_KEY" -CAcreateserial -out "$KEYS_DIR/node.crt" -days 965 -sha256
  chmod 600 "$KEYS_DIR"/*.key

else
  # Check and generate relay key/cert if missing
  if [ ! -f "$KEYS_DIR/relay.key" ] || [ ! -f "$KEYS_DIR/relay.crt" ]; then
    echo "Relay key/cert missing. Regenerating relay..."
    openssl genrsa -out "$KEYS_DIR/relay.key" 2048
    openssl req -new -key "$KEYS_DIR/relay.key" -out "$KEYS_DIR/relay.csr" -subj "/CN=${RELAY_HOSTNAME}"
  openssl x509 -req -in "$KEYS_DIR/relay.csr" -CA "$ROOT_CRT" -CAkey "$ROOT_KEY" -CAcreateserial -out "$KEYS_DIR/relay.crt" -days 965 -sha256
    chmod 600 "$KEYS_DIR/relay.key"
  fi
  # Check and generate node key/cert if missing
  if [ ! -f "$KEYS_DIR/node.key" ] || [ ! -f "$KEYS_DIR/node.crt" ]; then
    echo "Node key/cert missing. Regenerating node..."
    openssl genrsa -out "$KEYS_DIR/node.key" 2048
    openssl req -new -key "$KEYS_DIR/node.key" -out "$KEYS_DIR/node.csr" -subj "/CN=ArduinoBusStopNode"
  openssl x509 -req -in "$KEYS_DIR/node.csr" -CA "$ROOT_CRT" -CAkey "$ROOT_KEY" -CAcreateserial -out "$KEYS_DIR/node.crt" -days 965 -sha256
    chmod 600 "$KEYS_DIR/node.key"
  fi
fi

# Stop and remove the existing container if it exists
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Stopping and removing existing container..."
  docker stop --timeout 1 "$CONTAINER_NAME"
  docker rm "$CONTAINER_NAME"
fi

# build the Docker image
docker build -t "$IMAGE_NAME" .

# Start a new container with persistent volume
docker run -d --restart unless-stopped --name "$CONTAINER_NAME" -p 443:443 "$IMAGE_NAME"

echo "Container built and started as $CONTAINER_NAME."
