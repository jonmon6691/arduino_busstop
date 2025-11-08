"""
Translink API Relay
-------------------
Relays incoming HTTPS requests to the Translink API using mutual TLS authentication.
This is a workaround for the Arduino's limited TLS capabilities, allowing it to communicate
with the Translink API which only supports TLS 1.3.
"""

import http.server
import socketserver
import urllib.request
import urllib.error
import ssl

# Configuration
TARGET_URL = 'https://getaway.translink.ca'
PORT = 443
CA_CERT = "/app/keys/root.crt"
SERVER_CERT = "/app/keys/relay.crt"
SERVER_KEY = "/app/keys/relay.key"

# SSL context for incoming connections (mutual TLS)
incoming_ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
incoming_ssl_context.load_cert_chain(certfile=SERVER_CERT, keyfile=SERVER_KEY)
incoming_ssl_context.load_verify_locations(cafile=CA_CERT)
incoming_ssl_context.verify_mode = ssl.CERT_REQUIRED
incoming_ssl_context.minimum_version = ssl.TLSVersion.TLSv1_2
incoming_ssl_context.maximum_version = ssl.TLSVersion.TLSv1_2

# SSL context for outgoing requests (default system trust)
outgoing_ssl_context = ssl.create_default_context()

class TranslinkRelayHandler(http.server.BaseHTTPRequestHandler):
    """Handles incoming GET requests and relays them to the Translink API."""
    def do_GET(self):
        target = TARGET_URL + self.path
        print(f"Relaying request to: {target}")
        try:
            req = urllib.request.Request(target, method='GET')
            # Forward relevant client headers (excluding host/connection)
            for header, value in self.headers.items():
                if header.lower() not in ('host', 'connection'):
                    req.add_header(header, value)
            response = urllib.request.urlopen(req, context=outgoing_ssl_context)
            self.send_response(response.status)
            # Copy response headers (filtering problematic ones)
            for header, value in response.getheaders():
                if header.lower() not in ('transfer-encoding', 'connection'):
                    self.send_header(header, value)
            self.end_headers()
            self.wfile.write(response.read())
        except urllib.error.HTTPError as e:
            print(f"HTTP Error: {e.code} for {target}")
            self.send_error(e.code, str(e.reason))
        except Exception as e:
            print(f"General Error: {e}")
            self.send_error(500, f"Internal Relay Error.")

class SSLTCPServer(socketserver.TCPServer):
    """TCPServer subclass that wraps connections with SSL."""
    def get_request(self):
        newsocket, fromaddr = self.socket.accept()
        connstream = incoming_ssl_context.wrap_socket(newsocket, server_side=True)
        return connstream, fromaddr

def main():
    with SSLTCPServer(("", PORT), TranslinkRelayHandler) as httpd:
        print(f"API Relay running on https://localhost:{PORT}")
        print(f"Relaying requests to {TARGET_URL}")
        httpd.serve_forever()

if __name__ == "__main__":
    main()
