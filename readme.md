# webserver

用C++实现Proactor下的LT模式的高性能WEB服务器

## 功能

- 利用I/O复用技术Epoll与线程池实现多线程的Reactor高并发模型；
- 利用正则与状态机解析HTTP请求报文，可以解析的文件类型有html、png、mp4等；
- 利用标准库容器封装char，实现自动增长的缓冲区；
- 实现GET、POST方法的部分内容的解析，处理POST请求，实现计算功能；

## 功能优化

- 定时检测不活跃的连接，并把它释放
  - 如果客户端不主动去关闭或者请求没有错误，请求是不会主动关闭的，定时的检测不活跃的连接，把不活跃的释放掉，文件描述符是有限的，如果客户端连接特别多又不断开文件描述符用完了再有新的客户来就连不进了。
- 进行服务器压力测试
  - 测试处在相同硬件上，不同服务的性能以及不同服务的性能不同硬件上同一个服务的运行状况。
  - 展示服务器的两项内容：每秒钟响应请求数和每秒钟传输数据量。
  - webbench -c 1000 -t 30 http://192.168.110.129:10000/index.html

## 致谢

Linux高性能服务器编程，游双著。 参考牛客WebServer服务器项目。