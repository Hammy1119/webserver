## 项目目的

### Web服务器能够很好的贯穿所学的知识

但是，Web服务器能够很好的贯穿之前所学的知识，之前看过的《C++ Primer》、《Effevtive C++》、《STL源码剖析》、《深度探索C++对象模型》、《TCP\IP详解卷1》、APUE、UNP，还包括了《后台开发核心技术与应用实践》等书，涵盖了

- TCP、HTTP协议
- 多进程多线程
- IO
- 锁
- 通信
- C++语法
- 编程规范
- Linux环境下各种工具的使用
- 版本控制Git
- Makefile和CMakeLists文件的编写
- 自动化构建工具Travis CI的使用

最终的版本在很多方面学习了muduo网络库，在看完陈硕的《Linux多线程服务端编程》后，对照着书把muduo的源码读了几遍，并重构了自己的服务器，最终的很多想法借鉴了muduo的思想。

## 遇到的困难

### 1. 如何设计各个线程个任务

其实我觉的实现上的困难都不算真正的困难吧，毕竟都能写出来，无非是解决bug花的时间的长短。
我遇到的最大的问题是不太理解One loop per thread这句话吧，翻译出来不就是每个线程一个循环，我最开始写的也是一个线程一个循环啊，muduo的实现和我的有什么区别呢？还有怎么设计才能减少竞态？

带着这些问题我看了《Linux多线程服务端编程》，并看完了muduo的源码，这些问题自然而然就解决了

### 2. 异步Log几秒钟才写一次磁盘，要是coredump了，这段时间内产生的log我去哪找啊？

其实这个问题非常简单了，也没花多少时间去解决，但我觉的非常好玩。coredump了自然会保存在core文件里了，无非就是把它找出来的问题了，在这里记录一下。

当然这里不管coredump的原因是什么，我只想看丢失的log。所以模拟的话在某个地方abort()就行

多线程调试嘛，先看线程信息，info thread，找到我的异步打印线程，切换进去看bt调用栈，正常是阻塞在条件变量是wait条件中的，frame切换到threadFunc(这个函数是我的异步log里面的循环的函数名)，剩下的就是print啦～不过，我的Buffer是用智能指针shared_ptr包裹的，直接->不行，gdb不识别，优化完.get()不让用，可能被inline掉了，只能直接从shared_ptr源码中找到_M_ptr成员来打印。

## 线程

一般而言，多线程服务器中的线程可分为以下几类：

- IO线程(负责网络IO)
- 计算线程(负责复杂计算)
- 第三方库所用线程

本程序中的Log线程属于第三种，其它线程属于IO线程，因为Web静态服务器计算量较小，所以没有分配计算线程，减少跨线程分配的开销，让IO线程兼顾计算任务。除Log线程外，每个线程一个事件循环，遵循One loop per thread。

## epoll工作模式

epoll的触发模式在这里我选择了ET模式，muduo使用的是LT，这两者IO处理上有很大的不同。ET模式要比LE复杂许多，它对用户提出了更高的要求，即每次读，必须读到不能再读(出现EAGAIN)，每次写，写到不能再写(出现EAGAIN)。而LT则简单的多，可以选择也这样做，也可以为编程方便，比如每次只read一次(muduo就是这样做的，这样可以减少系统调用次数)。

## 定时器

每个SubReactor持有一个定时器，用于处理超时请求和长时间不活跃的连接。muduo中介绍了时间轮的实现和用stl里set的实现，这里我的实现直接使用了stl里的priority_queue，底层是小根堆，并采用惰性删除的方式，时间的到来不会唤醒线程，而是每次循环的最后进行检查，如果超时了再删，因为这里对超时的要求并不会很高，如果此时线程忙，那么检查时间队列的间隔也会短，如果不忙，也给了超时请求更长的等待时间。

## 并发模型

程序使用Reactor模型，并使用多线程提高并发度。为避免线程频繁创建和销毁带来的开销，使用线程池，在程序的开始创建固定数量的线程。使用epoll作为IO多路复用的实现方式。

## 结构

看了很多Github上别人写的服务器，以及博客上的一些总结，结合自己的理解写出来的。模型结构如下：

- 使用了epoll边沿触发+EPOLLONESHOT+非阻塞IO
- 使用了一个固定线程数的线程池
- 实现了一个任务队列，由条件变量触发通知新任务的到来
- 实现了一个小根堆的定时器及时剔除超时请求，使用了STL的优先队列来管理定时器
- 解析了HTTP的get、post请求，支持长短连接
- mime设计为单例模式
- 线程的工作分配为：
  - 主线程负责等待epoll中的事件，并把到来的事件放进任务队列，在每次循环的结束剔除超时请求和被置为删除的时间结点
  - 工作线程阻塞在条件变量的等待中，新任务到来后，某一工作线程会被唤醒，执行具体的IO操作和计算任务，如果需要继续监听，会添加到epoll中
- 锁的使用有两处：
  - 第一处是任务队列的添加和取操作，都需要加锁，并配合条件变量，跨越了多个线程。
  - 第二处是定时器结点的添加和删除，需要加锁，主线程和工作线程都要操作定时器队列。

第一版的服务器已经相对较完整了，该有的功能都已经具备了

## 项目的重点、难点，以及怎么解决难点的

> - 项目中的重点在于应用层上HTTP协议的使用，怎么去解析HTTP请求，怎么根据HTTP请求去做出应答这样一个流程我觉得是这个项目的重点。
> - 项目的难点我觉得在于如何将一个完整的服务器拆分成多个模块，同时在实现的时候又将其各个部分功能组合在一起。首先一个HTTP服务器要实现的是完成请求应答这样一件事，然后需要考虑到读写数据的缓冲区、同时多个连接需要处理多种事件、连接超时关闭避免资源消耗等这一系列问题，所以就有了各种模块。单独编写模块出来后还要考虑怎么去组合怎么去搭配，互相之间的接口要怎么设计，我觉得是我在项目中遇到的问题。
> - 难点的解决通过自顶向下的设计方式。先确定整体的功能也就是完成一次HTTP传输，再向下考虑需要用到什么组件，然后再依次实现各个组件，最后再将模块组合起来，期间需要多次修改接口。

## 项目实现的效果以及瓶颈和不足

> - 项目实现效果我是部署在单核2G的云服务器上的，跑过压力测试但是实际效果并不理想，只有3000左右的QPS（并发数/平均访问时间），拆测可能跟设计模式的选择还有测试服务器的性能有关系。
> - 该服务器的瓶颈我觉得在于网络设计模式的选择。首先该服务器的网络设计模式是采用的单Reactor多线程模式。就是主线程内循环使用IO多路复用，监听连接并且建立连接，同时也会监听新连接上的读写事件，并将读写和业务逻辑处理分发给多线程进行处理。这么一个设计模式的缺陷在于一个Reactor对象承担了所有事件的监听和响应，当遇到突发的高并发时，往往不能及时处理新连接。

## 针对瓶颈如何改进

> - 针对不足可以改变网络设计模式。即采用多Reactor多线程模型。相比改变前，多Reactor多线程模型中主线程主Reactor只负责监听连接还有建立连接，并且将建立好的连接通过生产者消费者模型传递给子线程里的子Reactor负责监听对应连接上的事件。这样主线程就可以在遇到瞬间并发时也能够及时处理新连接的建立。

## 采用什么网络模型

> - 采用单Reactor多线程模型。主线程是一个Reactor，进行IO多路复用实现连接事件和读写事件的监听，同时主线程负责新连接的建立，子线程则负责数据的读写还有业务逻辑处理。

## HTTP解析怎么实现的（正则表达式和有限状态机）

> - HTTP解析是通过正则表达式还有有限状态机实现的。一个HTTP请求报文是由三部分组成的，请求行、头部字段还有消息体，通过在状态机里重复这三个部分之间的解析跳转，最终可以完成一个完整请求报文的解析。HTTP1.1请求报文是ASCII码的固定文本形式，通过正则表达式去解析可以简化我们的任务量。