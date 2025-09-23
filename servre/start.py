from http.server import SimpleHTTPRequestHandler, HTTPServer

# 定义服务器地址和端口
port = 9000
server_address = ('localhost', port)

# 创建 HTTP 服务器
httpd = HTTPServer(server_address, SimpleHTTPRequestHandler)

# 启动服务器
print("Start Server: ", server_address)
httpd.serve_forever()