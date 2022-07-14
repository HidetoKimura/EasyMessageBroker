#include <stdio.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include "ez_stream.h"
#include "ez_log.h"

using namespace std;

#define WRAP_SYSCALL(result_, syscall_)         \
    do {                                        \
        result_ = syscall_;                     \
        if( result_ < 0 && errno == EINTR) {    \
            errno = 0;                          \
        } else {                                \
            break;                              \
        }                                       \
    } while(1);                                 \

EventLoop::EventLoop() 
{
    m_epoll_fd  = -1;
    m_running   = true;
    m_item_list = {};

    WRAP_SYSCALL(m_epoll_fd, epoll_create1(0));
    if (m_epoll_fd < 0) {
        LOGE << "epoll_create failed:";
        return;
    }
}

EventLoop::~EventLoop() 
{
    int ret;
    m_running = false;
    if(m_epoll_fd) {
        WRAP_SYSCALL(ret, ::close(m_epoll_fd));
    }
}
        
void EventLoop::add_item(EventLoopItem &item)
{
    struct epoll_event ev;
    int ret;

    if(m_epoll_fd == -1) return;

    ev.data.fd = item.fd;
    ev.events = EPOLLIN;

    WRAP_SYSCALL(ret, ::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, item.fd, &ev));
    if (ret < 0) {
        LOGE << "epoll_ctl::EPOLL_CTL_ADD";
        ::perror("epoll_ctl::EPOLL_CTL_ADD");
        WRAP_SYSCALL(ret, ::close(m_epoll_fd));
        return;
    }
    m_item_list.push_back(item);
}

void EventLoop::del_item(int fd) 
{

    for (auto it = m_item_list.begin(); it != m_item_list.end();)
    {
        if((*it).fd == fd) {
            it = m_item_list.erase(it);
        }
        else {
            it++;
        }
    }

}
 
void EventLoop::run(void)
{
    #define MAX_EVENTS 16
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if(m_epoll_fd == -1) return;

    while(m_running) {
        WRAP_SYSCALL(nfds, ::epoll_wait(m_epoll_fd, events, MAX_EVENTS, -1));
        if (nfds == -1) {
            LOGE << "epoll_wait";
            break;
        }

        for (int n = 0; n < nfds; n++) {
            for (auto it = m_item_list.begin(); it != m_item_list.end(); it++)
            {
                if((*it).fd == events[n].data.fd) {
                    (*it).dispatch((*it).fd);
                }
            }

        }
        
    }
}

#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

SocketStream::SocketStream(std::string addr)
{
    m_addr = addr;
}

SocketStream::~SocketStream()
{
    int ret;

    WRAP_SYSCALL(ret, ::unlink(m_addr.c_str()));
}

int SocketStream::listen(void)
{
    #define MAX_LISTEN  128
    int ret, val, fd;

    WRAP_SYSCALL(fd, ::socket(AF_UNIX, SOCK_STREAM, 0));
    if (fd < 0)
    {
        ::perror("socket error:");
        return -1;
    }

    sockaddr_un addr;
    ::bzero(&addr, sizeof addr);
    addr.sun_family = AF_UNIX;
    ::strcpy(addr.sun_path, m_addr.c_str());

    // delete file that going to bind
    WRAP_SYSCALL(ret, ::unlink(m_addr.c_str()));

    WRAP_SYSCALL(ret, ::bind(fd, (struct sockaddr*)&addr, sizeof addr));
    if (ret < 0)
    {
        ::perror("bind error:");
        return -1;
    }

    WRAP_SYSCALL(ret, ::listen(fd, MAX_LISTEN));
    if (ret < 0)
    {
        ::perror("listen error:");
        return -1;
    }

    //NON BLOCKING
    val = 1;
    WRAP_SYSCALL(ret, ::ioctl(fd, FIONBIO, &val));
    if (ret < 0)
    {
        ::perror("ioctl error:");
        return -1;
    }

    struct sigaction act;
    ::memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = SIG_IGN;

    WRAP_SYSCALL(ret, sigaction(SIGPIPE, &act, NULL));

    return fd;

}

int SocketStream::connect(void)
{
    int ret, val, fd;

    WRAP_SYSCALL(fd, ::socket(AF_UNIX, SOCK_STREAM, 0));
    if (fd < 0)
    {
        ::perror("socket error:");
        return -1;
    }

    struct sockaddr_un addr;
    ::bzero(&addr, sizeof addr);

    addr.sun_family = AF_UNIX;
    ::strcpy(addr.sun_path, m_addr.c_str());

    WRAP_SYSCALL(ret, ::connect(fd, (struct sockaddr*)&addr, sizeof addr));
    if (ret < 0)
    {
        ::perror("connect error:");
        return -1;
    }

    //NON BLOCKING
    val = 1;
    WRAP_SYSCALL(ret, ::ioctl(fd, FIONBIO, &val));
    if (ret < 0)
    {
        ::perror("ioctl error:");
        return -1;
    }

    struct sigaction act;
    ::memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = SIG_IGN;

    WRAP_SYSCALL(ret, sigaction(SIGPIPE, &act, NULL));

    return fd;
}

int  SocketStream::accept(int listen_fd)
{
    int ret, val, conn_sock;
    sockaddr_un addr;
    socklen_t addrlen = 0;
 
    WRAP_SYSCALL(conn_sock, ::accept(listen_fd, (struct sockaddr *) &addr, &addrlen));
    if (conn_sock < 0) {
        ::perror("accept failed:");
        return -1;
    }

    //NON BLOCKING
    val = 1;
    WRAP_SYSCALL(ret, ::ioctl(conn_sock, FIONBIO, &val));
    if (ret < 0)
    {
        ::perror("ioctl error:");
        return -1;
    }

    return conn_sock;
}

#define SOCKET_HEAD_SIGN    0x11223344

typedef struct {
    uint32_t    head_sign;
    uint32_t    body_len;
} ss_msg_t;
    
int32_t SocketStream::read(int fd, void* buf, int32_t size)
{
    #define MILLI_SEC   1000000
    ss_msg_t    *header;
    int         r_len, len, ret;
    char        *p_buf;
    struct timespec req = {0, 100 * MILLI_SEC};

    r_len = sizeof(header);
    p_buf = (char*)alloca(r_len);
    header = (ss_msg_t*)p_buf;
    len = 0;

    do {
        WRAP_SYSCALL(len, ::read(fd, (void*)p_buf, r_len));
        if (len == r_len) 
        {
            break;
        }
        else if (len < 0 && errno == EAGAIN)  
        {
            ::perror("read error:");
            WRAP_SYSCALL(ret, ::nanosleep(&req, NULL));
            continue;
        }
        else if (len < 0)  
        {
            ::perror("read error:");
            return -1; 
        }
        else if (len == 0)  
        {
            return 0; 
        }

        p_buf   += len;
        r_len   -= len;

    } while(1);

    if(header->head_sign != SOCKET_HEAD_SIGN)
    {
        LOGE << "socket header was destroyed.";
        return -1;
    }
    r_len = header->body_len;
    if(r_len > size)
    {
        LOGE << "buffer size was too small.";
        return -1;
    }

    p_buf = (char*)buf;

    do {
        WRAP_SYSCALL(len, ::read(fd, (void*)p_buf, r_len));
        if (len == r_len) 
        {
            break;
        }
        else if (len < 0 && errno == EAGAIN)  
        {
            ::perror("read error:");
            WRAP_SYSCALL(ret, ::nanosleep(&req, NULL));
            continue;
        }
        else if (len < 0)  
        {
            ::perror("read error:");
            return -1; 
        }
        else if (len == 0)  
        {
            return 0; 
        }

        p_buf   += len;
        r_len   -= len;

    } while(1);

    return header->body_len;
}

int32_t SocketStream::write(int fd, void* buf, int32_t size)
{
    #define MILLI_SEC   1000000
    ss_msg_t    *header;
    int         w_len, len, ret;
    char        *p_buf;
    struct timespec req = {0, 100 * MILLI_SEC};

    w_len = sizeof(header);
    p_buf = (char*)alloca(w_len);
    header = (ss_msg_t*)p_buf;
    header->head_sign = SOCKET_HEAD_SIGN;
    header->body_len = size;

    len = 0;

    do {
        WRAP_SYSCALL(len, ::write(fd, (void*)p_buf, w_len));
        if (len == w_len) 
        {
            break;
        }
        else if (len < 0 && errno == EAGAIN)  
        {
            ::perror("write error:");
            WRAP_SYSCALL(ret, ::nanosleep(&req, NULL));
            continue;
        }
        else if (len < 0)  
        {
            ::perror("write error:");
            return -1; 
        }
        else if (len == 0)  
        {
            return 0; 
        }

        p_buf   += len;
        w_len   -= len;

    } while(1);

    p_buf = (char*)buf;
    w_len = size;

    do {
        WRAP_SYSCALL(len, ::write(fd, (void*)p_buf, w_len));
        if (len == w_len) 
        {
            break;
        }
        else if (len < 0 && errno == EAGAIN)  
        {
            ::perror("read error:");
            WRAP_SYSCALL(ret, ::nanosleep(&req, NULL));
            continue;
        }
        else if (len < 0)  
        {
            ::perror("read error:");
            return -1; 
        }
        else if (len == 0)  
        {
            return 0; 
        }

        p_buf   += len;
        w_len   -= len;

    } while(1);

    return header->body_len;
}


