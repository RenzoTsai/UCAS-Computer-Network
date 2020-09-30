# HTTP服务器/客户端实验

[蔡润泽](https://github.com/RenzoTsai)

2020年9月29日

[本实验 `Github` 地址](https://github.com/RenzoTsai/UCAS-Computer-Network/tree/master/EXP03-socket)

## 实验内容

### 1、使用C语言分别实现最简单的HTTP服务器和HTTP客户端

- 服务器监听`80`端口，收到HTTP请求，解析请求内容，回复HTTP应答

- 对于本地存在的文件，返回`HTTP 200 OK`和相应文件

- 对于本地不存在的文件，返回`HTTP 404 File Not Found` 

### 2、服务器、客户端需要支持HTTP Get方法

### 3、服务器使用多线程支持多路并发

## 设计思路

1、已有代码框架已经能实现socket下的echo，而实现HTTP client和server首先要做的是实现HTTP GET请求与相应。

2、在查询`HTTP GET`请求格式后，了解到`GET`请求报文格式如下图：
![HTTP-request](/EXP03-socket/assets/HTTP-request.png)

因此，本设计在 `http-client.c` 文件里添加了字符串`request_head`来生成请求报文，并在程序运行时通过`scanf`获得用户输入的请求文件路径。在加上请求头部后，`Client`将请求发送给`Server`。

3、`Server` 在收到 `HTTP GET` 请求格式后，了解到响应报文格式如下图：
 ![HTTP-reply](/EXP03-socket/assets/HTTP-reply.png)
因此，本设计在 `http-server.c` 文件里添加了 `int msg_handler(char * msg, char * path)` 函数来解析请求报文，并提取出请求文件的地址。

4、`Server` 在打开目标文件后，读取目标文件的内容，并将内容传递至 `return_message`，`return_message` 此外还会附上必要的响应头部属性最后`Server` 会将`return_message`发送给`Client`。

## 测试项目

### 1、所实现的HTTP服务器、客户端可以完成上述功能

- 使用客户端连续多次获取文件

- 同时启动多个客户端进程分别获取文件

- 使用客户端请求不存在的文件

### 2、使用”python -m SimpleHTTPServer 80”和wget分别替代服务器和客户端，对端可以正常工作

## 遇到的问题

## 参考文献或网站

[1]	https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers

[2]	https://www.cnblogs.com/an-wen/p/11180076.html

