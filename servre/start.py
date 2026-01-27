# 使用Python代码启动
import http.server
import socketserver

PORT = 9000

Handler = http.server.SimpleHTTPRequestHandler

with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
    print(f"服务器已启动，访问地址: http://localhost:{PORT}")
    httpd.serve_forever()