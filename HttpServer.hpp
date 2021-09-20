#pragma once

#include <pthread.h>
#include "Sock.hpp"
#include "Protocol.hpp"
#include "ThreadPool.hpp"

#define PORT 8081

class HttpServer{
    private:
        int port;
        int lsock;
        ThreadPool *tp;
        static HttpServer *http_svr;
        static pthread_mutex_t lock;

        //HttpServer()
        //{}
    public:
        HttpServer(int _p = PORT)
            :port(_p),lsock(-1),tp(nullptr)
        {}

        static HttpServer *GetInstance(int sk)
        {
            if(nullptr == http_svr){
                pthread_mutex_lock(&lock);
                if(nullptr == http_svr){
                    http_svr = new HttpServer(sk);
                }
                pthread_mutex_unlock(&lock);
            }
            return http_svr;
        }

        void InitServer()
        {
            signal(SIGPIPE, SIG_IGN);
            lsock = Sock::Socket(); // 创建套接字
            Sock::SetSockOpt(lsock);// 将套接字设置为可复用状态
            Sock::Bind(lsock, port);// 绑定端口号
            Sock::Listen(lsock);

            tp = new ThreadPool();
            tp->InitThreadPool();
        }

        void Start()
        {
            for(;;){
                int sock = Sock::Accept(lsock);
                if(sock < 0){
                    continue;
                }
                LOG(Notice, "get a new link ...");
                Task *tk = new Task(sock);
                tp->PushTask(tk);
                //demo
                //pthread_t tid;
                //int *sockp = new int(sock);
                //pthread_create(&tid, nullptr, Entry::HandlerHttp, sockp); // 要让线程获取请求，分析请求，构建响应，发送响应
                //pthread_detach(tid);  // 线程分离: 防内存泄漏 & 主线程阻塞
            }
        }

        ~HttpServer()
        {
            if(lsock >= 0){
                close(lsock);
            }
        }
};

HttpServer *HttpServer::http_svr = nullptr;
pthread_mutex_t HttpServer::lock = PTHREAD_MUTEX_INITIALIZER;



