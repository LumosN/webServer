#pragma once

#include "Sock.hpp"
#include "Log.hpp"
#include "Util.hpp"

// /a/b/c.html
#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"
#define VERSION "HTTP/1.0"

static std::string CodeToDesc(int code)
{
    std::string desc;
    switch(code){
        case 200:
            desc="OK";
            break;
        case 404:
            desc="Not Found";
            break;
        default:
            desc="OK";
            break;
    }

    return desc;
}

static std::string SuffixToDesc(const std::string &suffix)
{
    if(suffix == ".html" || suffix == ".htm"){
        return "text/html";
    }
    else if(suffix == ".js"){
        return "application/x-javascript";
    }
    else if(suffix == ".css"){
        return "text/css";
    }
    else if(suffix == ".jpg"){
        return "image/jpeg";
    }
    else{
        return "text/html";
    }
}

class HttpRequest{
    private:
        std::string request_line; // 请求行
        std::vector<std::string> request_header; // 请求报头
        std::string blank; // 空行
        std::string request_body; // 正文
    private:
        std::string method;
        std::string uri;
        std::string version;

        std::string path;
        std::string query_string;

        std::unordered_map<std::string,std::string> header_kv;

        ssize_t content_length;

        // 两种情况需要cgi：
		// 1、需要读取正文——POST方法且有content_length
		// 2、GET方法&带参
		bool cgi; 


        ssize_t file_size;

        std::string suffix;
    public:
        HttpRequest()
            :blank("\n"),content_length(-1),\
             path(WEBROOT),cgi(false),suffix("text/html")
        {}
        void SetRequestLine(const std::string &line)
        {
            request_line = line;
        }
        void InsertHeaderLine(const std::string &line)
        {
            request_header.push_back(line);
            LOG(Notice, line);
        }
        void RequestLineParse() // 对请求行解析
        {
            Util::StringParse(request_line, method, uri, version);  // 把一个字符串解析成三个信息
           // LOG(Notice, request_line);
           // LOG(Notice, method);
           // LOG(Notice, uri);
           // LOG(Notice, version);
        }
        void RequestHeaderParse() // 对报头解析
        {
            for(auto it = request_header.begin(); it != request_header.end(); it++){
                std::string k,v;
                Util::MakeStringToKV(*it, k, v);
                LOG(Notice, k);
                LOG(Notice, v);
                if(k == "Content-Length"){
                    content_length = Util::StringToInt(v);
                }
                header_kv.insert({k,v});
            }
        }
        bool IsNeedRecvBody() // 是否需要读取正文
        {
            //Post,PoSt, POSt
			// strcasecmp 忽略大小写比较字符串
            if(strcasecmp(method.c_str(),"POST") == 0 && content_length > 0){ // 是post方法，并且content_length大于0
                cgi = true;
                return true;
            }

            return false;
        }
        ssize_t GetContentLength()
        {
            return content_length;
        }
        void SetRequestBody(const std::string &body)
        {
            request_body = body;
        }
        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(), "POST") == 0 ||\
                    strcasecmp(method.c_str(), "GET") == 0){
                return true;
            }
            return false;
        }
        bool IsGet()
        {
            return strcasecmp(method.c_str(), "GET") == 0 ? true : false;
        }
        bool IsPost()
        {
            return strcasecmp(method.c_str(), "POST") == 0 ? true : false;
        }
        void UriParse()
        {
            //uri存在
            std::size_t pos = uri.find('?');
            if(pos == std::string::npos){
                path += uri;
            }
            else{
                path += uri.substr(0, pos);
                query_string = uri.substr(pos+1);
                cgi = true;
            }
        }
        void SetUriEqPath()
        {
            path += uri;
        }
        
        void IsAddHomePage() // 是否需要添加首页
        {
            //path / or /a/b/c.html or /s
            if(path[path.size()-1] == '/'){
                path += HOMEPAGE;
            }
        }
        std::string GetPath()
        {
            return path;
        }
        std::string SetPath(std::string _path)
        {
            path = _path;
        }
        void SetCgi()
        {
            cgi = true;
        }
        bool IsCgi()
        {
            return cgi;
        }
        void SetFileSize(ssize_t s)
        {
            file_size = s;
        }
        ssize_t GetFileSize()
        {
            return file_size;
        }
        std::string GetQueryString()
        {
            return query_string;
        }
        std::string GetBody()
        {
            return request_body;
        }
        std::string GetMethod()
        {
            return method;
        }
        std::string MakeSuffix()
        {
            std::string suffix;
            std::size_t pos = path.rfind(".");
            if(std::string::npos != pos){
                suffix = path.substr(pos);
            }
            return suffix;
        }
        ~HttpRequest()
        {}
};
class HttpResponse{
    private:
        std::string status_line;
        std::vector<std::string> response_header;
        std::string blank;
        std::string response_body;
    public:
        HttpResponse():blank("\r\n")
        {}
        void SetStatusLine(const std::string &sline)
        {
            status_line = sline;
        }
        std::string GetStatusLine()
        {
            return status_line;
        }
        const std::vector<std::string>& GetRspHeader()
        {
            response_header.push_back(blank);
            return response_header;
        }
        void AddHeader(const std::string &ct)
        {
            response_header.push_back(ct);
        }
        ~HttpResponse()
        {}
};

// 处理服务器中所有的数据请求
class EndPoint{
    private:
        int sock;
        HttpRequest req;
        HttpResponse rsp;
    private:
        void GetRequestLine() // 获取请求行
        {
            std::string line; 
            Sock::GetLine(sock, line); // 获取
            req.SetRquestLine(line); 
            req.RequestLineParse(); //解析
        }
        void SetResponseStatusLine(int code)
        {
            std::string status_line;
            status_line = VERSION;
            status_line += " ";
            status_line += std::to_string(code);
            status_line += " ";
            status_line += CodeToDesc(code);
            status_line += "\r\n";

            rsp.SetStatusLine(status_line);
        }
        void GetRequestHeader()
        {
            std::string line;
            do{
                line="";
                Sock::GetLine(sock, line);
                req.InsertHeaderLine(line);
            }while(!line.empty());

            req.RequestHeaderParse();
        }
        void GetRequestBody()
        {
            ssize_t len = req.GetContentLength();
            char c;
            std::string body;
            while(len){
                ssize_t s = recv(sock, &c, 1, 0);
                body.push_back(c);
                len--;
            }
            req.SetRequestBody(body);
        }

        void SetReaponseHeaderLine()
        {
            std::string suffix = req.MakeSuffix();
            std::string content_type = "Content-Type: ";
            content_type += SuffixToDesc(suffix);
            content_type += "\r\n";
            rsp.AddHeader(content_type);
        }
    public:
        EndPoint(int _sock):sock(_sock)
        {}
        void RecvRequest()
        {
            //获取完整http请求
            //分析http请求
            //读取并分析完毕第一行
            GetRequestLine();
            //读取报头 && blank
            GetRequestHeader();
            //是否需要读取正文
            if(req.IsNeedRecvBody()){
                //yes
                GetRequestBody();
            }
            //已经读完了所有的请求
            
        }
        void MakeResponse()
        {
            //开始分析
            //只处理GET and POST
            int code = 200;
            ssize_t size = 0; //file_size
            std::string path;
            if(!req.IsMethodLegal()){
                LOG(Warning, "method is not legal!");
                code = 404;
                goto end;
            }
            if(req.IsGet()){
                req.UriParse();
            }
            else{
                //POST
                req.SetUriEqPath();
            }
            req.IsAddHomePage();
            //如下三种情况
            //get && 没有参数 -> path
            //get && 有参数   -> path && query_string
            //POST -> uri -> path && body
            path = req.GetPath();
            LOG(Notice, path);
            struct stat st; // stat 获取文件属性
            if(stat(path.c_str(), &st) < 0){
                LOG(Warning, "html is not exist! 404");
                code = 404;
                goto end;
            }
            else{ //文件存在
                if(S_ISDIR(st.st_mode)){ 
					//是一个目录
                    // /a/b/c/index.html
                    path += "/";
                    req.SetPath(path);
                    req.IsAddHomePage();
                }
                else{
                    if((st.st_mode & S_IXUSR) ||\
                       (st.st_mode & S_IXGRP) ||\
                       (st.st_mode & S_IXOTH)){
                        req.SetCgi();
                    }
                    else{
                        //正常的网页请求
                        //Do Nothing
                    }
                }
                if(!req.IsCgi()){ // 非cgi，才需要设置文件大小
                    req.SetFileSize(st.st_size);
                }
            }
end:
            //制作response
            SetResponseStatusLine(code);
            SetReaponseHeaderLine();
            //TODO
        }
        void ExecNonCgi(const std::string &path) // 以非CGI方式运行
        {
            ssize_t size = req.GetFileSize();
            int fd = open(path.c_str(), O_RDONLY);
            if(fd < 0){
                LOG(Error, "path is not exists bug!!!");
                return;
            }
            sendfile(sock, fd, nullptr, size);
            close(fd);
        }
        void ExecCgi()
        {
           // std::string arg;
           // if(req.IsGet()){
           //     arg = req.GetQueryString();
           // }
           // else{
           //     arg = req.GetBody();
           // }
            std::string content_length_env;
            std::string path = req.GetPath();
            std::string method = req.GetMethod();
            std::string method_env="METHOD=";
            method_env += method;

            LOG(Notice, method);
            std::string query_string;
            std::string query_string_env;

            std::string body;
            int pipe_in[2] = {0};
            int pipe_out[2] = {0};

            //站在被调用进程的角度
            pipe(pipe_in);
            pipe(pipe_out);

            putenv((char*)method_env.c_str());
            pid_t id = fork();
            if(id == 0){
                //child
                close(pipe_in[1]); //从管道里读，要关闭写端
                close(pipe_out[0]); // 将运行完的数据写回管道，因此要关闭读端

                dup2(pipe_in[0], 0); // 重定向
                dup2(pipe_out[1], 1);

                //exec
                //第一种通过环境变量给子进程 -> GET -> query_string
                if(req.IsGet()){
                    query_string = req.GetQueryString();
                    query_string_env = "QUERY_STRING=";
                    query_string_env += query_string;
                    putenv((char*)query_string_env.c_str());
                }else if(req.IsPost()){
                    content_length_env = "CONTENT-LENGTH=";
                    content_length_env += std::to_string(req.GetContentLength());
                    LOG(Notice, content_length_env);
                    putenv((char*)content_length_env.c_str());
                }
                else{
                    //TODO
                }

                execl(path.c_str(), path.c_str(), nullptr);
                exit(0);
            }

			//father
            close(pipe_in[0]);
            close(pipe_out[1]);
            //第二种通过管道传递给子进程 -> POST -> body
            char c = 'X';
            if(req.IsPost()){
                body = req.GetBody();
                LOG(Notice, body);
                int i = 0;
                for(; i < body.size(); i++){
                    write(pipe_in[1], &body[i], 1);
                }
            }
            ssize_t s = 0;
            do{
                s = read(pipe_out[0], &c, 1);
                if(s > 0){
                    send(sock, &c, 1, 0);
                }
            }while(s > 0);

            waitpid(id, nullptr, 0);
        }
        void SendResponse()
        {
            std::string line = rsp.GetStatusLine();
            send(sock, line.c_str(), line.size(), 0);
            const auto &header = rsp.GetRspHeader();
            auto it = header.begin();
            for(; it != header.end(); it++){
                send(sock, it->c_str(), it->size(), 0);
            }

            if(req.IsCgi()){
                //yes->cgi
                LOG(Notice, "use cgi model!");
                ExecCgi();
            }
            else{
                LOG(Notice, "use non cgi model!");
                std::string path = req.GetPath();
                ExecNonCgi(path);
            }
        }
        ~EndPoint()
        {
            if(sock >= 0){
                close(sock);
            }
        }
};

class Entry{
    public:
        static void HandlerHttp(int sock)
        {
            //int sock = *(int*)arg; delete (int*)arg;

// 条件编译
#ifdef DEBUG
            char request[10240];
            recv(sock, request, sizeof(request), 0);
            close(sock);
#else
            EndPoint *ep = new EndPoint(sock);
            ep->RecvRequest(); // 读取请求
            ep->MakeResponse();// 制作响应
            ep->SendResponse();// 发送响应

            delete ep;
#endif

        }
};




