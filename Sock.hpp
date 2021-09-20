#pragma once

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "Log.hpp"

#define BACKLOG 5

class Sock{
    public:
        static int Socket()
        {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if(sock < 0){
                LOG(Fatal, "socket create error"); // 日志信息
                exit(SocketErr);
            }
            return sock;
        }
        static void Bind(int sock, int port) // 绑定
        {
            struct sockaddr_in local;
            bzero(&local, sizeof(local));

            local.sin_family = AF_INET;
            local.sin_port   = htons(port); // 将整型变量主机字节顺序转网络字节顺须
            local.sin_addr.s_addr = htonl(INADDR_ANY); // 主机转网络

            if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
                LOG(Fatal, "socket bind error");
                exit(BindErr);
            }
        }
        static void Listen(int sock) // 监听
        {
            if(listen(sock, BACKLOG) < 0){
                LOG(Fatal, "socket listen error");
                exit(ListenErr);
            }
        }
        static void SetSockOpt(int sock) // 设置套接字为复用状态
        {
            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        }
        static int Accept(int sock) // 获取新连接
        {
            struct sockaddr_in peer; // 远端
            socklen_t len = sizeof(peer);
            int s = accept(sock, (struct sockaddr*)&peer, &len);
            if(s < 0){  
                LOG(Warning, "accept error");
            }

            return s;
        }
        //作用：整体读取一行内容
        static void GetLine(int sock, std::string &line)
        {
            //\n,\r,\r\n -> \n
            //按字符读取
            char c = 'X';
            while(c != '\n'){
                ssize_t s = recv(sock, &c, 1, 0);// 0阻塞式读取
                if(s > 0){
                    if(c == '\r'){
                        ssize_t ss = recv(sock, &c, 1, MSG_PEEK); // MSG_PEEK读上来但不取走（数据窥探）
                        if(ss > 0 && c == '\n'){ // 说明分隔符是\r\n，要把\r\n换成\n供上层使用
                            //\r\n
                            recv(sock, &c, 1, 0);
                        }
                        else{
                            //\r
                            c = '\n';
                        }
                    }//if
                    //1. 读取到的内容是常规字符
                    //2. \n
                    //3. \r\n,\r->\n
                    if(c != '\n'){
                        line.push_back(c);
                    }
                }
            }
        }
};







