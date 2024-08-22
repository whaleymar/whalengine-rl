import http.server
from http.server import SimpleHTTPRequestHandler
import socketserver

class CORSRequestHandler(SimpleHTTPRequestHandler):
    # ENABLES SHAREDARRAYBUFFER, WHICH ALLOWS MULTITHREADING, BUT IS NOT SUPPORTED BUT ITCH.IO
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

PORT = 8080

# with socketserver.TCPServer(("", PORT), CORSRequestHandler) as httpd:
with socketserver.TCPServer(("", PORT), SimpleHTTPRequestHandler) as httpd:
    print("Serving at port", PORT)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("Shutting down")
        httpd.server_close()
    
