/* $begin proxy main */
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* 对于访问LRU更新时，我认为应该对LRU上一把大锁 */
sem_t lrut;
/*
 * data 存储Web Object
 * lruCnt LRU计数器，越大表示近期越久没有访问
 * readCnt 读写者锁变量之一，表示目前访问临界区的读者数量
 * dataSize 记录存储的Web Object字节大小
 * isNew 表示区域是否还未用来存储Web Object
 */
struct cachePart {
    char data[MAX_OBJECT_SIZE];
    int lruCnt, readCnt, dataSize, isNew;
    sem_t rdt, wrt;
    char hostname[MAXLINE], uri[MAXLINE], contentTpye[MAXLINE];
};
struct cachePart *cache;
int cachePartNum;

void initCache();
void *serverThread(void *argp);
void client2Proxy(int connfd);
void proxy2Client(int idx, int connfd);
void proxy2Server(char *hostname, char *port, char *uri, rio_t *connfdRp);
void cacheWebObject(char *cacheBuf, int bodySize, char* hostname, char* uri, char *contentTpye);
/* 参数含义分别为:要写入信息的套接字文件描述符，导致错误的原因，响应体上的status-code，响应体上的status-message, 具体原因 */
void clientError(int fd, char *cause, char *statusCode, char *shortmsg, char *longmsg);

int main(int argv, char **argc)
{
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage addr;
    socklen_t addrlen;
    pthread_t tid;

    /* 检查是否传入必要参数端口 */
    if (argv != 2) {
        fprintf(stderr, "usage: %s <port>\n", argc[0]);
        exit(1);
    }
    /* 对Cache进行初始化 */
    initCache();
    /* 
     * 对指定端口进行监听，接收来自Web brower的请求：
     *  - 要求多线程实现
     *  - 要求鲁棒性：避免段错误，内存泄露，文件描述符缺失等错误；确保能够对错误输入，恶意输入有鲁班性
     */
    listenfd = Open_listenfd(argc[1]);
    addrlen = sizeof(addr);
    while (1) {
        /* int accept(int listenfd, struct sockaddr *addr, int *addrlen); */
        connfd = (int *)Malloc(sizeof(int)); // 这里因为多线程和API的原因connfd不能定义成int类型且需要Malloc进行分配空间
        *connfd = Accept(listenfd, (SA *)&addr, &addrlen);

        /* $begin DEBUG information */
        Getnameinfo((SA *)&addr, addrlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Proxy accepted connection from (%s, %s)\n", hostname, port);
        /* $end DEBUG information */
        
        /* int pthread_create(pthread_t *tid, pthread_attr_t *attr, void * (*routine)(void *), void *argp); */
        Pthread_create(&tid, NULL, serverThread, connfd);
    }
    return 0;
}
/* end proxy main */

/* $begin initCache */
void initCache()
{
    cachePartNum = MAX_CACHE_SIZE / MAX_OBJECT_SIZE;
    cache = (struct cachePart *)Malloc(sizeof(struct cachePart) * cachePartNum);
    for (int i = 0; i < cachePartNum; i++) {
        cache[i].lruCnt = 0; cache[i].readCnt = 0;
        cache[i].dataSize = 0; cache[i].isNew = 1;
        Sem_init(&cache[i].rdt, 0, 1);
        Sem_init(&cache[i].wrt, 0, 1);
    }
    Sem_init(&lrut, 0, 1);
}
/* $end initCache */

/*
 * 线程处理程序
 */
/* $begin serverThread */
void *serverThread(void *argp)
{
    int connfd = *(int *)argp;
    Free(argp);
    /* 为了避免内存泄漏，每个可结合线程都应该要么被其他线程显式地收回，要么通过调用 pthread_detach 函数被分离 */
    Pthread_detach(Pthread_self());
    client2Proxy(connfd);
    Close(connfd); // 注意关闭文件描述符
    return NULL;
}
/* $end serverThread */

/*
 * proxy线程处理来自客户端的HTTP请求
 */
/* $begin client2Proxy */
void client2Proxy(int connfd)
{
    /* 接收来自Web brower的请求信息 */
    rio_t rp;
    /* 
     * MAXLINE定义在csapp.h中，值为8KB = 8192B
     *  - 现代操作系统页大小一般为 4 KB 或 8 KB，设置缓冲区大小为页的倍数（如 8 KB）可以更高效地进行内存分配和管理。
     *  - 网络协议如 TCP 通常推荐使用缓冲区大小为 8 KB，以适应 MTU（Maximum Transmission Unit）的最大数据包限制，减少分包和重传。
     */
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], hostname[MAXLINE], port[MAXLINE];
    int i, j, flag = 0;

    Rio_readinitb(&rp, connfd);
    /* 接收请求行 */
    Rio_readlineb(&rp, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    /* 目前只实现GET请求 */
    if (!(strcmp(method, "GET") == 0)) {
        clientError(connfd, method, "501", "Not Implemented", 
            "proxy does not implement this method");
        return;
    }

    /* $begin DEBUG information */
    printf("Proxy receive request: %s\n", buf);
    /* $end DEBUG information */

    /* 需要判断下uri中是否包含了hostname和port信息 */
    /* 
     * char *strstr(const char *haystack, const char *needle); 
     * haystack：待搜索的主字符串; needle：要搜索的子串。
     * 返回值: 返回指向该子串在主字符串中第一次出现位置的指针, 否则返回NULL
     * 经过如下操作,若uri包含了hostname和port信息，则将得到如下内容：
     * origin uri: http://www.baidu.com:8080/home.html
     * origin hostname :
     * origin port : 80 
     * now uri: /home.html 
     * now hostname : www.baidu.com
     * now port : 8080
     */
    hostname[0] = '\0';
    port[0] = '8', port[1] = '0', port[2] = '\0'; // 默认HTTP端口为80
    if (strstr(uri, "http://") != NULL) {
        for (i = 7; uri[i] != '\0' && uri[i] != '/' && uri[i] != ':'; i++) hostname[i - 7] = uri[i];
        hostname[i - 7] = '\0';
        if (uri[i] == ':') i++;
        for (j = 0; uri[i] != '\0' && uri[i] != '/'; i++, j++) port[j] = uri[i];
        if (j != 0) port[j] = '\0';
        strcpy(uri, uri + i);
        if (strlen(uri) <= 0) uri[0] = '/', uri[1] = '\0';
    }
    /* 首先需要查询下cache中是否已经缓存过所需的Web Object */
    for (i = 0; i < cachePartNum; i++) {
        P(&cache[i].rdt); cache[i].readCnt++; 
        if (cache[i].readCnt == 1) P(&cache[i].wrt);
        V(&cache[i].rdt);

        if (strcmp(cache[i].hostname, hostname) == 0 && 
            strcmp(cache[i].uri, uri) == 0) {
                flag = 1;
                break;
            }
        
        P(&cache[i].rdt); cache[i].readCnt--; 
        if (cache[i].readCnt == 0) V(&cache[i].wrt);
        V(&cache[i].rdt);
    }
    if (flag) proxy2Client(i, connfd); // 有缓存直接Proxy将Web Object发送给客户端
    else proxy2Server(hostname, port, uri, &rp); // 无缓存创建与Web服务器的连接
}
/* $end client2Proxy */

/*
 * proxy将cache[idx]中缓存的数据发送给客户端
 */
/* $begin proxy2Client */
void proxy2Client(int idx, int connfd)
{
    int i;
    char buf[MAXLINE + 32];
    rio_t rp;

    /* 接收掉请求头 */
    Rio_readinitb(&rp, connfd);
    Rio_readlineb(&rp, buf, MAXLINE);
    /* $begin DEBUG information */
    while (strcmp(buf, "\r\n") != 0) {
        printf("proxy receive request header: %s", buf);
        Rio_readlineb(&rp, buf, MAXLINE);
    }
    printf("\r\n");
    /* $end DEBUG information */

    /* 响应体 */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(connfd, buf, strlen(buf));
    /* 响应头 */
    P(&cache[idx].rdt); cache[idx].readCnt++; 
    if (cache[idx].readCnt == 1) P(&cache[idx].wrt);
    V(&cache[idx].rdt);
    
    sprintf(buf, "Content-type: %s\r\n\r\n", cache[idx].contentTpye);
    
    P(&cache[idx].rdt); cache[idx].readCnt--; 
    if (cache[idx].readCnt == 0) V(&cache[idx].wrt);
    V(&cache[idx].rdt);
    Rio_writen(connfd, buf, strlen(buf));
    /* 响应体 */
    P(&cache[idx].rdt); cache[idx].readCnt++; 
    if (cache[idx].readCnt == 1) P(&cache[idx].wrt);
    V(&cache[idx].rdt);

    Rio_writen(connfd, cache[idx].data, cache[idx].dataSize);

    P(&cache[idx].rdt); cache[idx].readCnt--; 
    if (cache[idx].readCnt == 0) V(&cache[idx].wrt);
    V(&cache[idx].rdt);
    /* 更新LRU计数器 */
    P(&lrut);
    for (i = 0; i < cachePartNum; i++) {
        if (i == idx) cache[i].lruCnt = 0;
        else cache[i].lruCnt++;
    }
    V(&lrut);
}
/* $end proxy2Client */

/*
 * proxy线程向Web服务器发送请求
 */
/* $begin proxy2Server */
void proxy2Server(char *hostname, char *port, char *uri, rio_t *connfdRp)
{
    rio_t rp;
    int clientfd, bodySize = 0, bufSize, flag = 1;
    char buf[MAXLINE], cacheBuf[MAX_OBJECT_SIZE], contentTpye[MAXLINE];
    
    /* proxy向Web服务器发送请求信息 */
    clientfd = Open_clientfd(hostname, port);
    /* 写入请求行 */
    sprintf(buf, "GET %s HTTP/1.0\r\n", uri);
    Rio_writen(clientfd, buf, strlen(buf));
    /* 写入请求头 */
    sprintf(buf, "Host: %s\r\n", hostname);
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    /* 将其余额外的请求头接收并发送给服务器 */
    Rio_readlineb(connfdRp, buf, MAXLINE);
    /* $begin DEBUG information */
    printf("proxy receive request header:\n");
    while (strcmp(buf, "\r\n") != 0) {
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);
        Rio_readlineb(connfdRp, buf, MAXLINE);
    }
    printf("%s", buf);
    /* $end DEBUG information */
    sprintf(buf, "\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    
    /* 从Web服务器上接收响应信息, 然后将从Web服务器上接收到的响应信息转发给客户端即可 */
    Rio_readinitb(&rp, clientfd);
    /* 响应体 */
    Rio_readlineb(&rp, buf, MAXLINE);
    Rio_writen(connfdRp->rio_fd, buf, strlen(buf));
    /* 响应头 */
    contentTpye[0] = '\0';
    Rio_readlineb(&rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n") != 0) {
        Rio_writen(connfdRp->rio_fd, buf, strlen(buf));
        if (strstr(buf, "Content-type:") != NULL) strcpy(contentTpye, buf + 14);
        Rio_readlineb(&rp, buf, MAXLINE);
    }
    if (strlen(contentTpye) == 0) printf("Warning: respone no Content-type\n");
    sprintf(buf, "\r\n");
    Rio_writen(connfdRp->rio_fd, buf, strlen(buf));
    /* 响应体 */
    /* 对于响应体既可能包含文本也可能包含二进制数据，所以需要使用Rio_readnb*/
    while ((bufSize = Rio_readnb(&rp, buf, MAXLINE)) != 0) {
        Rio_writen(connfdRp->rio_fd, buf, bufSize);
        if (flag && (bodySize + bufSize) <= MAX_OBJECT_SIZE){
            memcpy(cacheBuf + bodySize, buf, bufSize);
            bodySize += bufSize;
        } else flag = 0;
    }
    /* 此次proxy与Web服务器的连接结束，关闭文件描述符 */
    Close(clientfd);
    /* 处理Cache */
    if (flag) cacheWebObject(cacheBuf, bodySize, hostname, uri, contentTpye);
    /* flag == 0 表示Web Object超过了MAX_OBJECT_SIZE, 放弃缓存 */
}
/* $end proxy2Server */

/*
 * 将Web Object缓存到Proxy中
 */
/* $begin cacheWebObject */
void cacheWebObject(char* cacheBuf, int bodySize, char* hostname, char* uri, char* contentTpye)
{
    int i, idx = 0, maxCnt = 0;
    /* LRU寻找缓存的位置 */
    P(&lrut);
    for (i = 0; i < cachePartNum; i++) {   
        if (cache[i].isNew) {
            idx = i;
            cache[i].isNew = 0;
            break;
        } else {
            if (maxCnt < cache[i].lruCnt) {
                maxCnt = cache[i].lruCnt;
                idx = i;
            }
        }
    }
    cache[idx].lruCnt = 0;
    V(&lrut);
    /* 将cache[idx]处的缓存换走 */
    P(&cache[idx].wrt);
    strcpy(cache[idx].hostname, hostname);
    strcpy(cache[idx].uri, uri);
    strcpy(cache[idx].contentTpye, contentTpye);
    cache[idx].dataSize = bodySize;
    memcpy(cache[idx].data, cacheBuf, bodySize);
    V(&cache[idx].wrt);
}
/* $end cacheWebObject */

/*
 * 错误响应函数
 */
/* $begin clientError */
void clientError(int fd, char *cause, char *statusCode, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];
    /* 主要每次换行以\r\n进行分隔,响应头结束标志为\r\n */
    /* 响应行 */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", statusCode, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    /* 
     * 响应头 
     * 在 HTTP/1.0 中，Content-Type 至少需要明确设置以保证兼容性；
     * Date 和 Content-Length 也非常常见并被推荐。
     */
    sprintf(buf, "Content-Type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));
    /* 响应体 */
    sprintf(buf, "<html>\r\n<title>Proxy Error</title>\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", statusCode, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s</p>\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr>\r\n<em>The Tiny Web server</em>\r\n</html>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clientError */