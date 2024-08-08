import http.server
from http.server import SimpleHTTPRequestHandler
import socketserver

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

PORT = 8080

with socketserver.TCPServer(("", PORT), CORSRequestHandler) as httpd:
    print("Serving at port", PORT)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("Shutting down")
        httpd.server_close()
    
