# busstop_relay

This generates a simple docker container which runs a secure API relay
between itself and translink.ca

This was needed because they changed to only supporting TLS1.3 and it seems
that this is working with the current versions of mbedtls integrated with 
the arduino esp library. While I try to untagle that mess, this relay avoids
the problem by providing a TLS1.2 gateway.

## Install

1. Clone this repo on your server and run `busstop_relay/build_and_run.sh`
2. On the machine you're building the arduino code, uncomment `#define USE_CLIENT_CERTIFICATE` in wifi_login.h and follow the instructions there about moving `client_auth.h` 
2. Paste the contents of `busstop_relay/keys/node.crt` into `[systems/client_auth.h] const char *clientCert`
3. Paste the contents of `busstop_relay/keys/node.key` into `[systems/client_auth.h] const char *clientKey`
4. Build and flash the ardino