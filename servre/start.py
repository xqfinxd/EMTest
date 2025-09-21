from http.server import SimpleHTTPRequestHandler, HTTPServer

# 定义服务器地址和端口
server_address = ('', 9000)

# 创建 HTTP 服务器
httpd = HTTPServer(server_address, SimpleHTTPRequestHandler)

# 启动服务器
httpd.serve_forever()