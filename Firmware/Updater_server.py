from http.server import HTTPServer, BaseHTTPRequestHandler

FW_VERSION = "2.6"  # change this to match your new firmware version

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/version":
            self.send_response(200)
            self.end_headers()
            self.wfile.write(FW_VERSION.encode())
        elif self.path == "/firmware.bin":
            with open("firmware.bin", "rb") as f:
                data = f.read()
            self.send_response(200)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        else:
            self.send_response(404)
            self.end_headers()
    def log_message(self, format, *args):
        print(f"[{self.address_string()}] {format % args}")

HTTPServer(("0.0.0.0", 8080), Handler).serve_forever()
